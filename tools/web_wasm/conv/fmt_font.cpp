// BFN fonts (JUTResFont). Header: char[8] magic, u32 size, u32 numBlocks, 16 pad.
// Blocks: u32 magic, u32 size, then per type (see JUTFont.h ResFONT).
#include "conv.h"

namespace conv {

void convBFN(Swapper &s)
{
    if (!s.check(0, 0x20, "BFN header"))
        return;
    s.w32(0, 4);
    s.raw(0x10, 0x10);
    u32 n = s.r32(0xC), off = 0x20;
    for (u32 i = 0; i < n && s.ok(); i++) {
        if (!s.check(off, 8, "BFN block"))
            return;
        u32 magic = s.g32(off), size = s.g32(off + 4);
        if (size < 8 || !s.check(off, size, "BFN block body"))
            return;
        u32 b = off + 8, end = off + size;
        switch (magic) {
        case 0x494E4631: // INF1
            s.w16(b, 6);
            s.raw(b + 12, end - (b + 12));
            break;
        case 0x57494431: // WID1: u16 start, u16 end, u8 width pairs
            s.w16(b, 2);
            s.raw(b + 4, end - (b + 4));
            break;
        case 0x474C5931: // GLY1: header then texture data (GPU, stays BE)
            s.w16(b, 4);
            s.w32(b + 8);
            s.w16(b + 12, 6);
            s.raw(b + 24, end - (b + 24));
            break;
        case 0x4D415031: { // MAP1: u16 method, start, end, count, then u16 table
            u16 method = s.r16(b), count = s.r16(b + 6);
            s.w16(b, 4);
            u32 entries = method == 3 ? count * 2u : method == 0 ? 0 : count;
            if (entries && s.check(b + 8, entries * 2, "MAP1 table"))
                s.w16(b + 8, entries);
            s.raw(b + 8 + entries * 2, end - (b + 8 + entries * 2));
            break;
        }
        default:
            s.fail("unknown BFN block 0x%08x at 0x%x", magic, off);
            return;
        }
        off = end;
    }
    if (off < s.size())
        s.raw(off, s.size() - off);
}

} // namespace conv
