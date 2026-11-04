#!/bin/sh
# text_cmds sort: its crash-signal handler exits 255 without a word, which hides why sort dies on this port.
# Make it say which signal, at what address and in which image/symbol (pc and lr), then exit as before.
#
#   userland/patch-sort.sh SRCDIR/text_cmds
set -eu
F="${1:?usage: $0 TEXT_CMDS_DIR}/sort/sort.c"
python3 - "$F" <<'PY'
import sys
p = sys.argv[1]
s = open(p).read()
if "userland/patch-sort.sh" in s:
    sys.exit(0)
old = "\tclear_tmp_files();\n\texit(-1);\n}"
new = '''\t/* userland/patch-sort.sh */
	{
		ucontext_t *uc = context;
		unsigned long long pc = uc->uc_mcontext->__ss.__pc, lr = uc->uc_mcontext->__ss.__lr;
		Dl_info di;
		char buf[256];
		int n = snprintf(buf, sizeof buf, "sort: signal %d addr %p pc %#llx lr %#llx\\n", sig,
		    siginfo ? siginfo->si_addr : 0, pc, lr);
		write(2, buf, n);
		if (dladdr((void *)pc, &di))
			n = snprintf(buf, sizeof buf, "sort: pc in %s %s+%#lx\\n", di.dli_fname,
			    di.dli_sname ? di.dli_sname : "?", (unsigned long)((char *)pc - (char *)di.dli_saddr));
		else
			n = snprintf(buf, sizeof buf, "sort: pc not in an image\\n");
		write(2, buf, n);
		if (dladdr((void *)lr, &di))
			n = snprintf(buf, sizeof buf, "sort: lr in %s %s+%#lx\\n", di.dli_fname,
			    di.dli_sname ? di.dli_sname : "?", (unsigned long)((char *)lr - (char *)di.dli_saddr));
		else
			n = snprintf(buf, sizeof buf, "sort: lr not in an image\\n");
		write(2, buf, n);
	}
	clear_tmp_files();
	exit(-1);
}'''
assert old in s
s = s.replace(old, new, 1)
s = s.replace("sig_handler(int sig __unused, siginfo_t *siginfo __unused,\n    void *context __unused)", "sig_handler(int sig, siginfo_t *siginfo, void *context)", 1)
s = s.replace("#include <err.h>", "#include <err.h>\n#include <dlfcn.h>\n#include <sys/ucontext.h>", 1)
assert "<dlfcn.h>" in s
open(p, "w").write(s)
PY

# sort_list_add: print what the allocation looks like (layout debugging, see docs/userland-plan.md)
G="${1}/sort/file.c"
python3 - "$G" <<'PY'
import sys
p = sys.argv[1]
s = open(p).read()
if "userland/patch-sort.sh" in s:
    sys.exit(0)
old = "\t\tl->list[indx] = sort_list_item_alloc();\n\t\tsort_list_item_set(l->list[indx], str);\n"
new = old + """\t\t/* userland/patch-sort.sh */
\t\tif (getenv("SORT_DEBUG")) {
\t\t\tstruct sort_list_item *it = l->list[indx];
\t\t\tdprintf(2, "sort_list_add: indx %zu list %p item %p str %p ka %p keys_num %zu keys_array_size %zu need_hint %d "
\t\t\t    "sizeof(item) %zu sizeof(key_value) %zu sizeof(key_hint) %zu k0 %p len %zu\\n",
\t\t\t    indx, (void *)l->list, (void *)it, (void *)str, (void *)&it->ka, keys_num, keys_array_size(),
\t\t\t    (int)need_hint, sizeof(struct sort_list_item), sizeof(struct key_value), sizeof(struct key_hint),
\t\t\t    (void *)get_key_from_keys_array(&it->ka, 0)->k, str->len);
\t\t}
"""
assert old in s
s = s.replace(old, new, 1)
if "extern bool need_hint" not in s:
    s = s.replace('#include "file.h"', '#include "file.h"\nextern bool need_hint;', 1)
open(p, "w").write(s)
PY
