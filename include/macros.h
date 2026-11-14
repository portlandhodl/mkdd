#ifndef MACROS_H
#define MACROS_H

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define CLAMP(low, high, x) \
    ((x) > (high) ? (high) : ((x) < (low) ? (low) : (x)))

#define ROUND_UP(x, align) (((x) + (align)-1) & (-(align)))
#define ROUND_UP_PTR(x, align) \
    ((void *)((((u32)(x)) + (align)-1) & (~((align)-1))))

#define ROUND_DOWN(x, align) ((x) & (-(align)))
#define ROUND_DOWN_PTR(x, align) ((void *)(((u32)(x)) & (~((align)-1))))

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#define STRING_SIZE(x) (sizeof(x) - 1) // for char arrays, subtract 1 to act as null terminator?

#define CLEAR_PATH(x) __memclr((x), sizeof((x)))

#define ALIGN(x) __attribute__((aligned(x)))
#define ATTRIBUTE_ALIGN(x) __attribute__((aligned(x)))
#ifdef __MWERKS__
#define DECL_SECTION(x) __declspec(section x)
#define DECL_WEAK __declspec(weak)
#define DONT_INLINE __attribute__((never_inline))
// inline definitions placed in a .cpp to control weak emission; the host build needs them out of line
#define DECOMP_INLINE inline
#define DECOMP_MWERKS 1
// big-endian data read through typed loads (left big-endian in the host port)
#define DECOMP_BE16(p) (*(u16 *)(p))
#define DECOMP_BE32(p) (*(u32 *)(p))
// choose the original expression, or a host-port replacement
#define DECOMP_HOST(orig, host) orig
#else
#define DECL_SECTION(x)
#define DECL_WEAK __attribute__((weak))
#define DONT_INLINE __attribute__((noinline))
#define DECOMP_INLINE
#define DECOMP_MWERKS 0
#define DECOMP_BE16(p) ((u16)(((const u8 *)(p))[0] << 8 | ((const u8 *)(p))[1]))
#define DECOMP_BE32(p) ((u32)((const u8 *)(p))[0] << 24 | (u32)((const u8 *)(p))[1] << 16 | (u32)((const u8 *)(p))[2] << 8 | ((const u8 *)(p))[3])
#define DECOMP_HOST(orig, host) host
#endif
// 3D perspective aspect ratio: the host port's widescreen option widens it (port_wide_aspect,
// 1 = the original 4:3, 4/3 = anamorphic 16:9)
#if DECOMP_MWERKS
#define DECOMP_WIDE(aspect) (aspect)
#else
#ifdef __cplusplus
extern "C" float port_wide_aspect;
#else
extern float port_wide_aspect;
#endif
#define DECOMP_WIDE(aspect) ((aspect) * port_wide_aspect)
#endif
// integer division that PowerPC's divw/divwu tolerate by zero (wasm traps)
#define DECOMP_IDIV(a, b) DECOMP_HOST((a) / (b), ((b) == 0 ? 0 : (a) / (b)))

// Align X to the previous N bytes (N must be power of two)
#define ALIGN_PREV(X, N) ((X) & ~((N)-1))
// Align X to the next N bytes (N must be power of two)
#define ALIGN_NEXT(X, N) ALIGN_PREV(((X) + (N)-1), N)
#define IS_ALIGNED(X, N) (((X) & ((N)-1)) == 0)
#define IS_NOT_ALIGNED(X, N) (((X) & ((N)-1)) != 0)

#define READU32_BE(ptr, offset) \
    (((u32)ptr[offset] << 24) | ((u32)ptr[offset + 1] << 16) | ((u32)ptr[offset + 2] << 8) | (u32)ptr[offset + 3]);

#define FLAG_ON(V, F) (((V) & (F)) == 0)
#define FLAG_OFF(V, F) (((V) & (F)))

// inserts empty bytes for unknown sections in classes/structs
#define PLACEHOLDER_BYTES(start, end) u8 _##start[end - start]

// Codewarrior tricks for matching decomp
// (Functions are given prototypes for -requireprotos)
#ifdef __MWERKS__
// Force BSS order
#define CW_FORCE_BSS(module, ...)    \
    void fake_function(...);         \
    void FORCE_BSS##module##x(void); \
    void FORCE_BSS##module##x(void)  \
    {                                \
        fake_function(__VA_ARGS__);  \
    }
// Force strings into pool
#define CW_FORCE_STRINGS(module, ...) \
    void fake_function(...);          \
    void FORCE_STRINGS##module(void); \
    void FORCE_STRINGS##module(void)  \
    {                                 \
        fake_function(__VA_ARGS__);   \
    }
#define ASM asm
#else
#define CW_FORCE_BSS(module, ...)
#define CW_FORCE_STRINGS(module, ...)
#define ASM
#endif

// For VSCode
#ifdef __INTELLISENSE__
#define asm
#define __attribute__(x)
#define __declspec(x)
#endif

#endif
