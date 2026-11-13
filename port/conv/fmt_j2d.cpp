// J2D resources. Specs are the decomp's loaders:
//  - .blo: J2DScreen::private_set (J2DScreen.cpp). Blocks are read through
//    JSURandomInputStream into typed locals/structs, so every u16/u32/f32 the
//    stream reads must be host order. v1 panes (PAN1/PIC1/WIN1/TBX1) are read
//    field by field (J2DPane::makePaneStream, J2D{Picture,Window,TextBox}::
//    private_readStream); v2 panes (PAN2/PIC2/WIN2/TBX2) as structs
//    (J2DPaneExBlock, J2DScrnBlockPictureParameter, J2DWindowData,
//    J2DTextBoxBlock). MAT1 is copied whole and overlaid with
//    J2DMaterialBlock / J2DMaterialFactory tables.
//  - .bti: ResTIMG header (JUTTexture); pixel and palette data stay BE (GPU).
#include "conv.h"

namespace conv {

// u64 fields (pane tag / user info) are compared as 64-bit integers
// (J2DPane::search(u64)). The Swapper has no 64-bit claim, so swap each half
// and exchange the halves; both steps are self-inverse and commute, so the
// reverse direction is the same operation.
static void w64(Swapper &s, u32 off)
{
    if (!s.check(off, 8, "u64"))
        return;
    bool fresh = s.claimAt(off) == Swapper::NONE && s.claimAt(off + 4) == Swapper::NONE;
    s.w32(off, 2);
    if (fresh) {
        u8 *p = s.data() + off;
        for (int i = 0; i < 4; i++) {
            u8 t = p[i];
            p[i] = p[i + 4];
            p[i + 4] = t;
        }
    }
}

namespace {

// Sequential reader over one block, mirroring the stream reads of the loader.
struct Cur {
    Swapper &s;
    u32 p, end;
    bool ok = true;
    Cur(Swapper &s, u32 p, u32 end) : s(s), p(p), end(end) {}
    bool need(u32 n)
    {
        if (!ok || p + n > end) {
            if (ok)
                s.fail("BLO pane field out of block at 0x%x (+%u > 0x%x)", p, n, end);
            ok = false;
            return false;
        }
        return true;
    }
    u8 u8_()
    {
        if (!need(1))
            return 0;
        s.raw(p, 1);
        return s.r8(p++);
    }
    u16 u16_()
    {
        if (!need(2))
            return 0;
        u16 v = s.g16(p);
        p += 2;
        return v;
    }
    u32 u32_()
    {
        if (!need(4))
            return 0;
        u32 v = s.g32(p);
        p += 4;
        return v;
    }
    void bytes(u32 n)
    {
        if (n && need(n)) {
            s.raw(p, n);
            p += n;
        }
    }
    // JUTResReference::getResource(stream) / J2DDataManage::get(stream):
    // u8 type, u8 length, char name[length]
    void resRef()
    {
        u8 t = u8_();
        (void)t;
        bytes(u8_());
    }
    // claim the rest of the block (skipped by the loader's final seek)
    void finish()
    {
        if (ok && p < end)
            s.raw(p, end - p);
    }
};

} // namespace

// J2DPane::makePaneStream (PAN1 body, also the start of PIC1/WIN1/TBX1)
static void bloPane1(Cur &c)
{
    u8 n = c.u8_(); // values remaining
    c.u8_();        // visible
    c.bytes(2);     // skip(2)
    c.u32_();       // u32 tag
    c.u16_();       // s16 x, y, w, h
    c.u16_();
    c.u16_();
    c.u16_();
    n -= 6;
    if (n) { c.u16_(); n--; } // angle (readU16)
    if (n) { c.u8_(); n--; }  // base position
    if (n) { c.u8_(); n--; }  // alpha
    if (n) { c.u8_(); n--; }  // inherit alpha
    // input->align(4): stream positions are file offsets
    u32 a = (c.p + 3) & ~3u;
    if (a > c.p && a <= c.end)
        c.bytes(a - c.p);
}

// J2DPane::makePaneExStream: J2DPaneExBlock (0x44) at the start of a PAN2 block
static void bloPaneEx(Swapper &s, u32 b, u32 end)
{
    if (!s.check(b, 0x44, "PAN2") || b + 0x44 > end) {
        s.fail("PAN2 block too small at 0x%x", b);
        return;
    }
    s.w32(b, 2);         // block header
    s.w16(b + 0x08, 2);  // _08, anim index
    s.raw(b + 0x0C, 2);  // visible, base position
    s.w16(b + 0x0E);     // _0E
    w64(s, b + 0x10);    // tag
    w64(s, b + 0x18);    // user info / message id
    s.w32(b + 0x20, 9);  // width, height, scale x/y, angle x/y/z, offset x/y
    s.raw(b + 0x44, end - (b + 0x44));
}

// v2 panes: block header, PAN2 sub-block, then a type-specific trailer
static u32 bloPaneExSub(Swapper &s, u32 b, u32 end)
{
    u32 sub = b + 8;
    if (!s.check(sub, 8, "PAN2 sub-block"))
        return end;
    u32 len = s.r32(sub + 4);
    if (len < 0x44 || sub + len > end) { // type is 'pan2' (unchecked by the loader)
        s.fail("bad PAN2 sub-block at 0x%x (len 0x%x)", sub, len);
        return end;
    }
    bloPaneEx(s, sub, sub + len);
    return sub + len;
}

static void bloTEX1(Swapper &s, u32 b, u32 end)
{
    // J2DScreen::getResReference: skip 4, s32 length, skip 4, s32 data offset
    s.w16(b + 8);  // count (skipped by the loader)
    s.raw(b + 10, 2);
    u32 off = s.g32(b + 12);
    u32 r = b + off;
    if (off < 16 || r + 2 > end) {
        s.fail("bad TEX1/FNT1 reference offset 0x%x at 0x%x", off, b);
        return;
    }
    s.raw(b + 16, off - 16);
    // J2DResReference: u16 count, u16 offsets[count], then u8 type/len/name entries
    u16 n = s.g16(r);
    if (!s.check(r + 2, n * 2u, "TEX1 offsets") || r + 2 + n * 2u > end) {
        s.fail("TEX1/FNT1 offsets out of block at 0x%x", b);
        return;
    }
    s.w16(r + 2, n);
    u32 p = r + 2 + n * 2u;
    s.raw(p, end - p);
}

static void bloMAT1(Swapper &s, u32 b, u32 end)
{
    u32 len = end - b;
    if (len < 0x68) {
        s.fail("MAT1 block too small at 0x%x", b);
        return;
    }
    u16 nmat = s.g16(b + 8);
    s.raw(b + 10, 2);
    s.w32(b + 0x0C, 23);
    u32 o[0x68 / 4];
    for (u32 i = 3; i < 0x68 / 4; i++) {
        o[i] = s.r32(b + i * 4);
        if (o[i] && (o[i] < 0x68 || o[i] >= len)) {
            s.fail("MAT1 table offset 0x%x out of block at 0x%x", o[i], b);
            return;
        }
    }
    // a table runs to the next table (or the end of the block)
    auto extent = [&](u32 off) {
        u32 e = len;
        for (u32 i = 3; i < 0x68 / 4; i++)
            if (o[i] > off && o[i] < e)
                e = o[i];
        return e - off;
    };
    // claim count elements of a table and leave the rest of its extent raw
    auto table = [&](u32 field, u32 count, u32 stride, void (*fn)(Swapper &, u32)) -> bool {
        u32 off = o[field / 4];
        if (!off)
            return count == 0 || (s.fail("MAT1 table 0x%x missing but referenced", field), false);
        u32 ext = extent(off), need = count * stride;
        if (need > ext) {
            s.fail("MAT1 table 0x%x: %u x %u bytes exceeds extent 0x%x", field, count, stride, ext);
            return false;
        }
        for (u32 i = 0; i < count; i++)
            fn(s, b + off + i * stride);
        s.raw(b + off + need, ext - need);
        return true;
    };

    // material index table (u16) -> number of init data entries
    u32 idxOff = o[0x10 / 4];
    if (!idxOff || !s.check(b + idxOff, nmat * 2u, "MAT1 index table")) {
        s.fail("MAT1 index table missing at 0x%x", b);
        return;
    }
    u32 ninit = 0;
    for (u32 i = 0; i < nmat; i++) {
        u16 v = s.r16(b + idxOff + i * 2);
        if (v + 1u > ninit)
            ninit = v + 1u;
    }
    table(0x10, nmat, 2, [](Swapper &s, u32 p) { s.w16(p); });

    // J2DMaterialInitData (0xE8): u8 x8, u16 x37 @08, u8 x32 @52, u16 x59 @72
    u32 initOff = o[0x0C / 4];
    if (!initOff || !s.check(b + initOff, ninit * 0xE8u, "MAT1 init data")) {
        s.fail("MAT1 init data missing at 0x%x", b);
        return;
    }
    table(0x0C, ninit, 0xE8, [](Swapper &s, u32 p) {
        s.raw(p, 8);
        s.w16(p + 0x08, 37);
        s.raw(p + 0x52, 0x20);
        s.w16(p + 0x72, 59);
    });

    // highest referenced index per table, from every init data entry
    u32 nCull = 0, nTexMtx = 0, nTexNo = 0, nFontNo = 0, nTevColor = 0;
    auto upd8 = [](u32 &n, u8 v) { if (v != 0xFF && v + 1u > n) n = v + 1u; };
    auto upd16 = [](u32 &n, u16 v) { if (v != 0xFFFF && v + 1u > n) n = v + 1u; };
    for (u32 i = 0; i < ninit; i++) {
        u32 p = b + initOff + i * 0xE8;
        upd8(nCull, s.r8(p + 0x01));
        for (u32 k = 0; k < 10; k++)
            upd16(nTexMtx, s.r16(p + 0x24 + k * 2));
        for (u32 k = 0; k < 8; k++)
            upd16(nTexNo, s.r16(p + 0x38 + k * 2));
        upd16(nFontNo, s.r16(p + 0x48));
        for (u32 k = 0; k < 4; k++)
            upd16(nTevColor, s.r16(p + 0x92 + k * 2));
    }

    // name table (ResNTAB, JUTNameTab): u16 count, pad, {u16 key, u16 offs}[], strings
    u32 ntOff = o[0x14 / 4];
    if (ntOff) {
        u32 nt = b + ntOff, ext = extent(ntOff);
        u16 n = s.r16(nt);
        if (4 + n * 4u > ext) {
            s.fail("MAT1 name table overflows at 0x%x", nt);
            return;
        }
        s.w16(nt, 2 + n * 2u);
        s.raw(nt + 4 + n * 4u, ext - (4 + n * 4u));
    }

    // J2DIndInitData, indexed by material index (not through the index
    // table). Only Mtx23 floats (3 matrices at +0x0C, stride 0x1C) are
    // multi-byte. Used only if mIndInitDataOffset - _14 > 4 (factory ctor).
    u32 indOff = o[0x18 / 4];
    if (indOff) {
        u32 ext = extent(indOff);
        if (indOff - ntOff > 4 && nmat) {
            u32 stride = ext / nmat;
            // disc data uses 0x128 (TP layout); the decomp's J2DIndInitData
            // declares 0x13C
            if (stride != 0x128 && stride != 0x13C)
                stride = 0x128;
            if (stride * nmat > ext) {
                s.fail("MAT1 ind data 0x%x x %u exceeds extent 0x%x", stride, nmat, ext);
                return;
            }
            for (u32 i = 0; i < nmat; i++) {
                u32 p = b + indOff + i * stride;
                for (u32 m = 0; m < 3; m++)
                    s.w32(p + 0x0C + m * 0x1C, 6);
            }
            s.raw(b + indOff, ext);
        } else {
            s.raw(b + indOff, ext);
        }
    }

    // GXCullMode (enum, 4 bytes)
    table(0x1C, nCull, 4, [](Swapper &s, u32 p) { s.w32(p); });
    // J2DTexMtxInfo (0x24): u8, u8, u16 pad, f32 center[3], f32 srt[5]
    table(0x34, nTexMtx, 0x24, [](Swapper &s, u32 p) {
        s.raw(p, 2);
        s.w16(p + 2);
        s.w32(p + 4, 8);
    });
    table(0x38, nTexNo, 2, [](Swapper &s, u32 p) { s.w16(p); });
    table(0x3C, nFontNo, 2, [](Swapper &s, u32 p) { s.w16(p); });
    // GXColorS10
    table(0x44, nTevColor, 8, [](Swapper &s, u32 p) { s.w16(p, 4); });

    // byte-only tables: GXColor (mat/k colors), u8 counts, u8-only info structs
    static const u32 rawFields[] = {0x20, 0x24, 0x28, 0x2C, 0x30, 0x40, 0x48,
                                    0x4C, 0x50, 0x54, 0x58, 0x5C, 0x60, 0x64};
    for (u32 f : rawFields)
        if (o[f / 4])
            s.raw(b + o[f / 4], extent(o[f / 4]));
    // anything between the header and the first table
    u32 first = len;
    for (u32 i = 3; i < 0x68 / 4; i++)
        if (o[i] && o[i] < first)
            first = o[i];
    if (first > 0x68)
        s.raw(b + 0x68, first - 0x68);
}

static void bloPIC1(Swapper &s, u32 b, u32 end)
{
    Cur c(s, b + 8, end);
    bloPane1(c);
    u8 n = c.u8_();
    c.resRef(); // TIMG
    c.resRef(); // TLUT
    c.u8_();    // binding
    n -= 3;
    if (n) { c.u8_(); n--; } // mirror
    if (n) { c.u8_(); n--; }
    if (n) { c.u32_(); n--; } // black
    if (n) { c.u32_(); n--; } // white
    for (int i = 0; n && i < 4; i++, n--)
        c.u32_(); // corner colors
    c.finish();
}

static void bloWIN1(Swapper &s, u32 b, u32 end)
{
    Cur c(s, b + 8, end);
    bloPane1(c);
    u8 n = c.u8_();
    for (int i = 0; i < 4; i++)
        c.u16_(); // window area (read16b)
    for (int i = 0; i < 4; i++)
        c.resRef(); // frame TIMG A-D
    c.resRef();     // TLUT
    c.u8_();        // wrap flags
    for (int i = 0; i < 4; i++)
        c.u32_(); // contents colors
    n -= 14;
    if (n) { c.resRef(); n--; } // contents TIMG
    if (n) { c.u32_(); n--; }   // black
    if (n) { c.u32_(); n--; }   // white
    c.finish();
}

static void bloTBX1(Swapper &s, u32 b, u32 end)
{
    Cur c(s, b + 8, end);
    bloPane1(c);
    u8 n = c.u8_();
    c.resRef(); // FONT
    c.u32_();   // char color
    c.u32_();   // gradient color
    c.u8_();    // flags
    c.u16_();   // char spacing
    c.u16_();   // line spacing
    c.u16_();   // font size x
    c.u16_();   // font size y
    s16 len = (s16)c.u16_();
    if (len > 0)
        c.bytes((u32)len);
    n -= 10;
    if (n) { c.u8_(); n--; }  // connect parent
    if (n) { c.u32_(); n--; } // black
    if (n) { c.u32_(); n--; } // white
    c.finish();
}

static void bloPIC2(Swapper &s, u32 b, u32 end)
{
    u32 t = bloPaneExSub(s, b, end);
    if (!s.ok())
        return;
    // J2DScrnBlockPictureParameter (0x30)
    if (t + 0x30 > end) {
        s.fail("PIC2 trailer out of block at 0x%x", t);
        return;
    }
    s.w16(t, 3);        // _00 (unread; trailer size 0x30), material num, material id
    s.raw(t + 0x06, 2); // _06 (unread; "RE" filler chars)
    s.w16(t + 0x08, 4); // _08[4]
    s.w16(t + 0x10, 8); // s16 tex coords
    s.w32(t + 0x20, 4); // corner colors
    s.raw(t + 0x30, end - (t + 0x30));
}

static void bloWIN2(Swapper &s, u32 b, u32 end)
{
    u32 t = bloPaneExSub(s, b, end);
    if (!s.ok())
        return;
    // J2DWindowData (0x40)
    if (t + 0x40 > end) {
        s.fail("WIN2 trailer out of block at 0x%x", t);
        return;
    }
    // _00[0x10] (unread): u16 trailer size (0x40), "RESERV" filler, 0xFF pad
    s.w16(t);
    s.raw(t + 2, 0x0E);
    s.w16(t + 0x10, 4);  // content material ids
    s.raw(t + 0x18, 2);  // wrap flags, _19
    s.w16(t + 0x1A, 11); // area x/y/w/h, _22, parent id, _26, _28[4]
    s.w32(t + 0x30, 4);  // content colors
    s.raw(t + 0x40, end - (t + 0x40));
}

static void bloTBX2(Swapper &s, u32 b, u32 end)
{
    u32 t = bloPaneExSub(s, b, end);
    if (!s.ok())
        return;
    // J2DTextBoxBlock (0x20), then mMaxReadLength chars
    if (t + 0x20 > end) {
        s.fail("TBX2 trailer out of block at 0x%x", t);
        return;
    }
    s.w16(t, 7);        // _00, anim index, material, char/line spacing, font size x/y
    s.raw(t + 0x0E, 2); // h/v binding
    s.w32(t + 0x10, 2); // char color, gradient color
    s.raw(t + 0x18, 4); // connect parent, pad
    s.w16(t + 0x1C, 2); // text length, max read length
    s.raw(t + 0x20, end - (t + 0x20));
}

void convBLO(Swapper &s)
{
    if (!s.check(0, 0x20, "BLO header"))
        return;
    // J3DFileHeader: u32 'SCRN', u32 'blo1'/'blo2' (compared as u32), size, block count
    u32 magic = s.r32(0), ver = s.r32(4);
    if (magic != 0x5343524E || (ver != 0x626C6F31 && ver != 0x626C6F32)) {
        s.fail("not a SCRN blo1/blo2 file");
        return;
    }
    s.w32(0, 4);
    s.raw(0x10, 0x10);
    u32 off = 0x20;
    while (off < s.size() && s.ok()) {
        if (s.size() - off < 8) {
            s.raw(off, s.size() - off);
            break;
        }
        u32 type = s.r32(off), len = s.r32(off + 4);
        if (len < 8 || !s.check(off, len, "BLO block")) {
            s.fail("bad BLO block 0x%08x len 0x%x at 0x%x", type, len, off);
            return;
        }
        u32 end = off + len;
        if (type != 0x50414E32) // PAN2 claims its own header
            s.w32(off, 2);
        switch (type) {
        case 0x494E4631: // INF1: u16 width, height, u32 color
            if (len < 0x10) {
                s.fail("INF1 too small at 0x%x", off);
                return;
            }
            s.w16(off + 8, 2);
            s.w32(off + 12);
            s.raw(off + 16, len - 16);
            break;
        case 0x45585431: // EXT1: end of hierarchy; anything after it is never read
            s.raw(off + 8, s.size() - (off + 8));
            return;
        case 0x42474E31: // BGN1
        case 0x454E4431: // END1
            s.raw(off + 8, len - 8);
            break;
        case 0x54455831: // TEX1
        case 0x464E5431: // FNT1
            bloTEX1(s, off, end);
            break;
        case 0x4D415431: // MAT1
            bloMAT1(s, off, end);
            break;
        case 0x50414E31: { // PAN1
            Cur c(s, off + 8, end);
            bloPane1(c);
            c.finish();
            break;
        }
        case 0x50494331: bloPIC1(s, off, end); break; // PIC1
        case 0x57494E31: bloWIN1(s, off, end); break; // WIN1
        case 0x54425831: bloTBX1(s, off, end); break; // TBX1
        case 0x50414E32: bloPaneEx(s, off, end); break; // PAN2
        case 0x50494332: bloPIC2(s, off, end); break;   // PIC2
        case 0x57494E32: bloWIN2(s, off, end); break;   // WIN2
        case 0x54425832: bloTBX2(s, off, end); break;   // TBX2
        default:
            s.fail("unknown BLO block 0x%08x at 0x%x", type, off);
            return;
        }
        off = end;
    }
}

// ResTIMG (0x20 header); image and palette data stay big-endian for the GPU.
void convBTI(Swapper &s)
{
    if (!s.check(0, 0x20, "BTI header"))
        return;
    s.raw(0, 2);       // format, transparency
    s.w16(2, 2);       // width, height
    s.raw(6, 4);       // wrap s/t, palette format, lut format
    s.w16(0x0A);       // palette entry count
    s.w32(0x0C);       // palette offset
    s.raw(0x10, 10);   // mip, edge lod, bias clamp, aniso, filters, lods, image count, _19
    s.w16(0x1A);       // lod bias
    s.w32(0x1C);       // image data offset
    if (s.size() > 0x20)
        s.raw(0x20, s.size() - 0x20);
}

// BMG messages and BMC color tables (Kameda/ReadPrintMessage.h). Header:
// u32 magic, type, size, numBlocks; u8 charset; 15 pad. Message text in DAT1
// is read byte-wise (including tag operands) and stays raw.
static void convMsgHeader(Swapper &s)
{
    s.w32(0, 4);
    s.raw(0x10, 0x10);
}

void convBMG(Swapper &s)
{
    if (!s.check(0, 0x20, "BMG header"))
        return;
    convMsgHeader(s);
    u32 n = s.r32(0xC), off = 0x20;
    for (u32 i = 0; i < n && s.ok(); i++) {
        if (!s.check(off, 8, "BMG block"))
            return;
        u32 kind = s.g32(off), size = s.g32(off + 4);
        if (size < 8 || !s.check(off, size, "BMG block body"))
            return;
        u32 b = off + 8, end = off + size;
        switch (kind) {
        case 0x494E4631: { // INF1: u16 count, entrySize, group; u8 color, pad; entries
            u16 count = s.r16(b), esize = s.r16(b + 2);
            s.w16(b, 3);
            s.raw(b + 6, 2);
            u32 e = b + 8;
            if (esize < 6 || !s.check(e, (u32)count * esize, "INF1 entries"))
                return;
            for (u32 k = 0; k < count; k++, e += esize) {
                s.w32(e);      // offset into DAT1
                s.w16(e + 4);  // rate
                s.raw(e + 6, esize - 6);
            }
            s.raw(e, end - e);
            break;
        }
        case 0x4D494431: { // MID1: u16 count, u8 format, u8 info, 4 pad, u32 ids
            u16 count = s.r16(b);
            s.w16(b);
            s.raw(b + 2, 6);
            if (s.check(b + 8, (u32)count * 4, "MID1 ids"))
                s.w32(b + 8, count);
            s.raw(b + 8 + count * 4u, end - (b + 8 + count * 4u));
            break;
        }
        default: // DAT1 text, STR1 strings
            s.raw(b, end - b);
            break;
        }
        off = end;
    }
    if (off < s.size())
        s.raw(off, s.size() - off);
}

void convBMC(Swapper &s)
{
    if (!s.check(0, 0x30, "BMC header"))
        return;
    convMsgHeader(s);
    s.w32(0x20, 2); // CLT1 kind, size
    u16 n = s.r16(0x28);
    s.w16(0x28, 2);
    if (s.check(0x2C, n * 4u, "BMC colors"))
        s.w32(0x2C, n); // read as u32 0xRRGGBBAA
    s.raw(0x2C + n * 4u, s.size() - (0x2C + n * 4u));
}

} // namespace conv
