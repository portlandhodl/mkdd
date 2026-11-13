/* Force-included into every translation unit of the host (WebAssembly) port.
 * Provides the Metrowerks/Gekko compiler intrinsics the decomp source uses. */
#ifndef MKDD_PORT_H
#define MKDD_PORT_H

#ifndef __MWERKS__

#ifdef __cplusplus
extern "C" {
#endif

static inline int __cntlzw(unsigned int x) { return x ? __builtin_clz(x) : 32; }

static inline unsigned int __mkdd_rotl(unsigned int x, int n)
{
    n &= 31;
    return n ? (x << n) | (x >> (32 - n)) : x;
}

static inline unsigned int __mkdd_mask(int mb, int me)
{
    unsigned int m = 0;
    int i = mb;
    for (;;) {
        m |= 0x80000000u >> i;
        if (i == me)
            break;
        i = (i + 1) & 31;
    }
    return m;
}

static inline int __rlwinm(int rs, int sh, int mb, int me)
{
    return (int)(__mkdd_rotl((unsigned int)rs, sh) & __mkdd_mask(mb, me));
}

static inline int __rlwimi(int ra, int rs, int sh, int mb, int me)
{
    unsigned int m = __mkdd_mask(mb, me);
    return (int)((__mkdd_rotl((unsigned int)rs, sh) & m) | ((unsigned int)ra & ~m));
}

static inline float __fabsf(float x) { return __builtin_fabsf(x); }
static inline double __fabs(double x) { return __builtin_fabs(x); }
static inline double __frsqrte(double x) { return 1.0 / __builtin_sqrt(x); }
static inline float __fres(float x) { return 1.0f / x; }
static inline double __fsel(double c, double a, double b) { return c >= 0.0 ? a : b; }
static inline int __abs(int x) { return x < 0 ? -x : x; }
static inline void *__memcpy(void *d, const void *s, unsigned long n) { return __builtin_memcpy(d, s, n); }

#ifdef __cplusplus
}
#endif

/* __declspec(weak) -> __mkdd_declspec_weak; section placement is dropped. */
#define __declspec(x) __mkdd_declspec_##x
#define __mkdd_declspec_weak __attribute__((weak))
#define __mkdd_declspec_section __mkdd_drop_section
#define __mkdd_drop_section(s)

/* Fakematches that name MWCC-mangled symbols directly; port/sdk/game_missing.cpp
 * fills these host-side stand-ins at startup. */
#define __vt__11J3DTexNoAnm port_vt_J3DTexNoAnm
#define mscaCallBackTbl__11DriverModel port_DriverModel_mscaCallBackTbl

#endif /* !__MWERKS__ */
#endif /* MKDD_PORT_H */
