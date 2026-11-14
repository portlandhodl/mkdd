/* Host port: the GX write-gather pipe. Stores to GXWGFifo.<type> append the value,
 * big-endian as on hardware, to the host FIFO that the GX backend decodes. */
#ifndef PORT_GXFIFO_H
#define PORT_GXFIFO_H

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif
void port_gx_w8(u8 v);
void port_gx_w16(u16 v);
void port_gx_w32(u32 v);
void port_gx_wf32(f32 v);
#ifdef __cplusplus
}

struct PortWG8 { void operator=(u8 v) { port_gx_w8(v); } };
struct PortWGs8 { void operator=(s8 v) { port_gx_w8((u8)v); } };
struct PortWG16 { void operator=(u16 v) { port_gx_w16(v); } };
struct PortWGs16 { void operator=(s16 v) { port_gx_w16((u16)v); } };
struct PortWG32 { void operator=(u32 v) { port_gx_w32(v); } };
struct PortWGs32 { void operator=(s32 v) { port_gx_w32((u32)v); } };
struct PortWGf32 { void operator=(f32 v) { port_gx_wf32(v); } };
struct PortWGPipe {
    PortWG8 u8;
    PortWGs8 s8;
    PortWG16 u16;
    PortWGs16 s16;
    PortWG32 u32;
    PortWGs32 s32;
    PortWGf32 f32;
};
extern PortWGPipe port_gx_wgpipe;
#define GXWGFifo port_gx_wgpipe
#else
/* C (the SDK layer itself) never draws through the pipe; stores land in a dummy. */
extern char port_gx_wgpipe[8];
#define GXWGFifo (*(volatile PPCWGPipe *)port_gx_wgpipe)
#endif

#endif
