/* Memory card: an emulated card in slot A (slot B is empty), at the CARD API level.
 *
 * The card is kept in memory as a raw card image in the hardware layout (the
 * format Dolphin's .raw files use): block 0 is the card header, blocks 1-2 the
 * two directory copies, blocks 3-4 the two FAT copies, then 8 KB data blocks. All
 * system fields are big-endian and carry the SDK checksums, and directory/FAT
 * updates alternate between the two copies with an increasing check code like the
 * SDK does. File contents are stored as the game hands them over: the game
 * serialises its save structs in host (little-endian) byte order, so the files
 * are only meaningful to this port, while the icons/banners and comments are
 * byte-order neutral.
 *
 * Persistence (whole image, after every change):
 *  - node: the file MKDD_CARD (default build/port/card/slotA.raw relative to the
 *    working directory); MKDD_CARD=none leaves slot A empty;
 *  - browser: Module.cardImage (Uint8Array) is the image to start from, and each
 *    change calls Module.cardSave(Uint8Array) (the page stores it in IndexedDB).
 * A missing or unusable image is replaced by a freshly formatted 16 Mbit card
 * (251 free blocks). Every call completes synchronously; the game only uses the
 * synchronous API from its card task thread. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <emscripten.h>
#include "port_sdk.h"

#define BLOCK 8192u
#define NSYS 5
#define DIR_ENT 64
#define DEFAULT_MBITS 16
#define SEG 512u /* read granularity (CARD_SEG_SIZE) */
#define CARD_ENCODE 0 /* ANSI, the US font encoding */

/* directory entry field offsets (CARDDir) */
#define D_GAME 0
#define D_COMPANY 4
#define D_BANNER 7
#define D_NAME 8
#define D_TIME 40
#define D_ICONADDR 44
#define D_ICONFMT 48
#define D_ICONSPEED 50
#define D_PERM 52
#define D_COPY 53
#define D_START 54
#define D_LENGTH 56
#define D_COMMENT 60
/* directory block check fields (CARDDirCheck at entry 127) */
#define DC_CODE 8186
#define DC_SUM 8188
/* FAT word indices */
#define F_SUM 0
#define F_INV 1
#define F_CODE 2
#define F_FREE 3
#define F_LAST 4

static u8 *sImage;  /* the whole card */
static u32 sSize;   /* bytes */
static u16 sBlocks; /* 8 KB blocks, including the 5 system blocks */
static int sPresent, sLoaded, sMounted;
static int sCurDir, sCurFat;        /* which copy (0/1) is current */
static int sRepaired;               /* verify() fixed a damaged copy */
static u8 sDir[BLOCK], sFat[BLOCK]; /* working copies of the current directory and FAT */
static char sPath[512];

static u16 rd16(const u8 *p) { return (u16)(p[0] << 8 | p[1]); }
static u32 rd32(const u8 *p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }
static void wr16(u8 *p, u16 v) { p[0] = (u8)(v >> 8); p[1] = (u8)v; }
static void wr32(u8 *p, u32 v) { p[0] = (u8)(v >> 24); p[1] = (u8)(v >> 16); p[2] = (u8)(v >> 8); p[3] = (u8)v; }

static u16 fat_get(int i) { return rd16(sFat + 2 * i); }
static void fat_set(int i, u16 v) { wr16(sFat + 2 * i, v); }
static u8 *dir_ent(int fileNo) { return sDir + fileNo * DIR_ENT; }
static int valid_block(u16 b) { return NSYS <= b && b < sBlocks; }

/* __CARDCheckSum over big-endian halfwords */
static void checksum(const u8 *p, int len, u16 *sum, u16 *inv)
{
    u16 s = 0, n = 0;
    for (int i = 0; i < len / 2; i++) {
        u16 w = rd16(p + 2 * i);
        s += w;
        n += (u16)~w;
    }
    *sum = s == 0xffff ? 0 : s;
    *inv = n == 0xffff ? 0 : n;
}

static int header_ok(void)
{
    u16 s, n;
    checksum(sImage, 0x1fc, &s, &n);
    return s == rd16(sImage + 0x1fc) && n == rd16(sImage + 0x1fe);
}

static int dir_ok(const u8 *d)
{
    u16 s, n;
    checksum(d, BLOCK - 4, &s, &n);
    return s == rd16(d + DC_SUM) && n == rd16(d + DC_SUM + 2);
}

static int fat_ok(const u8 *f)
{
    u16 s, n;
    checksum(f + 4, BLOCK - 4, &s, &n);
    if (s != rd16(f) || n != rd16(f + 2))
        return 0;
    u16 freeCount = 0;
    for (int b = NSYS; b < sBlocks; b++)
        if (rd16(f + 2 * b) == 0)
            freeCount++;
    return freeCount == rd16(f + 2 * F_FREE);
}

/* ---- persistence ---- */

EM_JS(int, port_js_card_is_node, (void), {
    return (typeof process !== 'undefined' && process.versions && process.versions.node) ? 1 : 0;
});
EM_JS(const char *, port_js_card_path, (void), {
    var p = (typeof process !== 'undefined' && process.env && process.env.MKDD_CARD) || '';
    return stringToNewUTF8(p);
});
EM_JS(int, port_js_card_browser_size, (void), {
    if (typeof Module === 'undefined') return -1;
    if (Module.cardDisabled) return -2;
    return Module.cardImage ? Module.cardImage.length : -1;
});
EM_JS(void, port_js_card_browser_load, (u8 *dst, u32 len), {
    HEAPU8.set(Module.cardImage.subarray(0, len), dst);
});
EM_JS(void, port_js_card_browser_save, (const u8 *src, u32 len), {
    var img = HEAPU8.slice(src, src + len);
    Module.cardImage = img;
    if (Module.cardSave) Module.cardSave(img);
});

static int usable_size(long n)
{
    /* 4..128 Mbit cards */
    for (long m = 4; m <= 128; m *= 2)
        if (n == m * 1024 * 1024 / 8)
            return 1;
    return 0;
}

static void mkdirs_for(const char *path)
{
    char tmp[512];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0777);
            *p = '/';
        }
    }
}

static void card_save(void)
{
    if (!sImage)
        return;
    if (!port_js_card_is_node()) {
        port_js_card_browser_save(sImage, sSize);
        return;
    }
    char tmp[540];
    snprintf(tmp, sizeof tmp, "%s.tmp", sPath);
    mkdirs_for(sPath);
    FILE *f = fopen(tmp, "wb");
    if (!f || fwrite(sImage, 1, sSize, f) != sSize) {
        OSReport("card: cannot write %s (%s)\n", tmp, strerror(errno));
        if (f)
            fclose(f);
        return;
    }
    fclose(f);
    if (rename(tmp, sPath) != 0)
        OSReport("card: cannot rename %s (%s)\n", tmp, strerror(errno));
}

static u32 now_seconds(void)
{
    /* seconds since 2000-01-01 00:00 */
    return (u32)((s64)time(NULL) - 946684800);
}

/* ---- format / directory and FAT updates ---- */

static void format_image(void)
{
    u32 t = now_seconds();
    memset(sImage, 0xff, sSize);

    /* CARDID */
    u8 *id = sImage;
    u32 rnd = t * 1103515245u + 12345u;
    for (int i = 0; i < 12; i++) {
        rnd = rnd * 1103515245u + 12345u;
        id[i] = (u8)(rnd >> 16);
    }
    u64 formatTime = (u64)t * PORT_TICKS_PER_SEC;
    wr32(id + 12, (u32)(formatTime >> 32));
    wr32(id + 16, (u32)formatTime);
    wr32(id + 20, 0); /* counterBias */
    wr32(id + 24, 0); /* language: English */
    wr32(id + 28, 0); /* viDTVStatus */
    wr16(id + 0x20, 0);
    wr16(id + 0x22, (u16)(sSize * 8 / (1024 * 1024)));
    wr16(id + 0x24, CARD_ENCODE);
    u16 s, n;
    checksum(id, 0x1fc, &s, &n);
    wr16(id + 0x1fc, s);
    wr16(id + 0x1fe, n);

    for (int i = 0; i < 2; i++) {
        u8 *d = sImage + (1 + i) * BLOCK;
        wr16(d + DC_CODE, (u16)i);
        checksum(d, BLOCK - 4, &s, &n);
        wr16(d + DC_SUM, s);
        wr16(d + DC_SUM + 2, n);
    }
    for (int i = 0; i < 2; i++) {
        u8 *f = sImage + (3 + i) * BLOCK;
        memset(f, 0, BLOCK);
        wr16(f + 2 * F_CODE, (u16)i);
        wr16(f + 2 * F_FREE, (u16)(sBlocks - NSYS));
        wr16(f + 2 * F_LAST, NSYS - 1);
        checksum(f + 4, BLOCK - 4, &s, &n);
        wr16(f, s);
        wr16(f + 2, n);
    }
}

/* Pick the current directory/FAT copies (the valid one with the newer check
 * code) and repair a damaged other copy. Returns CARD_RESULT_READY or _BROKEN. */
static s32 verify(void)
{
    if (!header_ok())
        return CARD_RESULT_BROKEN;
    if (rd16(sImage + 0x24) != CARD_ENCODE)
        return CARD_RESULT_ENCODING;

    u8 *d0 = sImage + 1 * BLOCK, *d1 = sImage + 2 * BLOCK;
    int v0 = dir_ok(d0), v1 = dir_ok(d1);
    if (!v0 && !v1)
        return CARD_RESULT_BROKEN;
    if (v0 && v1)
        sCurDir = (s16)(rd16(d1 + DC_CODE) - rd16(d0 + DC_CODE)) > 0 ? 1 : 0;
    else
        sCurDir = v1 ? 1 : 0;
    if (!(v0 && v1)) {
        memcpy(sCurDir ? d0 : d1, sCurDir ? d1 : d0, BLOCK);
        sRepaired = 1;
    }

    u8 *f0 = sImage + 3 * BLOCK, *f1 = sImage + 4 * BLOCK;
    v0 = fat_ok(f0);
    v1 = fat_ok(f1);
    if (!v0 && !v1)
        return CARD_RESULT_BROKEN;
    if (v0 && v1)
        sCurFat = (s16)(rd16(f1 + 2 * F_CODE) - rd16(f0 + 2 * F_CODE)) > 0 ? 1 : 0;
    else
        sCurFat = v1 ? 1 : 0;
    if (!(v0 && v1)) {
        memcpy(sCurFat ? f0 : f1, sCurFat ? f1 : f0, BLOCK);
        sRepaired = 1;
    }

    memcpy(sDir, sImage + (1 + sCurDir) * BLOCK, BLOCK);
    memcpy(sFat, sImage + (3 + sCurFat) * BLOCK, BLOCK);
    return CARD_RESULT_READY;
}

/* __CARDUpdateDir: write the working directory to the other copy, newer code. */
static void commit_dir(void)
{
    u16 s, n;
    wr16(sDir + DC_CODE, (u16)(rd16(sDir + DC_CODE) + 1));
    checksum(sDir, BLOCK - 4, &s, &n);
    wr16(sDir + DC_SUM, s);
    wr16(sDir + DC_SUM + 2, n);
    sCurDir ^= 1;
    memcpy(sImage + (1 + sCurDir) * BLOCK, sDir, BLOCK);
}

static void commit_fat(void)
{
    u16 s, n;
    fat_set(F_CODE, (u16)(fat_get(F_CODE) + 1));
    checksum(sFat + 4, BLOCK - 4, &s, &n);
    fat_set(F_SUM, s);
    fat_set(F_INV, n);
    sCurFat ^= 1;
    memcpy(sImage + (3 + sCurFat) * BLOCK, sFat, BLOCK);
}

/* __CARDAllocBlock; returns the first block or 0xffff */
static u16 alloc_blocks(u16 count)
{
    if (fat_get(F_FREE) < count)
        return 0xffff;
    fat_set(F_FREE, (u16)(fat_get(F_FREE) - count));
    u16 start = 0xffff, prev = 0, b = fat_get(F_LAST);
    int tries = 0;
    while (count > 0) {
        if (++tries > sBlocks - NSYS)
            return 0xffff;
        b++;
        if (!valid_block(b))
            b = NSYS;
        if (fat_get(b) == 0) {
            if (start == 0xffff)
                start = b;
            else
                fat_set(prev, b);
            prev = b;
            fat_set(b, 0xffff);
            count--;
        }
    }
    fat_set(F_LAST, b);
    return start;
}

static void free_blocks(u16 b)
{
    while (valid_block(b)) {
        u16 next = fat_get(b);
        fat_set(b, 0);
        fat_set(F_FREE, (u16)(fat_get(F_FREE) + 1));
        b = next;
    }
}

/* ---- access checks (CARDOpen.c) ---- */

static int ent_used(const u8 *e) { return e[D_GAME] != 0xff; }

static s32 ent_access(const u8 *e)
{
    if (!ent_used(e))
        return CARD_RESULT_NOFILE;
    const DVDDiskID *id = DVDGetCurrentDiskID();
    if (memcmp(e + D_GAME, id->gameName, 4) == 0 && memcmp(e + D_COMPANY, id->company, 2) == 0)
        return CARD_RESULT_READY;
    return CARD_RESULT_NOPERM;
}

static s32 ent_readable(const u8 *e)
{
    s32 r = ent_access(e);
    if (r == CARD_RESULT_NOPERM && (e[D_PERM] & CARD_ATTR_PUBLIC))
        return CARD_RESULT_READY;
    return r;
}

static int name_eq(const u8 *e, const char *name)
{
    return strncmp((const char *)e + D_NAME, name, CARD_FILENAME_MAX) == 0;
}

static s32 find_file(const char *name, s32 *fileNo)
{
    for (int i = 0; i < CARD_MAX_FILE; i++) {
        const u8 *e = dir_ent(i);
        if (ent_access(e) == CARD_RESULT_READY && name_eq(e, name)) {
            *fileNo = i;
            return CARD_RESULT_READY;
        }
    }
    return CARD_RESULT_NOFILE;
}

/* ---- loading ---- */

static void card_load(void)
{
    if (sLoaded)
        return;
    sLoaded = 1;
    long size = -1;
    int fromImage = 0;
    sSize = DEFAULT_MBITS * 1024 * 1024 / 8;

    if (port_js_card_is_node()) {
        const char *p = port_js_card_path();
        snprintf(sPath, sizeof sPath, "%s", p[0] ? p : "build/port/card/slotA.raw");
        free((void *)p);
        if (strcmp(sPath, "none") == 0) {
            OSReport("card: slot A empty (MKDD_CARD=none)\n");
            return;
        }
        FILE *f = fopen(sPath, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            size = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (usable_size(size)) {
                sSize = (u32)size;
                sImage = malloc(sSize);
                fromImage = fread(sImage, 1, sSize, f) == sSize;
            } else {
                OSReport("card: %s has an unusable size (%ld), using a new card\n", sPath, size);
            }
            fclose(f);
        }
    } else {
        size = port_js_card_browser_size();
        if (size == -2) {
            OSReport("card: slot A empty\n");
            return;
        }
        if (usable_size(size)) {
            sSize = (u32)size;
            sImage = malloc(sSize);
            port_js_card_browser_load(sImage, sSize);
            fromImage = 1;
        }
    }

    sPresent = 1;
    if (!sImage)
        sImage = malloc(sSize);
    sBlocks = (u16)(sSize / BLOCK);
    if (!fromImage) {
        format_image();
        OSReport("card: slot A: new %u-block memory card%s%s\n", sBlocks - NSYS,
                 sPath[0] ? " at " : "", sPath);
        card_save();
    } else {
        OSReport("card: slot A: %u-block memory card%s%s\n", sBlocks - NSYS, sPath[0] ? " from " : "", sPath);
    }
}

static s32 chan_state(s32 chan)
{
    if (chan != 0 || !sPresent)
        return CARD_RESULT_NOCARD;
    return sMounted ? CARD_RESULT_READY : CARD_RESULT_NOCARD;
}

/* ---- API ---- */

void CARDInit(void) { card_load(); }

s32 CARDProbeEx(s32 chan, s32 *memSize, s32 *sectorSize)
{
    card_load();
    if (chan != 0 || !sPresent)
        return CARD_RESULT_NOCARD;
    if (memSize)
        *memSize = (s32)(sSize * 8 / (1024 * 1024));
    if (sectorSize)
        *sectorSize = BLOCK;
    return CARD_RESULT_READY;
}

s32 CARDMount(s32 chan, void *workArea, CARDCallback detachCallback)
{
    (void)workArea;
    (void)detachCallback;
    card_load();
    if (chan != 0 || !sPresent)
        return CARD_RESULT_NOCARD;
    sMounted = 1;
    return verify();
}

s32 CARDUnmount(s32 chan)
{
    if (chan != 0 || !sPresent)
        return CARD_RESULT_NOCARD;
    sMounted = 0;
    return CARD_RESULT_READY;
}

s32 CARDCheck(s32 chan)
{
    s32 r = chan_state(chan);
    if (r < 0)
        return r;
    r = verify();
    if (r == CARD_RESULT_READY && sRepaired) {
        sRepaired = 0;
        card_save();
    }
    return r;
}

s32 CARDFormat(s32 chan)
{
    s32 r = chan_state(chan);
    if (r < 0)
        return r;
    format_image();
    r = verify();
    card_save();
    return r;
}

s32 CARDFreeBlocks(s32 chan, s32 *byteNotUsed, s32 *filesNotUsed)
{
    s32 r = chan_state(chan);
    if (r < 0)
        return r;
    if (byteNotUsed)
        *byteNotUsed = (s32)(BLOCK * fat_get(F_FREE));
    if (filesNotUsed) {
        s32 n = 0;
        for (int i = 0; i < CARD_MAX_FILE; i++)
            if (!ent_used(dir_ent(i)))
                n++;
        *filesNotUsed = n;
    }
    return CARD_RESULT_READY;
}

static void open_info(s32 chan, s32 fileNo, CARDFileInfo *fileInfo)
{
    fileInfo->chan = chan;
    fileInfo->fileNo = fileNo;
    fileInfo->offset = 0;
    fileInfo->length = (s32)(rd16(dir_ent(fileNo) + D_LENGTH) * BLOCK);
    fileInfo->iBlock = rd16(dir_ent(fileNo) + D_START);
}

s32 CARDOpen(s32 chan, const char *fileName, CARDFileInfo *fileInfo)
{
    fileInfo->chan = -1;
    s32 r = chan_state(chan);
    if (r < 0)
        return r;
    s32 fileNo;
    r = find_file(fileName, &fileNo);
    if (r < 0)
        return r;
    if (!valid_block(rd16(dir_ent(fileNo) + D_START)))
        return CARD_RESULT_BROKEN;
    open_info(chan, fileNo, fileInfo);
    return CARD_RESULT_READY;
}

s32 CARDFastOpen(s32 chan, s32 fileNo, CARDFileInfo *fileInfo)
{
    if (fileNo < 0 || fileNo >= CARD_MAX_FILE)
        return CARD_RESULT_FATAL_ERROR;
    fileInfo->chan = -1;
    s32 r = chan_state(chan);
    if (r < 0)
        return r;
    r = ent_readable(dir_ent(fileNo));
    if (r < 0)
        return r;
    if (!valid_block(rd16(dir_ent(fileNo) + D_START)))
        return CARD_RESULT_BROKEN;
    open_info(chan, fileNo, fileInfo);
    return CARD_RESULT_READY;
}

s32 CARDClose(CARDFileInfo *fileInfo)
{
    if (chan_state(fileInfo->chan) < 0)
        return CARD_RESULT_NOCARD;
    fileInfo->chan = -1;
    return CARD_RESULT_READY;
}

s32 CARDCreate(s32 chan, const char *fileName, u32 size, CARDFileInfo *fileInfo)
{
    if (strlen(fileName) > CARD_FILENAME_MAX)
        return CARD_RESULT_NAMETOOLONG;
    s32 r = chan_state(chan);
    if (r < 0)
        return r;
    if (size == 0 || size % BLOCK != 0)
        return CARD_RESULT_FATAL_ERROR;

    const DVDDiskID *id = DVDGetCurrentDiskID();
    int freeNo = -1;
    for (int i = 0; i < CARD_MAX_FILE; i++) {
        u8 *e = dir_ent(i);
        if (!ent_used(e)) {
            if (freeNo < 0)
                freeNo = i;
        } else if (ent_access(e) == CARD_RESULT_READY && name_eq(e, fileName)) {
            return CARD_RESULT_EXIST;
        }
    }
    if (freeNo < 0)
        return CARD_RESULT_NOENT;
    if (BLOCK * fat_get(F_FREE) < size)
        return CARD_RESULT_INSSPACE;

    u16 start = alloc_blocks((u16)(size / BLOCK));
    if (start == 0xffff) {
        verify(); /* drop the partial allocation */
        return CARD_RESULT_BROKEN;
    }
    commit_fat();

    u8 *e = dir_ent(freeNo);
    memset(e, 0xff, DIR_ENT);
    memcpy(e + D_GAME, id->gameName, 4);
    memcpy(e + D_COMPANY, id->company, 2);
    memset(e + D_NAME, 0, CARD_FILENAME_MAX);
    strncpy((char *)e + D_NAME, fileName, CARD_FILENAME_MAX);
    e[D_BANNER] = 0;
    e[D_PERM] = CARD_ATTR_PUBLIC;
    e[D_COPY] = 0;
    wr16(e + D_START, start);
    wr16(e + D_LENGTH, (u16)(size / BLOCK));
    wr32(e + D_ICONADDR, 0xffffffff);
    wr16(e + D_ICONFMT, 0);
    wr16(e + D_ICONSPEED, CARD_STAT_SPEED_FAST);
    wr32(e + D_COMMENT, 0xffffffff);
    wr32(e + D_TIME, now_seconds());
    commit_dir();
    card_save();

    open_info(chan, freeNo, fileInfo);
    return CARD_RESULT_READY;
}

/* Byte address on the card of file offset `offset`, or 0 past the end. */
static u32 file_addr(s32 fileNo, u32 offset)
{
    u16 b = rd16(dir_ent(fileNo) + D_START);
    for (u32 i = 0; i < offset / BLOCK; i++) {
        if (!valid_block(b))
            return 0;
        b = fat_get(b);
    }
    if (!valid_block(b))
        return 0;
    return b * BLOCK + offset % BLOCK;
}

static s32 transfer(CARDFileInfo *fileInfo, void *buf, s32 length, s32 offset, u32 align, int write)
{
    s32 r = chan_state(fileInfo->chan);
    if (r < 0)
        return r;
    s32 fileNo = fileInfo->fileNo;
    if (fileNo < 0 || fileNo >= CARD_MAX_FILE)
        return CARD_RESULT_FATAL_ERROR;
    if (offset < 0 || length < 0 || (offset % align) != 0 || (length % align) != 0) {
        OSReport("card: unaligned %s offset %d length %d\n", write ? "write" : "read", offset, length);
        return CARD_RESULT_FATAL_ERROR;
    }
    u8 *e = dir_ent(fileNo);
    r = write ? ent_access(e) : ent_readable(e);
    if (r < 0)
        return r;
    if ((u32)rd16(e + D_LENGTH) * BLOCK < (u32)(offset + length))
        return CARD_RESULT_LIMIT;

    u8 *p = buf;
    u32 off = (u32)offset, left = (u32)length;
    while (left) {
        u32 addr = file_addr(fileNo, off);
        if (!addr)
            return CARD_RESULT_BROKEN;
        u32 n = BLOCK - off % BLOCK;
        if (n > left)
            n = left;
        if (write)
            memcpy(sImage + addr, p, n);
        else
            memcpy(p, sImage + addr, n);
        p += n;
        off += n;
        left -= n;
    }
    fileInfo->offset = (s32)off;
    if (write) {
        wr32(e + D_TIME, now_seconds());
        commit_dir();
        card_save();
    }
    return CARD_RESULT_READY;
}

s32 CARDRead(CARDFileInfo *fileInfo, void *addr, s32 length, s32 offset)
{
    return transfer(fileInfo, addr, length, offset, SEG, 0);
}

s32 CARDWrite(CARDFileInfo *fileInfo, const void *addr, s32 length, s32 offset)
{
    return transfer(fileInfo, (void *)addr, length, offset, BLOCK, 1);
}

static s32 delete_no(s32 fileNo)
{
    u8 *e = dir_ent(fileNo);
    s32 r = ent_access(e);
    if (r < 0)
        return r;
    u16 start = rd16(e + D_START);
    memset(e, 0xff, DIR_ENT);
    commit_dir();
    free_blocks(start);
    commit_fat();
    card_save();
    return CARD_RESULT_READY;
}

s32 CARDDelete(s32 chan, const char *fileName)
{
    s32 r = chan_state(chan);
    if (r < 0)
        return r;
    s32 fileNo;
    r = find_file(fileName, &fileNo);
    if (r < 0)
        return r;
    return delete_no(fileNo);
}

s32 CARDFastDelete(s32 chan, s32 fileNo)
{
    if (fileNo < 0 || fileNo >= CARD_MAX_FILE)
        return CARD_RESULT_FATAL_ERROR;
    s32 r = chan_state(chan);
    if (r < 0)
        return r;
    return delete_no(fileNo);
}

s32 CARDRename(s32 chan, const char *oldName, const char *newName)
{
    if (oldName[0] == (char)0xff || newName[0] == (char)0xff || oldName[0] == 0 || newName[0] == 0)
        return CARD_RESULT_FATAL_ERROR;
    if (strlen(oldName) > CARD_FILENAME_MAX || strlen(newName) > CARD_FILENAME_MAX)
        return CARD_RESULT_NAMETOOLONG;
    s32 r = chan_state(chan);
    if (r < 0)
        return r;
    int oldNo = -1;
    for (int i = 0; i < CARD_MAX_FILE; i++) {
        u8 *e = dir_ent(i);
        if (ent_access(e) != CARD_RESULT_READY)
            continue;
        if (name_eq(e, oldName))
            oldNo = i;
        if (name_eq(e, newName))
            return CARD_RESULT_EXIST;
    }
    if (oldNo < 0)
        return CARD_RESULT_NOFILE;
    u8 *e = dir_ent(oldNo);
    memset(e + D_NAME, 0, CARD_FILENAME_MAX);
    strncpy((char *)e + D_NAME, newName, CARD_FILENAME_MAX);
    wr32(e + D_TIME, now_seconds());
    commit_dir();
    card_save();
    return CARD_RESULT_READY;
}

/* CARDStat.c UpdateIconOffsets */
static void icon_offsets(const u8 *e, CARDStat *stat)
{
    u32 offset = rd32(e + D_ICONADDR);
    u16 iconFormat = rd16(e + D_ICONFMT);
    u8 bannerFormat = e[D_BANNER];
    int iconTlut = 0;
    if (offset == 0xffffffff) {
        stat->bannerFormat = 0;
        stat->iconFormat = 0;
        stat->iconSpeed = 0;
        bannerFormat = 0;
        iconFormat = 0;
        offset = 0;
    }
    switch (bannerFormat & CARD_STAT_BANNER_MASK) {
    case CARD_STAT_BANNER_C8:
        stat->offsetBanner = offset;
        offset += CARD_BANNER_WIDTH * CARD_BANNER_HEIGHT;
        stat->offsetBannerTlut = offset;
        offset += 2 * 256;
        break;
    case CARD_STAT_BANNER_RGB5A3:
        stat->offsetBanner = offset;
        offset += 2 * CARD_BANNER_WIDTH * CARD_BANNER_HEIGHT;
        stat->offsetBannerTlut = 0xffffffff;
        break;
    default:
        stat->offsetBanner = 0xffffffff;
        stat->offsetBannerTlut = 0xffffffff;
        break;
    }
    for (int i = 0; i < CARD_ICON_MAX; i++) {
        switch ((iconFormat >> (2 * i)) & CARD_STAT_ICON_MASK) {
        case CARD_STAT_ICON_C8:
            stat->offsetIcon[i] = offset;
            offset += CARD_ICON_WIDTH * CARD_ICON_HEIGHT;
            iconTlut = 1;
            break;
        case CARD_STAT_ICON_RGB5A3:
            stat->offsetIcon[i] = offset;
            offset += 2 * CARD_ICON_WIDTH * CARD_ICON_HEIGHT;
            break;
        default:
            stat->offsetIcon[i] = 0xffffffff;
            break;
        }
    }
    if (iconTlut) {
        stat->offsetIconTlut = offset;
        offset += 2 * 256;
    } else {
        stat->offsetIconTlut = 0xffffffff;
    }
    stat->offsetData = offset;
}

s32 CARDGetStatus(s32 chan, s32 fileNo, CARDStat *stat)
{
    if (fileNo < 0 || fileNo >= CARD_MAX_FILE)
        return CARD_RESULT_FATAL_ERROR;
    s32 r = chan_state(chan);
    if (r < 0)
        return r;
    const u8 *e = dir_ent(fileNo);
    r = ent_readable(e);
    if (r < 0)
        return r;
    memcpy(stat->gameName, e + D_GAME, 4);
    memcpy(stat->company, e + D_COMPANY, 2);
    stat->length = (u32)rd16(e + D_LENGTH) * BLOCK;
    memcpy(stat->fileName, e + D_NAME, CARD_FILENAME_MAX);
    stat->time = rd32(e + D_TIME);
    stat->bannerFormat = e[D_BANNER];
    stat->iconAddr = rd32(e + D_ICONADDR);
    stat->iconFormat = rd16(e + D_ICONFMT);
    stat->iconSpeed = rd16(e + D_ICONSPEED);
    stat->commentAddr = rd32(e + D_COMMENT);
    icon_offsets(e, stat);
    return CARD_RESULT_READY;
}

s32 CARDSetStatus(s32 chan, s32 fileNo, CARDStat *stat)
{
    if (fileNo < 0 || fileNo >= CARD_MAX_FILE ||
        (stat->iconAddr != 0xffffffff && CARD_READ_SIZE <= stat->iconAddr) ||
        (stat->commentAddr != 0xffffffff &&
         BLOCK - CARD_COMMENT_SIZE < stat->commentAddr % BLOCK))
        return CARD_RESULT_FATAL_ERROR;
    s32 r = chan_state(chan);
    if (r < 0)
        return r;
    u8 *e = dir_ent(fileNo);
    r = ent_access(e);
    if (r < 0)
        return r;
    e[D_BANNER] = stat->bannerFormat;
    wr32(e + D_ICONADDR, stat->iconAddr);
    wr16(e + D_ICONFMT, stat->iconFormat);
    wr16(e + D_ICONSPEED, stat->iconSpeed);
    wr32(e + D_COMMENT, stat->commentAddr);
    icon_offsets(e, stat);
    if (stat->iconAddr == 0xffffffff)
        wr16(e + D_ICONSPEED, (u16)((rd16(e + D_ICONSPEED) & ~CARD_STAT_SPEED_MASK) | CARD_STAT_SPEED_FAST));
    wr32(e + D_TIME, now_seconds());
    commit_dir();
    card_save();
    return CARD_RESULT_READY;
}
