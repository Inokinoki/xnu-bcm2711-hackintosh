#!/bin/sh
# Gate for userland Mach-O files: they must run on the Pi 4 kernel, i.e. plain arm64 for a Cortex-A72
# (ARMv8.0, no pointer authentication, no LSE atomics). Needs lipo, otool and codesign (macOS).
#
#   tools/check-arm64-plain.sh FILE|DIR...
#
# Per Mach-O file, checks: only the arm64 slice (not arm64e, not x86_64); cpusubtype ALL; no PAC, LSE or
# RCpc instruction in __text; every dependency is under /usr/lib (our own root, not the SDK or a
# system framework); an ad-hoc or better code signature. Non-Mach-O files are skipped.
# Exit status is the number of failing files.
set -u
BAD_INSN='^(pac[a-z0-9]*|aut[a-z0-9]*|xpac[a-z0-9]*|bra[ab]z?|blra[ab]z?|retaa|retab|eretaa|eretab|ldra[ab]|ldadd[a-z]*|ldclr[a-z]*|ldset[a-z]*|ldeor[a-z]*|ldsmax[a-z]*|ldsmin[a-z]*|ldumax[a-z]*|ldumin[a-z]*|swp[a-z]*|cas[a-z]*|ldapr[a-z]*|ldapur[a-z]*|stlur[a-z]*)$'
OK_DEP='^/usr/lib/(libSystem\.B\.dylib|system/[^/]+\.dylib|libc\+\+(abi)?(\.1)?\.dylib|lib[A-Za-z0-9_+.-]+\.dylib)$'
fail=0

report() {
	if [ -n "$2" ]; then
		echo "FAIL $1:$2"
		fail=$((fail + 1))
	else
		echo "ok   $1"
	fi
}

check() {
	f=$1
	archs=$(lipo -archs "$f" 2>/dev/null) || return 0
	bad=
	[ "$archs" = arm64 ] || bad="$bad archs='$archs' (want arm64 only);"
	if [ "$archs" = arm64 ]; then
		otool -hv -arch arm64 "$f" 2>/dev/null | grep -Eq 'ARM64 +ALL' ||
			bad="$bad cpusubtype is not ALL (arm64e/PAC ABI?);"
		insn=$(otool -tv -arch arm64 "$f" 2>/dev/null | awk -F'\t' -v re="$BAD_INSN" '$2 ~ re {print $2}' | sort | uniq -c | sort -rn | head -5)
		[ -z "$insn" ] || bad="$bad forbidden instructions:$(echo "$insn" | tr '\n' ' ');"
		# static archives and object files have no dependency list and no signature
		[ "$(head -c 7 "$f")" = '!<arch>' ] && { report "$f" "$bad"; return 0; }
		ftype=$(otool -hv -arch arm64 "$f" 2>/dev/null | awk 'END{print $5}')
		if [ "$ftype" != OBJECT ]; then
			deps=$(otool -L -arch arm64 "$f" 2>/dev/null | sed 1d | awk '{print $1}' | grep -Ev "$OK_DEP" | tr '\n' ' ')
			[ -z "$deps" ] || bad="$bad dependencies outside /usr/lib: $deps;"
			codesign -dv "$f" >/dev/null 2>&1 || bad="$bad unsigned;"
		fi
	fi
	report "$f" "$bad"
}

[ $# -gt 0 ] || { echo "usage: $0 FILE|DIR..." >&2; exit 2; }
for p in "$@"; do
	if [ -d "$p" ]; then
		for f in $(find "$p" -type f -not -path '*.dSYM/*'); do check "$f"; done
	else
		check "$p"
	fi
done
exit "$fail"
