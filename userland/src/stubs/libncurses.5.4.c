/*
 * libncurses (termcap interface only, enough for bash and ls to link and run): no terminal database, so tgetent
 * reports "no termcap" and callers fall back to dumb-terminal behaviour. Replace with ncurses-61 when a command
 * needs real curses.
 */
#include <stddef.h>

int tgetent(char *bp, const char *name) { (void)bp; (void)name; return -1; }
int tgetflag(const char *id) { (void)id; return 0; }
int tgetnum(const char *id) { (void)id; return -1; }
char *tgetstr(const char *id, char **area) { (void)id; (void)area; return NULL; }
char *tgoto(const char *cap, int col, int row) { (void)cap; (void)col; (void)row; return NULL; }
int tputs(const char *str, int affcnt, int (*putc_fn)(int))
{
	(void)affcnt;
	for (; str && *str; str++) {
		putc_fn(*str);
	}
	return 0;
}
