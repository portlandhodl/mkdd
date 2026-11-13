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

__attribute__((constructor)) static void load_dol_data(void)
{
    char path[1024];
    u8 hdr[0x100];
    FILE *f;
    size_t b;
    snprintf(path, sizeof path, "%s/sys/main.dol", port_disc_root());
    if (!(f = fopen(path, "rb")) || fread(hdr, 1, sizeof hdr, f) != sizeof hdr) {
        fprintf(stderr, "[dol] cannot read %s\n", path);
        abort();
    }
    for (b = 0; b < sizeof sBlobs / sizeof sBlobs[0]; b++) {
        int s;
        for (s = 0; s < 18; s++) {
            u32 off = be32(hdr + s * 4), addr = be32(hdr + 0x48 + s * 4), size = be32(hdr + 0x90 + s * 4);
            if (sBlobs[b].addr >= addr && sBlobs[b].addr + sBlobs[b].size <= addr + size) {
                fseek(f, (long)(off + sBlobs[b].addr - addr), SEEK_SET);
                fread(sBlobs[b].dst, 1, sBlobs[b].size, f);
                break;
            }
        }
        if (s == 18) {
            fprintf(stderr, "[dol] 0x%08x not in any section\n", sBlobs[b].addr);
            abort();
        }
    }
    fclose(f);
    convert_blob(_pSystemArc, sizeof _pSystemArc, "system.arc");
    convert_blob(JUTResFONT_Ascfont_fix12, sizeof JUTResFONT_Ascfont_fix12, "Ascfont_fix12.bfn");
}
