/* Data the original links into main.dol as binary blobs (config.yml "extract").
 * The port copies them out of the user's own main.dol at startup. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "port_sdk.h"

__attribute__((aligned(32))) u8 _pSystemArc[0x4A260];
__attribute__((aligned(32))) u8 JUTResFONT_Ascfont_fix12[0x4160];
char _pBuildtimeDat[0x1B];

static const struct {
    void *dst;
    u32 addr, size;
} sBlobs[] = {
    {_pSystemArc, 0x80316680, sizeof _pSystemArc},
    {JUTResFONT_Ascfont_fix12, 0x80362160, sizeof JUTResFONT_Ascfont_fix12},
    {_pBuildtimeDat, 0x803608E0, sizeof _pBuildtimeDat},
};

unsigned char *port_conv_file(const char *name, const unsigned char *in, unsigned inSize, unsigned *outSize);

/* convert in place; the blobs have fixed-size homes, so the layout must not change */
static void convert_blob(void *data, u32 size, const char *name)
{
    unsigned outSize;
    unsigned char *out = port_conv_file(name, (const unsigned char *)data, size, &outSize);
    if (outSize != size) {
        fprintf(stderr, "[dol] %s changed size on conversion (%u -> %u)\n", name, size, outSize);
        abort();
    }
    memcpy(data, out, size);
    free(out);
}

static u32 be32(const u8 *p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }

u8 *port_read_main_dol(u32 *outSize);

/* Called from OSInit, before the game touches any of these. */
void port_load_dol_data(void)
{
    u32 dolSize;
    u8 *dol = port_read_main_dol(&dolSize);
    size_t b;
    if (!dol || dolSize < 0x100) {
        fprintf(stderr, "[dol] cannot read main.dol from the disc\n");
        abort();
    }
    for (b = 0; b < sizeof sBlobs / sizeof sBlobs[0]; b++) {
        int s;
        for (s = 0; s < 18; s++) {
            u32 off = be32(dol + s * 4), addr = be32(dol + 0x48 + s * 4), size = be32(dol + 0x90 + s * 4);
            if (sBlobs[b].addr >= addr && sBlobs[b].addr + sBlobs[b].size <= addr + size &&
                off + (sBlobs[b].addr - addr) + sBlobs[b].size <= dolSize) {
                memcpy(sBlobs[b].dst, dol + off + (sBlobs[b].addr - addr), sBlobs[b].size);
                break;
            }
        }
        if (s == 18) {
            fprintf(stderr, "[dol] 0x%08x not in any section\n", sBlobs[b].addr);
            abort();
        }
    }
    free(dol);
    convert_blob(_pSystemArc, sizeof _pSystemArc, "system.arc");
    convert_blob(JUTResFONT_Ascfont_fix12, sizeof JUTResFONT_Ascfont_fix12, "Ascfont_fix12.bfn");
}
