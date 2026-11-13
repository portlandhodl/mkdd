// Converter core: Swapper, Yaz0, RARC archives and dispatch.
#include "conv.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace conv {

static Str vfmt(const char *fmt, va_list ap)
{
    char buf[512];
    vsnprintf(buf, sizeof buf, fmt, ap);
    return buf;
}

void Report::error(const Str &where, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    errors.push_back(where + ": " + vfmt(fmt, ap));
    va_end(ap);
}

void Report::warn(const Str &where, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    warnings.push_back(where + ": " + vfmt(fmt, ap));
    va_end(ap);
}

// ---- Swapper ----

static const u8 START = 0x80; // marks the first byte of a swapped element

Swapper::Swapper(u8 *data, u32 size, bool srcBig, const Str &name, Report &rep)
    : mData(data), mSize(size), mSrcBig(srcBig), mOk(true), mName(name), mRep(rep), mClaim(size, NONE)
{
}

void Swapper::fail(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (mOk || mRep.errors.size() < 10000)
        mRep.errors.push_back(mName + ": " + vfmt(fmt, ap));
    va_end(ap);
    mOk = false;
}

void Swapper::warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    mRep.warnings.push_back(mName + ": " + vfmt(fmt, ap));
    va_end(ap);
}

bool Swapper::check(u32 off, u32 len, const char *what)
{
    if (inRange(off, len))
        return true;
    fail("%s out of range: 0x%x+0x%x > 0x%x", what, off, len, mSize);
    return false;
}

// order the bytes are in now: destination order if already swapped
bool Swapper::claim(u32 off, u32 len, Claim c)
{
    u32 i;
    bool fresh = true, same = true;
    for (i = 0; i < len; i++) {
        u8 m = mClaim[off + i];
        u8 want = (u8)(c | (i == 0 ? START : 0));
        if (m != NONE)
            fresh = false;
        if (m != want)
            same = false;
    }
    if (fresh) {
        for (i = 0; i < len; i++)
            mClaim[off + i] = (u8)(c | (i == 0 ? START : 0));
        return true;
    }
    if (!same)
        fail("conflicting claim at 0x%x (width %u, existing 0x%02x)", off, len, mClaim[off]);
    return false;
}

static inline bool swappedAt(const Vec<u8> &claim, u32 off)
{
    u8 m = claim[off] & ~START;
    return m == Swapper::W16 || m == Swapper::W32;
}

u8 Swapper::r8(u32 off)
{
    if (!check(off, 1, "r8"))
        return 0;
    return mData[off];
}

u16 Swapper::r16(u32 off)
{
    if (!check(off, 2, "r16"))
        return 0;
    const u8 *p = mData + off;
    bool big = swappedAt(mClaim, off) ? !mSrcBig : mSrcBig;
    return big ? (u16)(p[0] << 8 | p[1]) : (u16)(p[1] << 8 | p[0]);
}

u32 Swapper::r32(u32 off)
{
    if (!check(off, 4, "r32"))
        return 0;
    const u8 *p = mData + off;
    bool big = swappedAt(mClaim, off) ? !mSrcBig : mSrcBig;
    return big ? (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]
               : (u32)p[3] << 24 | (u32)p[2] << 16 | (u32)p[1] << 8 | p[0];
}

f32 Swapper::rf32(u32 off)
{
    u32 u = r32(off);
    f32 f;
    memcpy(&f, &u, 4);
    return f;
}

void Swapper::w16(u32 off, u32 count)
{
    if (!count || !check(off, count * 2, "s16"))
        return;
    for (u32 i = 0; i < count; i++, off += 2)
        if (claim(off, 2, W16))
            std::swap(mData[off], mData[off + 1]);
}

void Swapper::w32(u32 off, u32 count)
{
    if (!count || !check(off, count * 4, "s32"))
        return;
    for (u32 i = 0; i < count; i++, off += 4)
        if (claim(off, 4, W32)) {
            std::swap(mData[off], mData[off + 3]);
            std::swap(mData[off + 1], mData[off + 2]);
        }
}

void Swapper::raw(u32 off, u32 len)
{
    if (!len || !check(off, len, "raw"))
        return;
    for (u32 i = 0; i < len; i++)
        if (mClaim[off + i] == NONE)
            mClaim[off + i] = RAW;
}

u32 Swapper::unclaimed() const
{
    return (u32)std::count(mClaim.begin(), mClaim.end(), (u8)NONE);
}

u32 Swapper::firstUnclaimed() const
{
    for (u32 i = 0; i < mSize; i++)
        if (mClaim[i] == NONE)
            return i;
    return mSize;
}

// ---- Yaz0 ----

bool isYaz0(const u8 *d, u32 size) { return size >= 16 && !memcmp(d, "Yaz0", 4); }

Vec<u8> yaz0Decode(const u8 *src, u32 size)
{
    u32 outSize = (u32)src[4] << 24 | (u32)src[5] << 16 | (u32)src[6] << 8 | src[7];
    Vec<u8> dst(outSize);
    u32 si = 16, di = 0;
    while (di < outSize && si < size) {
        u8 flags = src[si++];
        for (int b = 0; b < 8 && di < outSize && si < size; b++) {
            if (flags & (0x80 >> b)) {
                dst[di++] = src[si++];
            } else {
                if (si + 1 >= size)
                    return dst;
                u8 b1 = src[si], b2 = src[si + 1];
                si += 2;
                u32 dist = ((b1 & 0xF) << 8 | b2) + 1;
                u32 n = b1 >> 4;
                if (n == 0)
                    n = (si < size ? src[si++] : 0) + 0x12;
                else
                    n += 2;
                if (dist > di)
                    return dst;
                for (; n && di < outSize; n--, di++)
                    dst[di] = dst[di - dist];
            }
        }
    }
    return dst;
}

// ---- dispatch ----

static Str extOf(const Str &name)
{
    size_t dot = name.rfind('.');
    Str e = dot == Str::npos ? "" : name.substr(dot + 1);
    for (auto &c : e)
        c = (char)tolower((unsigned char)c);
    return e;
}

struct FormatEntry {
    const char *ext;
    Converter fn;
};

// Registered by the fmt_*.cpp files (see formats.cpp).
extern const FormatEntry kFormats[];
extern const unsigned kNumFormats;

Converter find(const Str &name, const u8 *data, u32 size)
{
    Str e = extOf(name);
    for (unsigned i = 0; i < kNumFormats; i++)
        if (e == kFormats[i].ext)
            return kFormats[i].fn;
    (void)data;
    (void)size;
    return nullptr;
}

bool wants(const Str &name)
{
    Str e = extOf(name);
    if (e == "arc" || e == "szs" || e == "rarc")
        return true;
    for (unsigned i = 0; i < kNumFormats; i++)
        if (e == kFormats[i].ext)
            return true;
    return false;
}

static bool convertPlain(const Str &name, u8 *data, u32 size, bool srcBig, Report &rep)
{
    Converter fn = find(name, data, size);
    if (!fn)
        return false;
    Swapper s(data, size, srcBig, name, rep);
    fn(s);
    rep.files++;
    if (s.ok() && s.unclaimed())
        rep.warn(name, "%u of %u bytes unclaimed (first at 0x%x)", s.unclaimed(), size, s.firstUnclaimed());
    return true;
}

// ---- RARC ----
//
// header (0x20): 8 x u32. info block at header_length: 6 x u32, u16 nextFreeFileID,
// u8 isSyncIDs, 5 pad. Offsets in the info block are relative to the info block.
// node (0x10): u32 type, u32 name, u16 hash, u16 count, u32 first. file entry
// (0x14): u16 id, u16 hash, u32 attr<<24|name, u32 data offset, u32 size, u32 runtime ptr.
// Member data starts at header_length + file_data_offset.

struct Member {
    Str name;
    u32 entry; // offset of the file entry
    u32 dataOff;
    u32 size;
    u8 attr;
};

void convRARC(Swapper &s)
{
    if (!s.check(0, 0x20, "RARC header"))
        return;
    s.w32(0, 8);
    u32 hl = s.r32(8);
    if (!s.check(hl, 0x20, "RARC info"))
        return;
    s.w32(hl, 6);
    s.w16(hl + 0x18);
    s.raw(hl + 0x1A, 6);
    u32 nn = s.r32(hl), no = s.r32(hl + 4), nf = s.r32(hl + 8), fo = s.r32(hl + 12);
    u32 sl = s.r32(hl + 16), so = s.r32(hl + 20);
    if (!s.check(hl + no, nn * 16, "RARC nodes") || !s.check(hl + fo, nf * 20, "RARC entries") ||
        !s.check(hl + so, sl, "RARC strings"))
        return;
    for (u32 i = 0; i < nn; i++) {
        u32 n = hl + no + i * 16;
        s.w32(n, 2);
        s.w16(n + 8, 2);
        s.w32(n + 12);
    }
    for (u32 i = 0; i < nf; i++) {
        u32 e = hl + fo + i * 20;
        s.w16(e, 2);
        s.w32(e + 4, 4);
    }
    s.raw(hl + so, sl);
}

static Vec<Member> rarcMembers(Swapper &s)
{
    Vec<Member> out;
    u32 hl = s.r32(8);
    u32 nf = s.r32(hl + 8), fo = s.r32(hl + 12), sl = s.r32(hl + 16), so = s.r32(hl + 20);
    const char *strs = (const char *)s.data() + hl + so;
    for (u32 i = 0; i < nf; i++) {
        u32 e = hl + fo + i * 20;
        u32 flag = s.r32(e + 4);
        Member m;
        m.attr = (u8)(flag >> 24);
        u32 nameOff = flag & 0xFFFFFF;
        if (m.attr & 2)
            continue; // directory
        m.name = nameOff < sl ? Str(strs + nameOff, strnlen(strs + nameOff, sl - nameOff)) : "?";
        m.entry = e;
        m.dataOff = s.r32(e + 8);
        m.size = s.r32(e + 12);
        out.push_back(m);
    }
    return out;
}

static void putBE32(u8 *p, u32 v) { p[0] = (u8)(v >> 24); p[1] = (u8)(v >> 16); p[2] = (u8)(v >> 8); p[3] = (u8)v; }
static void putLE32(u8 *p, u32 v) { p[3] = (u8)(v >> 24); p[2] = (u8)(v >> 16); p[1] = (u8)(v >> 8); p[0] = (u8)v; }

static bool convertArchive(const Str &name, Vec<u8> &data, bool srcBig, Report &rep)
{
    Swapper s(data.data(), (u32)data.size(), srcBig, name, rep);
    convRARC(s);
    if (!s.ok())
        return true;
    u32 hl = s.r32(8), fdo = s.r32(12);
    u32 base = hl + fdo;
    Vec<Member> members = rarcMembers(s);
    bool rebuild = false;
    for (auto &m : members)
        if (m.attr & 4)
            rebuild = true;
    rep.files++;

    if (!rebuild) {
        // convert members in place; skip ranges already converted (shared data)
        Vec<std::pair<u32, u32>> done;
        for (auto &m : members) {
            if (!s.check(base + m.dataOff, m.size, ("member " + m.name).c_str()))
                continue;
            if (std::find(done.begin(), done.end(), std::make_pair(m.dataOff, m.size)) != done.end())
                continue;
            done.push_back(std::make_pair(m.dataOff, m.size));
            u8 *p = data.data() + base + m.dataOff;
            if (m.size >= 4 && !memcmp(p, srcBig ? "RARC" : "CRAR", 4)) {
                Vec<u8> sub(p, p + m.size);
                convertArchive(name + "/" + m.name, sub, srcBig, rep);
                if (sub.size() == m.size)
                    memcpy(p, sub.data(), m.size);
                else
                    rep.error(name + "/" + m.name, "nested archive changed size");
            } else {
                convertPlain(name + "/" + m.name, p, m.size, srcBig, rep);
            }
        }
        return true;
    }

    // Some members are Yaz0-compressed: expand and convert them, then rebuild the
    // file data region with every member stored uncompressed (32-byte aligned).
    Vec<u8> out(data.begin(), data.begin() + base);
    for (auto &m : members) {
        Vec<u8> body;
        if (s.inRange(base + m.dataOff, m.size))
            body.assign(data.begin() + base + m.dataOff, data.begin() + base + m.dataOff + m.size);
        if (isYaz0(body.data(), (u32)body.size()))
            body = yaz0Decode(body.data(), (u32)body.size());
        if (body.size() >= 4 && !memcmp(body.data(), srcBig ? "RARC" : "CRAR", 4))
            convertArchive(name + "/" + m.name, body, srcBig, rep);
        else
            convertPlain(name + "/" + m.name, body.data(), (u32)body.size(), srcBig, rep);
        u32 off = (u32)(out.size() - base);
        out.insert(out.end(), body.begin(), body.end());
        out.resize((out.size() + 31) & ~31u);
        // entry fields are already in destination order
        u8 *e = out.data() + m.entry;
        u32 flag = srcBig ? (u32)e[7] << 24 | (u32)e[6] << 16 | (u32)e[5] << 8 | e[4]
                          : (u32)e[4] << 24 | (u32)e[5] << 16 | (u32)e[6] << 8 | e[7];
        flag &= ~((u32)(0x04 | 0x80) << 24); // no longer compressed
        flag |= (u32)0x01 << 24;             // stored as a plain file
        void (*put)(u8 *, u32) = srcBig ? putLE32 : putBE32;
        put(e + 4, flag);
        put(e + 8, off);
        put(e + 12, (u32)body.size());
    }
    void (*put)(u8 *, u32) = srcBig ? putLE32 : putBE32;
    put(out.data() + 4, (u32)out.size());                   // file_length
    put(out.data() + 0x10, (u32)(out.size() - base));       // file_data_length
    put(out.data() + 0x14, (u32)(out.size() - base));       // MRAM part: whole archive
    put(out.data() + 0x18, 0);                              // ARAM part
    data.swap(out);
    return true;
}

bool convertFile(const Str &name, Vec<u8> &data, bool srcBig, Report &rep)
{
    if (isYaz0(data.data(), (u32)data.size()))
        data = yaz0Decode(data.data(), (u32)data.size());
    if (data.size() >= 4 && !memcmp(data.data(), srcBig ? "RARC" : "CRAR", 4))
        return convertArchive(name, data, srcBig, rep);
    return convertPlain(name, data.data(), (u32)data.size(), srcBig, rep);
}

} // namespace conv
