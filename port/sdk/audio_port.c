/* Host-side helpers for JAudio data that stays big-endian (sequence bytecode). */
#include <stdlib.h>
#include <string.h>
#include "port_sdk.h"

/* JASOscillator::Point {s16 mode, s16 time, s16 value}; a table ends with the
 * first point whose mode is > 10. Copies are cached per source address. */
#define MAX_TABLES 128
#define MAX_POINTS 64

static struct {
    const void *src;
    s16 pts[MAX_POINTS * 3];
} sTables[MAX_TABLES];
static int sNumTables;

const void *port_osc_table(const void *be)
{
    const u8 *p = (const u8 *)be;
    int i, n;
    if (!be)
        return NULL;
    for (i = 0; i < sNumTables; i++)
        if (sTables[i].src == be)
            return sTables[i].pts;
    if (sNumTables == MAX_TABLES)
        sNumTables = 0; /* recycle oldest; tables are tiny and rarely change */
    i = sNumTables++;
    sTables[i].src = be;
    for (n = 0; n < MAX_POINTS; n++) {
        int k;
        for (k = 0; k < 3; k++)
            sTables[i].pts[n * 3 + k] = (s16)(p[(n * 3 + k) * 2] << 8 | p[(n * 3 + k) * 2 + 1]);
        if (sTables[i].pts[n * 3] > 10)
            break;
    }
    return sTables[i].pts;
}
