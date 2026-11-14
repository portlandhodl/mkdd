/* AR/ARQ: the 16 MB auxiliary RAM as a host buffer. ARAM addresses are offsets
 * into it; main-RAM addresses are host pointers. Transfers copy immediately and
 * complete from port_poll(). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "port_sdk.h"

#define ARAM_SIZE 0x01000000u
#define ARAM_BASE 0x4000u /* first 16 KB are reserved for the DSP on hardware */

static u8 *sAram;
static u32 sAllocNext;

u8 *port_aram(void) { return sAram; }

u32 ARInit(u32 *stack_index_addr, u32 num_entries)
{
    (void)stack_index_addr;
    (void)num_entries;
    if (!sAram) {
        sAram = (u8 *)calloc(1, ARAM_SIZE);
        sAllocNext = ARAM_BASE;
    }
    return ARAM_BASE;
}

u32 ARAlloc(u32 length)
{
    u32 r = sAllocNext;
    sAllocNext += length;
    return r;
}

u32 ARGetBaseAddress(void) { return ARAM_BASE; }
u32 ARGetSize(void) { return ARAM_SIZE; }
void ARQInit(void) { ARInit(NULL, 0); }

static void arq_done(void *a, void *b)
{
    ARQRequest *req = (ARQRequest *)a;
    (void)b;
    if (req->callback)
        req->callback((u32)req);
}

void ARQPostRequest(ARQRequest *req, u32 owner, u32 type, u32 priority, u32 source, u32 dest, u32 length,
                    ARQCallback callback)
{
    ARInit(NULL, 0);
    req->next = NULL;
    req->owner = owner;
    req->type = type;
    req->priority = priority;
    req->source = source;
    req->dest = dest;
    req->length = length;
    req->callback = callback;
    if (type == ARQ_TYPE_MRAM_TO_ARAM) {
        if (dest + length > ARAM_SIZE) {
            fprintf(stderr, "[ar] write past ARAM: 0x%x+0x%x\n", dest, length);
            abort();
        }
        memcpy(sAram + dest, (void *)source, length);
    } else {
        if (source + length > ARAM_SIZE) {
            fprintf(stderr, "[ar] read past ARAM: 0x%x+0x%x\n", source, length);
            abort();
        }
        memcpy((void *)dest, sAram + source, length);
    }
    port_defer(arq_done, req, NULL);
}
