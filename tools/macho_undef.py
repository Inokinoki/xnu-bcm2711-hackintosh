#!/usr/bin/env python3
"""Report undefined symbols of the Mach-O files in a userland root that no library they load provides.

  macho_undef.py ROOT [--fail]

For each thin arm64 Mach-O under ROOT (executables and dylibs), collect the undefined, non-weak external
symbols and look for them in the exports of every library in its load closure (LC_LOAD_DYLIB,
LC_REEXPORT_DYLIB and so on, resolved by install name inside ROOT). Libraries that are not in ROOT
(the SDK's libSystem pieces we do not build) are listed once as "missing library" and their symbols are
reported as unresolved: that is the work list of stand-ins still to write. The output is grouped by symbol
with the files that need it. Exit status 1 with --fail when anything is unresolved.

Exports come from the export trie (it has re-exports and alias_list symbols, which the symbol table does not),
undefined symbols from LC_SYMTAB.
"""
import os
import struct
import sys

MH_MAGIC_64 = 0xfeedfacf
LC_SYMTAB = 0x2
LC_LOAD_DYLIB = 0xc
LC_LOAD_WEAK_DYLIB = 0x80000018
LC_REEXPORT_DYLIB = 0x8000001f
LC_LAZY_LOAD_DYLIB = 0x20
LC_LOAD_UPWARD_DYLIB = 0x80000023
LC_ID_DYLIB = 0xd
LC_DYLD_INFO, LC_DYLD_INFO_ONLY, LC_DYLD_EXPORTS_TRIE = 0x22, 0x80000022, 0x80000033
DYLIB_CMDS = {LC_LOAD_DYLIB, LC_LOAD_WEAK_DYLIB, LC_REEXPORT_DYLIB, LC_LAZY_LOAD_DYLIB, LC_LOAD_UPWARD_DYLIB}

N_STAB, N_PEXT, N_TYPE, N_EXT = 0xe0, 0x10, 0x0e, 0x01
N_UNDF, N_SECT = 0x0, 0xe
N_WEAK_REF, N_WEAK_DEF = 0x40, 0x80

# provided by dyld / the linker, not by a dylib
BUILTIN = {"dyld_stub_binder", "___dso_handle", "__mh_execute_header", "__mh_dylib_header", "__mh_bundle_header"}


def uleb(buf, pos):
    result = shift = 0
    while True:
        b = buf[pos]
        pos += 1
        result |= (b & 0x7f) << shift
        if not b & 0x80:
            return result, pos
        shift += 7


def trie_names(buf, off, size):
    """every symbol name in the export trie at buf[off:off+size]"""
    names = []
    end = off + size
    stack = [(off, b"")]
    seen = set()
    while stack:
        node, prefix = stack.pop()
        if node in seen or node >= end:
            continue
        seen.add(node)
        pos = node
        terminal, pos = uleb(buf, pos)
        if terminal:
            names.append(prefix.decode(errors="replace"))
        pos += terminal
        nchildren = buf[pos]
        pos += 1
        for _ in range(nchildren):
            e = buf.index(b"\0", pos)
            label = buf[pos:e]
            pos = e + 1
            child, pos = uleb(buf, pos)
            stack.append((off + child, prefix + label))
    return names


def parse(path):
    """-> dict(id, loads, reexports, exports, undefined) or None if not a thin arm64 Mach-O."""
    with open(path, "rb") as f:
        buf = f.read()
    if len(buf) < 32 or struct.unpack_from("<I", buf, 0)[0] != MH_MAGIC_64:
        return None
    cputype, = struct.unpack_from("<I", buf, 4)
    if cputype != 0x0100000c:  # CPU_TYPE_ARM64
        return None
    ncmds, = struct.unpack_from("<I", buf, 16)
    off = 32
    info = {"id": None, "loads": [], "reexports": [], "exports": set(), "undefined": set()}
    symoff = nsyms = stroff = 0
    trie = None
    for _ in range(ncmds):
        cmd, size = struct.unpack_from("<II", buf, off)
        if cmd == LC_SYMTAB:
            symoff, nsyms, stroff, _ = struct.unpack_from("<4I", buf, off + 8)
        elif cmd in (LC_DYLD_INFO, LC_DYLD_INFO_ONLY):
            eoff, esize = struct.unpack_from("<II", buf, off + 40)
            if esize:
                trie = (eoff, esize)
        elif cmd == LC_DYLD_EXPORTS_TRIE:
            eoff, esize = struct.unpack_from("<II", buf, off + 8)
            if esize:
                trie = (eoff, esize)
        elif cmd in DYLIB_CMDS or cmd == LC_ID_DYLIB:
            nameoff, = struct.unpack_from("<I", buf, off + 8)
            name = buf[off + nameoff:off + size].split(b"\0", 1)[0].decode()
            if cmd == LC_ID_DYLIB:
                info["id"] = name
            else:
                info["loads"].append(name)
                if cmd == LC_REEXPORT_DYLIB:
                    info["reexports"].append(name)
        off += size
    for i in range(nsyms):
        strx, ntype, _, desc, _ = struct.unpack_from("<IBBHQ", buf, symoff + 16 * i)
        if ntype & N_STAB or not ntype & N_EXT:
            continue
        end = buf.index(b"\0", stroff + strx)
        name = buf[stroff + strx:end].decode(errors="replace")
        kind = ntype & N_TYPE
        if kind == N_UNDF:
            if not desc & N_WEAK_REF:
                info["undefined"].add(name)
        elif kind == N_SECT and not ntype & N_PEXT and trie is None:
            info["exports"].add(name)
    if trie:
        info["exports"] |= set(trie_names(buf, *trie))
    return info


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    fail = "--fail" in sys.argv
    if len(args) != 1:
        sys.exit(__doc__)
    root = os.path.abspath(args[0])

    files = {}  # path inside root -> info
    for d, dirs, names in os.walk(root):
        dirs[:] = [x for x in dirs if not x.endswith(".dSYM")]
        for n in names:
            p = os.path.join(d, n)
            if os.path.islink(p) or not os.path.isfile(p):
                continue
            info = parse(p)
            if info:
                files["/" + os.path.relpath(p, root)] = info
    by_id = {i["id"]: i for i in files.values() if i["id"]}
    by_path = dict(files)

    def lookup(install_name):
        return by_id.get(install_name) or by_path.get(install_name)

    def exports_of(lib, seen):
        """exports of LIB plus those of everything it re-exports"""
        if id(lib) in seen:
            return set()
        seen.add(id(lib))
        out = set(lib["exports"])
        for r in lib["reexports"]:
            sub = lookup(r)
            if sub:
                out |= exports_of(sub, seen)
        return out

    missing_libs = {}
    unresolved = {}  # symbol -> set of files
    for path, info in sorted(files.items()):
        provided = set()
        for l in info["loads"]:
            lib = lookup(l)
            if lib:
                provided |= exports_of(lib, set())
            else:
                missing_libs.setdefault(l, set()).add(path)
        provided |= info["exports"]  # a file's own definitions (and the executable's)
        for sym in info["undefined"]:
            if sym not in provided and sym not in BUILTIN:
                unresolved.setdefault(sym, set()).add(path)

    print(f"{len(files)} Mach-O files, {len(unresolved)} unresolved symbols")
    for l, users in sorted(missing_libs.items()):
        print(f"missing library {l} (loaded by {len(users)} files)")
    for sym, users in sorted(unresolved.items()):
        shown = ", ".join(sorted(users)[:4]) + (" ..." if len(users) > 4 else "")
        print(f"  {sym}  <- {shown}")
    sys.exit(1 if fail and unresolved else 0)


if __name__ == "__main__":
    main()
