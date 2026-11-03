/*
 * What openlibm leaves to its libc / assumes about long double. On arm64 Darwin long double is the same type as
 * double, so the long double entry points are the double ones.
 */
#include <math.h>

double scalbn(double x, int n);

double ldexp(double x, int n) { return scalbn(x, n); }
long double ldexpl(long double x, int n) { return scalbn(x, n); }
long double scalbnl(long double x, int n) { return scalbn(x, n); }

/*
 * <fenv.h> on arm64 Darwin: fenv_t is { __fpsr, __fpcr }, the exception flags live in FPSR and the rounding mode
 * in FPCR bits 23:22. openlibm has no aarch64 fenv for Darwin and Apple's libm is closed.
 */
typedef struct { unsigned int __fpsr, __fpcr; } fenv_t;
typedef unsigned short fexcept_t;

#define EXC_MASK 0x9f /* IOC DZC OFC UFC IXC IDC (bits 0-4, 7) */
#define RND_SHIFT 22
#define RND_MASK (3u << RND_SHIFT)

static inline unsigned int rd_fpsr(void) { unsigned long v; __asm__ volatile("mrs %0, fpsr" : "=r"(v)); return (unsigned int)v; }
static inline unsigned int rd_fpcr(void) { unsigned long v; __asm__ volatile("mrs %0, fpcr" : "=r"(v)); return (unsigned int)v; }
static inline void wr_fpsr(unsigned int v) { __asm__ volatile("msr fpsr, %0" : : "r"((unsigned long)v)); }
static inline void wr_fpcr(unsigned int v) { __asm__ volatile("msr fpcr, %0" : : "r"((unsigned long)v)); }

int fegetenv(fenv_t *e) { e->__fpsr = rd_fpsr(); e->__fpcr = rd_fpcr(); return 0; }
int fesetenv(const fenv_t *e) { wr_fpsr(e->__fpsr); wr_fpcr(e->__fpcr); return 0; }
int feholdexcept(fenv_t *e) { fegetenv(e); wr_fpsr(e->__fpsr & ~EXC_MASK); wr_fpcr(e->__fpcr & ~(EXC_MASK << 8)); return 0; }
int feupdateenv(const fenv_t *e) { unsigned int raised = rd_fpsr() & EXC_MASK; fesetenv(e); wr_fpsr(rd_fpsr() | raised); return 0; }
int fegetround(void) { return (int)(rd_fpcr() & RND_MASK); }
int fesetround(int mode) { if (mode & ~RND_MASK) return -1; wr_fpcr((rd_fpcr() & ~RND_MASK) | (unsigned int)mode); return 0; }
int feclearexcept(int excepts) { wr_fpsr(rd_fpsr() & ~((unsigned int)excepts & EXC_MASK)); return 0; }
int fetestexcept(int excepts) { return (int)(rd_fpsr() & (unsigned int)excepts & EXC_MASK); }
int feraiseexcept(int excepts) { wr_fpsr(rd_fpsr() | ((unsigned int)excepts & EXC_MASK)); return 0; }
int fegetexceptflag(fexcept_t *flagp, int excepts) { *flagp = (fexcept_t)(rd_fpsr() & (unsigned int)excepts & EXC_MASK); return 0; }
int fesetexceptflag(const fexcept_t *flagp, int excepts) { unsigned int m = (unsigned int)excepts & EXC_MASK; wr_fpsr((rd_fpsr() & ~m) | (*flagp & m)); return 0; }
