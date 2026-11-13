/* GPU front end: the write-gather pipe sink. Bytes arrive big-endian, exactly
 * as the hardware FIFO receives them. (Placeholder: the command processor that
 * decodes this stream lives in port/gpu.) */
#include <string.h>
#include <dolphin/types.h>
#include "port_gxfifo.h"

/* ---- write-gather pipe: big-endian bytes, as the hardware FIFO receives them ---- */

static u8 sFifo[1 << 20];
static u32 sFifoPos;

static inline void fifo_put(u8 v)
{
    sFifo[sFifoPos++ & (sizeof sFifo - 1)] = v;
}

void port_gx_w8(u8 v) { fifo_put(v); }
void port_gx_w16(u16 v) { fifo_put((u8)(v >> 8)); fifo_put((u8)v); }
void port_gx_w32(u32 v) { port_gx_w16((u16)(v >> 16)); port_gx_w16((u16)v); }
void port_gx_wf32(f32 v)
{
    u32 u;
    memcpy(&u, &v, 4);
    port_gx_w32(u);
}

/* C++ code refers to the pipe object; it carries no state. */
char port_gx_wgpipe[8];

