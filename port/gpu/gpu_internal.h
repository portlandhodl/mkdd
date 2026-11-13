/* Internal interface between the parts of the GPU front end (port/gpu). */
#ifndef PORT_GPU_INTERNAL_H
#define PORT_GPU_INTERNAL_H

#include <stdint.h>
#include <string.h>
#include "gpu.h"

#ifdef __cplusplus
extern "C" {
#endif

extern GpuRegs gpu_r;
extern GpuStats gpu_cur;

/* Nonzero when draws must be decoded and transformed (a backend that renders
 * is attached). With the null backend draws are only measured and skipped. */
extern int gpu_xform_enabled;

/* The active backend (never NULL). */
extern const GpuBackend *gpu_backend;

static inline uint32_t gpu_be16(const uint8_t *p) { return (uint32_t)p[0] << 8 | p[1]; }
static inline uint32_t gpu_be32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static inline float gpu_xf_float(uint32_t reg) /* XF register 0x1000 + reg as float */
{
    float f;
    memcpy(&f, &gpu_r.xf[reg], 4);
    return f;
}

/* Is [addr, addr + size) inside wasm linear memory? (Guards indexed reads.) */
int gpu_ram_ok(uint32_t addr, uint32_t size);

/* vertex loader (gpu_vtx.c) */
void gpu_vtx_invalidate(void);                 /* CP VCD/VAT changed */
uint32_t gpu_vtx_size(int fmt);                /* bytes per vertex of format `fmt` */
/* Decode, transform and assemble `count` vertices of primitive `prim` (opcode
 * bits 3-5) in format `fmt` from big-endian `data`, and hand them to the backend. */
void gpu_vtx_draw(int prim, int fmt, uint32_t count, const uint8_t *data);

/* debug software rasteriser (gpu_dump.c): returns a backend if MKDD_DUMP_FRAMES
 * is set, else NULL. gpu_dump_frame_start runs at every present. */
const GpuBackend *gpu_dump_backend(void);

#ifdef __cplusplus
}
#endif

#endif
