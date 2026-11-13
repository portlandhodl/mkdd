// JAudio2 resources. Spec: libs/JSystem/JAudio (JAUAudioArcInterpreter, JAUSection,
// JASBNKParser, JASWSParser, JAUSoundTable, JAUStreamFileTable, JAUSeqCollection)
// and src/Inagaki/GameAudioArcLoader.cpp ('baac').
//
//   .baa   "AA_<" command stream of u32 words (readU32_), each command pointing at a
//          section by offset from the archive start: ws (WSYS), bnk (IBNK), bst,
//          bstn, bsft, bsc (sequence collection), bms (sequence), baac (list of
//          nested scene archives), sect (four u8 operands).
//   .bsft  stream file table, .bstn sound name table (same layouts as in the .baa)
//   .bms   sequence bytecode: left big-endian. JASSeqReader reads operands through
//          typed u16/u32 loads (read16/read24/get16/get24/get32), read24 even loads
//          a u32 overlapping the preceding opcode byte, so the bytecode can't be
//          converted in place; the port's reader must read big-endian explicitly.
#include "conv.h"

namespace conv {

// Claim what no walker claimed in [off, off+len) as raw. With zero set, the
// bytes are expected to be alignment padding and anything else is reported.
static void fill(Swapper &s, u32 off, u32 len, bool zero, const char *what)
{
    if (!s.check(off, len, what))
        return;
    if (zero) {
        for (u32 i = 0; i < len; i++) {
            if (s.claimAt(off + i) == Swapper::NONE && s.data()[off + i] != 0) {
                s.warn("%s: nonzero unclaimed byte at 0x%x", what, off + i);
                break;
            }
        }
    }
    s.raw(off, len);
}

// NUL-terminated string; returns the offset past the terminator
static u32 cstr(Swapper &s, u32 off)
{
    u32 e = off;
    while (e < s.size() && s.data()[e])
        e++;
    if (e >= s.size()) {
        s.fail("unterminated string at 0x%x", off);
        return s.size();
    }
    s.raw(off, e + 1 - off);
    return e + 1;
}

// ---- bst: JAUSoundTable ----
// header: u32 'BST ', u32, u32, u32 rootOffset, 16 pad. Root: u32 numSections,
// u32 sectionOffsets[]. Section: u32 numGroups, u32 groupOffsets[]. Group: u32
// numItems, u32 unk, u32 entries[] (type << 24 | itemOffset). Items by type:
// 0x10 (se, 0x14), 0x20 (seq, 0xc), 0x30 (stream, 0x10); see JAUSoundTableItem.
static void convBST(Swapper &s, u32 b, u32 end)
{
    if (!s.check(b, 0x20, "BST header"))
        return;
    if (s.r32(b) != 0x42535420) {
        s.fail("BST: bad magic at 0x%x", b);
        return;
    }
    s.w32(b, 4);
    u32 root = b + s.r32(b + 12);
    if (!s.check(root, 4, "BST root"))
        return;
    u32 ns = s.g32(root);
    if (!s.check(root + 4, ns * 4, "BST sections"))
        return;
    s.w32(root + 4, ns);
    for (u32 i = 0; i < ns && s.ok(); i++) {
        u32 so = s.r32(root + 4 + i * 4);
        if (!so)
            continue;
        so += b;
        if (!s.check(so, 4, "BST section"))
            return;
        u32 ng = s.g32(so);
        if (!s.check(so + 4, ng * 4, "BST groups"))
            return;
        s.w32(so + 4, ng);
        for (u32 g = 0; g < ng && s.ok(); g++) {
            u32 go = s.r32(so + 4 + g * 4);
            if (!go)
                continue;
            go += b;
            if (!s.check(go, 8, "BST group"))
                return;
            u32 ni = s.g32(go);
            u32 x = s.g32(go + 4);
            if (x) // unknown 8 bytes, never read
                s.raw(b + x, 8);
            if (!s.check(go + 8, ni * 4, "BST items"))
                return;
            for (u32 k = 0; k < ni && s.ok(); k++) {
                u32 e = s.g32(go + 8 + k * 4);
                u32 it = e & 0xFFFFFF, type = e >> 24;
                if (!it)
                    continue;
                it += b;
                u32 len = type == 0x10 ? 0x14 : type == 0x20 ? 0xC : type == 0x30 ? 0x10 : 0;
                if (!len) {
                    s.fail("BST: unknown item type 0x%x at 0x%x", type, go + 8 + k * 4);
                    return;
                }
                if (!s.check(it, len, "BST item"))
                    return;
                s.raw(it, 2);     // priority, _1
                s.w16(it + 2);    // resource id
                s.w32(it + 4);    // sw bit
                s.raw(it + 8, 2); // _8, ptrack
                s.w16(it + 10);   // seq resource id
                if (len >= 0x10)
                    s.w32(it + 12); // _c (f32)
                if (len >= 0x14) {
                    s.w16(it + 16); // audible sw
                    s.raw(it + 18, 2);
                }
            }
        }
    }
    fill(s, b, end - b, true, "BST");
}

// ---- bstn: JAUSoundNameTable ----
// header as bst ('BSTN'). Root: u32 numSections, u32 sectionOffsets[]. Section:
// u32 numGroups, u32 nameOffset, u32 groupOffsets[]. Group: u32 numItems,
// u32 nameOffset, u32 itemNameOffsets[]. Names are strings.
static void convBSTN(Swapper &s, u32 b, u32 end)
{
    if (!s.check(b, 0x20, "BSTN header"))
        return;
    if (s.r32(b) != 0x4253544E) {
        s.fail("BSTN: bad magic at 0x%x", b);
        return;
    }
    s.w32(b, 4);
    u32 root = b + s.r32(b + 12);
    if (!s.check(root, 4, "BSTN root"))
        return;
    u32 ns = s.g32(root);
    if (!s.check(root + 4, ns * 4, "BSTN sections"))
        return;
    s.w32(root + 4, ns);
    for (u32 i = 0; i < ns && s.ok(); i++) {
        u32 so = s.r32(root + 4 + i * 4);
        if (!so)
            continue;
        so += b;
        if (!s.check(so, 8, "BSTN section"))
            return;
        u32 ng = s.g32(so);
        u32 name = s.g32(so + 4);
        if (name)
            cstr(s, b + name);
        if (!s.check(so + 8, ng * 4, "BSTN groups"))
            return;
        s.w32(so + 8, ng);
        for (u32 g = 0; g < ng && s.ok(); g++) {
            u32 go = s.r32(so + 8 + g * 4);
            if (!go)
                continue;
            go += b;
            if (!s.check(go, 8, "BSTN group"))
                return;
            u32 ni = s.g32(go);
            name = s.g32(go + 4);
            if (name)
                cstr(s, b + name);
            if (!s.check(go + 8, ni * 4, "BSTN items"))
                return;
            for (u32 k = 0; k < ni && s.ok(); k++) {
                u32 n = s.g32(go + 8 + k * 4);
                if (n)
                    cstr(s, b + n);
            }
        }
    }
    fill(s, b, end - b, true, "BSTN");
}

// ---- bsft: JAUStreamFileTable ----
// u8 magic[4] "bsft" (compared bytewise), u32 count, u32 nameOffsets[count], strings
static void convBSFT(Swapper &s, u32 b)
{
    if (!s.check(b, 8, "BSFT header"))
        return;
    if (s.r8(b) != 'b' || s.r8(b + 1) != 's' || s.r8(b + 2) != 'f' || s.r8(b + 3) != 't') {
        s.fail("BSFT: bad magic at 0x%x", b);
        return;
    }
    s.raw(b, 4);
    u32 n = s.g32(b + 4);
    if (!s.check(b + 8, n * 4, "BSFT offsets"))
        return;
    s.w32(b + 8, n);
    for (u32 i = 0; i < n && s.ok(); i++)
        cstr(s, b + s.r32(b + 8 + i * 4));
}

// ---- bsc: JAUSeqCollection ----
// s8 'S', s8 'C' (compared bytewise), u16 numGroups, u32 size, u32 groupOffsets[];
// group: u32 count, u32 seqOffsets[]. Everything else is sequence bytecode (raw).
static void convBSC(Swapper &s, u32 b, u32 end)
{
    if (!s.check(b, 8, "BSC header"))
        return;
    if (s.r8(b) != 'S' || s.r8(b + 1) != 'C') {
        s.fail("BSC: bad magic at 0x%x", b);
        return;
    }
    s.raw(b, 2);
    u32 ng = s.g16(b + 2);
    s.w32(b + 4);
    if (!s.check(b + 8, ng * 4, "BSC groups"))
        return;
    s.w32(b + 8, ng);
    for (u32 g = 0; g < ng && s.ok(); g++) {
        u32 go = b + s.r32(b + 8 + g * 4);
        if (!s.check(go, 4, "BSC group"))
            return;
        u32 n = s.g32(go);
        if (!s.check(go + 4, n * 4, "BSC group offsets"))
            return;
        s.w32(go + 4, n);
    }
    fill(s, b, end - b, false, "BSC"); // bytecode
}

// ---- ws: JASWSParser ----
// WSYS header: u32 magic, size, id, waveTableSize, winfOffset, wbctOffset, 8 pad.
// WINF: u32 magic, count, archiveOffsets[]. Archive: char name[0x70], u32 count,
// u32 waveOffsets[]. Wave (0x2c): u8 x4, f32, u32 x6, s16 x2, u32 x2.
// WBCT: u32 magic, unk, count, sceneOffsets[]. SCNE: u32 magic, u32, u32,
// u32 cdf, u32 cex, u32 cst. C-DF/C-EX/C-ST: u32 magic, count, entryOffsets[];
// C-DF entries (0x38): u32 id (awID << 16 | waveID), u32 x13.
static void convCtrl(Swapper &s, u32 b, u32 off)
{
    if (!s.check(off, 8, "WSYS ctrl"))
        return;
    u32 n = s.r32(off + 4);
    s.w32(off, 2);
    if (!s.check(off + 8, n * 4, "WSYS ctrl entries"))
        return;
    s.w32(off + 8, n);
    for (u32 i = 0; i < n && s.ok(); i++) {
        u32 e = s.r32(off + 8 + i * 4);
        if (e && s.check(b + e, 0x38, "WSYS ctrl wave"))
            s.w32(b + e, 14);
    }
}

static void convWS(Swapper &s, u32 b)
{
    if (!s.check(b, 0x20, "WSYS header"))
        return;
    if (s.r32(b) != 0x57535953) {
        s.fail("WSYS: bad magic at 0x%x", b);
        return;
    }
    u32 size = s.r32(b + 4);
    s.w32(b, 6);
    if (!s.check(b, size, "WSYS"))
        return;

    u32 winf = s.r32(b + 0x10);
    if (winf) {
        winf += b;
        if (!s.check(winf, 8, "WINF"))
            return;
        u32 na = s.r32(winf + 4);
        s.w32(winf, 2);
        if (!s.check(winf + 8, na * 4, "WINF offsets"))
            return;
        s.w32(winf + 8, na);
        for (u32 a = 0; a < na && s.ok(); a++) {
            u32 ao = s.r32(winf + 8 + a * 4);
            if (!ao)
                continue;
            ao += b;
            if (!s.check(ao, 0x74, "WSYS archive"))
                return;
            s.raw(ao, 0x70);
            u32 nw = s.g32(ao + 0x70);
            if (!s.check(ao + 0x74, nw * 4, "WSYS wave offsets"))
                return;
            s.w32(ao + 0x74, nw);
            for (u32 w = 0; w < nw && s.ok(); w++) {
                u32 wo = s.r32(ao + 0x74 + w * 4);
                if (!wo)
                    continue;
                wo += b;
                if (!s.check(wo, 0x2C, "WSYS wave"))
                    return;
                s.raw(wo, 4);
                s.w32(wo + 4, 7);
                s.w16(wo + 0x20, 2);
                s.w32(wo + 0x24, 2);
            }
        }
    }

    u32 wbct = s.r32(b + 0x14);
    if (wbct) {
        wbct += b;
        if (!s.check(wbct, 12, "WBCT"))
            return;
        u32 ng = s.r32(wbct + 8);
        s.w32(wbct, 3);
        if (!s.check(wbct + 12, ng * 4, "WBCT offsets"))
            return;
        s.w32(wbct + 12, ng);
        for (u32 g = 0; g < ng && s.ok(); g++) {
            u32 so = s.r32(wbct + 12 + g * 4);
            if (!so)
                continue;
            so += b;
            if (!s.check(so, 0x18, "SCNE"))
                return;
            s.w32(so, 6);
            for (u32 k = 0; k < 3; k++) {
                u32 c = s.r32(so + 12 + k * 4);
                if (c)
                    convCtrl(s, b, b + c);
            }
        }
    }
    fill(s, b, size, true, "WSYS");
}

// ---- bnk: JASBNKParser ----
static void convOsc(Swapper &s, u32 b, u32 off)
{
    if (!s.check(off, 0x18, "BNK osc"))
        return;
    s.raw(off, 4); // u8 target, pad
    s.w32(off + 4, 5);
    for (u32 k = 0; k < 2; k++) {
        u32 p = s.r32(off + 8 + k * 4);
        if (!p)
            continue;
        p += b;
        // JASBNKParser::getOscTableEndPtr: points of 3 x s16 up to and including
        // the first with mode > 10
        for (;;) {
            if (!s.check(p, 6, "BNK osc table"))
                return;
            s16 mode = s.rs16(p);
            s.w16(p, 3);
            p += 6;
            if (mode > 10)
                break;
        }
    }
}

static void convRand(Swapper &s, u32 off)
{
    if (!s.check(off, 0xC, "BNK rand"))
        return;
    s.raw(off, 4);
    s.w32(off + 4, 2);
}

static void convSense(Swapper &s, u32 off)
{
    if (!s.check(off, 0xC, "BNK sense"))
        return;
    s.raw(off, 4);
    s.w32(off + 4, 2);
}

static void convVmap(Swapper &s, u32 off)
{
    if (!s.check(off, 0x10, "BNK vmap"))
        return;
    s.raw(off, 4);
    s.w32(off + 4, 3);
}

static void convBNK(Swapper &s, u32 b)
{
    if (!s.check(b, 0x3E4, "IBNK header"))
        return;
    if (s.r32(b) != 0x49424E4B) {
        s.fail("IBNK: bad magic at 0x%x", b);
        return;
    }
    u32 size = s.r32(b + 4);
    s.w32(b, 3);
    if (!s.check(b, size, "IBNK"))
        return;
    s.w32(b + 0x20, 1 + 0x80); // BANK magic, inst offsets
    s.w32(b + 0x3B4, 12);      // perc offsets
    for (u32 i = 0; i < 0x80 && s.ok(); i++) {
        u32 io = s.r32(b + 0x24 + i * 4);
        if (!io)
            continue;
        io += b;
        if (!s.check(io, 0x2C, "BNK inst"))
            return;
        s.w32(io);
        s.raw(io + 4, 4);
        s.w32(io + 8, 9);
        u32 nk = s.r32(io + 0x28);
        if (!s.check(io + 0x2C, nk * 4, "BNK keymap offsets"))
            return;
        s.w32(io + 0x2C, nk);
        for (u32 k = 0; k < 2; k++) {
            u32 o = s.r32(io + 0x10 + k * 4);
            if (o)
                convOsc(s, b, b + o);
            o = s.r32(io + 0x18 + k * 4);
            if (o)
                convRand(s, b + o);
            o = s.r32(io + 0x20 + k * 4);
            if (o)
                convSense(s, b + o);
        }
        for (u32 k = 0; k < nk && s.ok(); k++) {
            u32 ko = s.r32(io + 0x2C + k * 4);
            if (!ko)
                continue;
            ko += b;
            if (!s.check(ko, 8, "BNK keymap"))
                return;
            s.raw(ko, 4); // u8 high, pad
            u32 nv = s.g32(ko + 4);
            if (!s.check(ko + 8, nv * 4, "BNK vmap offsets"))
                return;
            s.w32(ko + 8, nv);
            for (u32 v = 0; v < nv; v++) {
                u32 vo = s.r32(ko + 8 + v * 4);
                if (vo)
                    convVmap(s, b + vo);
            }
        }
    }
    for (u32 i = 0; i < 12 && s.ok(); i++) {
        u32 po = s.r32(b + 0x3B4 + i * 4);
        if (!po)
            continue;
        po += b;
        if (!s.check(po, 0x288, "BNK perc"))
            return;
        u32 magic = s.g32(po);
        s.raw(po + 4, 0x84);
        s.w32(po + 0x88, 0x80);
        if (magic == 0x50455232) { // 'PER2': s8 pan[0x80], u16 release[0x80]
            if (!s.check(po + 0x288, 0x180, "BNK PER2 tables"))
                return;
            s.raw(po + 0x288, 0x80);
            s.w16(po + 0x308, 0x80);
        }
        for (u32 j = 0; j < 0x80 && s.ok(); j++) {
            u32 pm = s.r32(po + 0x88 + j * 4);
            if (!pm)
                continue;
            pm += b;
            if (!s.check(pm, 0x14, "BNK pmap"))
                return;
            s.w32(pm, 5); // volume, pitch, rand offsets, velo region count
            u32 nv = s.r32(pm + 0x10);
            for (u32 k = 0; k < 2; k++) {
                u32 o = s.r32(pm + 8 + k * 4);
                if (o)
                    convRand(s, b + o);
            }
            if (!s.check(pm + 0x14, nv * 4, "BNK pmap vmap offsets"))
                return;
            s.w32(pm + 0x14, nv);
            for (u32 v = 0; v < nv; v++) {
                u32 vo = s.r32(pm + 0x14 + v * 4);
                if (vo)
                    convVmap(s, b + vo);
            }
        }
    }
    fill(s, b, size, true, "IBNK");
}

// ---- baa: JAUAudioArcInterpreter ----
static void convBAA(Swapper &s, u32 b, int depth)
{
    if (!s.check(b, 4, "AA header") || s.r32(b) != 0x41415F3C) { // 'AA_<'
        s.fail("baa: bad magic at 0x%x", b);
        return;
    }
    s.w32(b);
    u32 p = b + 4;
    for (;;) {
        if (!s.ok() || !s.check(p, 4, "AA command"))
            return;
        u32 cmd = s.g32(p);
        p += 4;
        u32 nargs;
        switch (cmd) {
        case 0x3E5F4141: return; // '>_AA'
        case 0x3E5F626C: nargs = 0; break; // '>_bl'
        case 0x77732020: nargs = 3; break; // 'ws  '
        case 0x626D7320: nargs = 3; break; // 'bms '
        case 0x626E6B20: // 'bnk '
        case 0x626C5F3C: // 'bl_<'
        case 0x62736320: // 'bsc '
        case 0x62737420: // 'bst '
        case 0x6273746E: // 'bstn'
        case 0x62616163: nargs = 2; break; // 'baac'
        case 0x626D7361: // 'bmsa'
        case 0x64737162: // 'dsqb'
        case 0x62736674: nargs = 1; break; // 'bsft'
        case 0x73656374: // 'sect': four u8
            if (s.check(p, 4, "sect"))
                s.raw(p, 4);
            p += 4;
            continue;
        default:
            s.fail("baa: unknown command 0x%08x at 0x%x", cmd, p - 4);
            return;
        }
        if (!s.check(p, nargs * 4, "AA command args"))
            return;
        u32 a[3] = {0, 0, 0};
        for (u32 i = 0; i < nargs; i++)
            a[i] = s.g32(p + i * 4);
        p += nargs * 4;
        switch (cmd) {
        case 0x77732020: convWS(s, b + a[1]); break;
        case 0x626E6B20: convBNK(s, b + a[1]); break;
        case 0x62736320:
            if (a[1] >= a[0])
                convBSC(s, b + a[0], b + a[1]);
            break;
        case 0x62737420:
            if (a[1] >= a[0])
                convBST(s, b + a[0], b + a[1]);
            break;
        case 0x6273746E:
            if (a[1] >= a[0])
                convBSTN(s, b + a[0], b + a[1]);
            break;
        case 0x62736674: convBSFT(s, b + a[0]); break;
        case 0x626D7320: // sequence bytecode, read big-endian
            if (a[2] >= a[1] && s.check(b + a[1], a[2] - a[1], "bms"))
                s.raw(b + a[1], a[2] - a[1]);
            break;
        case 0x62616163: { // 'baac': u32 count, u32 offsets[] to nested archives
            u32 c = b + a[0];
            if (depth > 0 || !s.check(c, 4, "baac")) {
                s.fail("baa: bad baac at 0x%x", c);
                return;
            }
            u32 n = s.g32(c);
            if (!s.check(c + 4, n * 4, "baac offsets"))
                return;
            s.w32(c + 4, n);
            for (u32 i = 0; i < n && s.ok(); i++)
                convBAA(s, c + s.r32(c + 4 + i * 4), depth + 1);
            break;
        }
        default: break;
        }
    }
}

static bool hasExt(const Str &name, const char *ext)
{
    size_t dot = name.rfind('.');
    if (dot == Str::npos)
        return false;
    Str e = name.substr(dot + 1);
    for (size_t i = 0; i < e.size(); i++)
        if (e[i] >= 'A' && e[i] <= 'Z')
            e[i] = (char)(e[i] - 'A' + 'a');
    return e == ext;
}

void convAudio(Swapper &s)
{
    if (hasExt(s.name(), "bms")) {
        s.raw(0, s.size());
        return;
    }
    if (hasExt(s.name(), "baa"))
        convBAA(s, 0, 0);
    else if (hasExt(s.name(), "bsft"))
        convBSFT(s, 0);
    else if (hasExt(s.name(), "bstn"))
        convBSTN(s, 0, s.size());
    else {
        s.fail("convAudio: unknown audio file type");
        return;
    }
    if (s.ok())
        fill(s, 0, s.size(), true, "file");
}

} // namespace conv
