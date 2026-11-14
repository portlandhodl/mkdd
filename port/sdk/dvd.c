/* DVD: the disc filesystem, served from the user's own disc.
 *
 * Two sources:
 *  - a disc image (GCM/ISO): MKDD_DISC=/path/game.iso under node, or the File
 *    the user picked in the browser (Module.discFile), read on demand;
 *  - an extracted directory (files/ and sys/, as port/tools/extract_disc.py writes).
 * Entry numbers index a table in FST order; lookups are case-insensitive as on
 * hardware. Formats port/conv knows are served as converted (host-order) copies.
 * Reads complete immediately; async callbacks are delivered from port_poll(). */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <emscripten.h>
#include "port_sdk.h"

typedef struct Entry {
    char *name;     /* component name */
    char *hostPath; /* directory source: full host path */
    u32 discOffset; /* image source: byte offset of the file on the disc */
    int parent;
    int isDir;
    int next;       /* index after this entry's subtree (dirs) */
    u32 rawSize;    /* size on disc */
    u32 size;       /* as served: the converted size for converted files */
    u8 *conv;       /* host-order copy (port/conv), loaded on first open */
    int convOpen;   /* open handles using conv */
    int convState;  /* 0 unknown, 1 converted, 2 served raw */
} Entry;

int port_conv_wants(const char *name);
unsigned char *port_conv_file(const char *name, const unsigned char *in, unsigned inSize, unsigned *outSize);

static Entry *sEntries;
static int sNumEntries, sCapEntries;
static char sRoot[512];
static DVDDiskID sDiskID;
static int sCurrentDir;
static int sImage;     /* 1: disc image source */
static FILE *sImageFile; /* node: the image file */

EM_JS(const char *, port_js_disc_root, (void), {
    var p = (typeof process !== 'undefined' && process.env && process.env.MKDD_DISC) || '';
    return stringToNewUTF8(p);
});

EM_JS(int, port_js_env, (const char *name), {
    var v = (typeof process !== 'undefined' && process.env) ? process.env[UTF8ToString(name)] : undefined;
    if (v === undefined && typeof Module !== 'undefined' && Module.env) v = Module.env[UTF8ToString(name)];
    return v ? (parseInt(v, 10) || 1) : 0;
});

/* browser: the disc image is a File object; read a slice of it */
EM_JS(int, port_js_has_disc_file, (void), { return (typeof Module !== 'undefined' && Module.discFile) ? 1 : 0; });
EM_ASYNC_JS(int, port_js_disc_read, (u32 off, u32 len, u8 *dst), {
    const ab = await Module.discFile.slice(off, off + len).arrayBuffer();
    HEAPU8.set(new Uint8Array(ab), dst);
    return ab.byteLength;
});

int port_env(const char *name) { return port_js_env(name); }

const char *port_disc_root(void)
{
    if (!sRoot[0]) {
        const char *r = port_js_disc_root();
        snprintf(sRoot, sizeof sRoot, "%s", r);
        free((void *)r);
    }
    return sRoot;
}

/* Browser reads without suspending the whole program: start the File read,
 * and let only the calling thread wait for it (port_dvd_poll wakes it), as a
 * drive read blocks only its caller on hardware. With EM_ASYNC_JS every read
 * (audio streams, loads) stalled all threads, rendering included, until the
 * browser returned the data. */
EM_JS(int, port_js_disc_read_start, (u32 off, u32 len, u8 *dst), {
    var r = Module.portDiscReads || (Module.portDiscReads = { next: 1, done: {} });
    var id = r.next++;
    var finish = function(n) {
        r.done[id] = n;
        if (r.wake) { var w = r.wake; r.wake = null; w(); }
    };
    Module.discFile.slice(off, off + len).arrayBuffer().then(function(ab) {
        HEAPU8.set(new Uint8Array(ab), dst);
        finish(ab.byteLength);
    }, function(e) {
        console.error('[dvd] read failed: ' + e);
        finish(0);
    });
    return id;
});
EM_JS(int, port_js_disc_read_poll, (int id), {
    var r = Module.portDiscReads;
    if (!r || !(id in r.done)) return -1;
    var n = r.done[id];
    delete r.done[id];
    return n;
});

/* Idle until `ms` passed or a disc read completed (one suspension, woken early). */
EM_ASYNC_JS(void, port_js_dvd_idle, (int ms), {
    await new Promise(function(res) {
        var r = Module.portDiscReads || (Module.portDiscReads = { next: 1, done: {} });
        var t = setTimeout(function() { r.wake = null; res(); }, ms);
        r.wake = function() { clearTimeout(t); res(); };
    });
});

/* Scheduler idle with reads in flight (os_thread.c). */
void port_dvd_idle(unsigned ms) { port_js_dvd_idle((int)ms); }

static OSThreadQueue sReadQueue;
static int sReadsPending;

int port_dvd_reads_pending(void) { return sReadsPending; }

/* From port_poll: let the threads waiting for a read check it. */
void port_dvd_poll(void)
{
    if (sReadsPending)
        OSWakeupThread(&sReadQueue);
}

/* Read from the disc image. Returns bytes read. */
static u32 image_read(u32 off, u32 len, void *dst)
{
    int id, n;
    if (sImageFile) {
        fseek(sImageFile, (long)off, SEEK_SET);
        return (u32)fread(dst, 1, len, sImageFile);
    }
    static int blocking = -1;
    if (blocking < 0)
        blocking = port_env("MKDD_DVD_BLOCKING"); /* the old behaviour, for comparison */
    if (port_perf_enabled)
        port_perf_add(PERF_READS, 1);
    if (blocking || !port_thread_can_wait())
        return (u32)port_js_disc_read(off, len, (u8 *)dst);
    id = port_js_disc_read_start(off, len, (u8 *)dst);
    sReadsPending++;
    while ((n = port_js_disc_read_poll(id)) < 0)
        OSSleepThread(&sReadQueue);
    sReadsPending--;
    return (u32)n;
}

static u32 be32(const u8 *p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }

static int add_entry(const char *name, const char *hostPath, int parent, int isDir, u32 size)
{
    Entry *e;
    if (sNumEntries == sCapEntries) {
        sCapEntries = sCapEntries ? sCapEntries * 2 : 256;
        sEntries = (Entry *)realloc(sEntries, sCapEntries * sizeof(Entry));
    }
    e = &sEntries[sNumEntries];
    memset(e, 0, sizeof *e);
    e->name = strdup(name);
    e->hostPath = hostPath ? strdup(hostPath) : NULL;
    e->parent = parent;
    e->isDir = isDir;
    e->rawSize = e->size = size;
    return sNumEntries++;
}

/* ---- extracted-directory source ---- */

static int cmp_names(const void *a, const void *b) { return strcasecmp(*(char *const *)a, *(char *const *)b); }

static void walk(const char *hostDir, int self)
{
    DIR *d = opendir(hostDir);
    struct dirent *de;
    char **names = NULL;
    int n = 0, cap = 0, i;
    if (!d) {
        fprintf(stderr, "[dvd] cannot open %s\n", hostDir);
        sEntries[self].next = sNumEntries;
        return;
    }
    while ((de = readdir(d))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        if (n == cap)
            names = (char **)realloc(names, (cap = cap ? cap * 2 : 32) * sizeof(char *));
        names[n++] = strdup(de->d_name);
    }
    closedir(d);
    qsort(names, n, sizeof(char *), cmp_names);
    for (i = 0; i < n; i++) {
        char path[1024];
        struct stat st;
        int idx;
        snprintf(path, sizeof path, "%s/%s", hostDir, names[i]);
        if (stat(path, &st) == 0) {
            idx = add_entry(names[i], path, self, S_ISDIR(st.st_mode), (u32)st.st_size);
            if (S_ISDIR(st.st_mode))
                walk(path, idx);
        }
        free(names[i]);
    }
    free(names);
    sEntries[self].next = sNumEntries;
}

/* ---- disc image source: boot.bin + FST ---- */

static void load_image_fst(void)
{
    u8 hdr[0x440];
    u32 fstOff, fstSize, n, i;
    u8 *fst;
    const char *names;
    int stack[64], depth = 0;
    image_read(0, sizeof hdr, hdr);
    memcpy(&sDiskID, hdr, sizeof sDiskID);
    fstOff = be32(hdr + 0x424);
    fstSize = be32(hdr + 0x428);
    fst = (u8 *)malloc(fstSize);
    image_read(fstOff, fstSize, fst);
    n = be32(fst + 8);
    names = (const char *)fst + n * 12;
    add_entry("", NULL, 0, 1, 0);
    sEntries[0].next = (int)n;
    stack[depth++] = 0;
    for (i = 1; i < n; i++) {
        const u8 *e = fst + i * 12;
        int isDir = e[0] != 0, idx;
        while (depth > 1 && (int)i >= sEntries[stack[depth - 1]].next)
            depth--;
        idx = add_entry(names + (be32(e) & 0xFFFFFF), NULL, stack[depth - 1], isDir, isDir ? 0 : be32(e + 8));
        if (isDir) {
            sEntries[idx].next = (int)be32(e + 8);
            if (depth < 64)
                stack[depth++] = idx;
        } else {
            sEntries[idx].discOffset = be32(e + 4);
        }
    }
    free(fst);
}

void DVDInit(void)
{
    const char *root;
    if (sNumEntries)
        return;
    OSInitThreadQueue(&sReadQueue);
    root = port_disc_root();
    if (port_js_has_disc_file()) {
        sImage = 1;
    } else {
        struct stat st;
        if (stat(root, &st) == 0 && S_ISREG(st.st_mode)) {
            sImage = 1;
            sImageFile = fopen(root, "rb");
        }
    }
    if (sImage) {
        load_image_fst();
    } else {
        char path[1024];
        FILE *f;
        snprintf(path, sizeof path, "%s/files", root);
        add_entry("", path, 0, 1, 0);
        walk(path, 0);
        snprintf(path, sizeof path, "%s/sys/boot.bin", root);
        if ((f = fopen(path, "rb"))) {
            fread(&sDiskID, 1, sizeof sDiskID, f);
            fclose(f);
        }
    }
    fprintf(stderr, "[dvd] %d entries from %s %s (%.6s)\n", sNumEntries, sImage ? "disc image" : "directory",
            sImageFile || !sImage ? root : "(browser file)", sDiskID.gameName);
}

/* main.dol from the disc (for the blobs the original links into the DOL). */
u8 *port_read_main_dol(u32 *outSize)
{
    u8 *buf = NULL;
    DVDInit();
    if (sImage) {
        u8 hdr[0x440], dh[0x100];
        u32 dolOff, size = 0;
        int s;
        image_read(0, sizeof hdr, hdr);
        dolOff = be32(hdr + 0x420);
        image_read(dolOff, sizeof dh, dh);
        for (s = 0; s < 18; s++) {
            u32 end = be32(dh + s * 4) + be32(dh + 0x90 + s * 4);
            if (end > size)
                size = end;
        }
        buf = (u8 *)malloc(size);
        image_read(dolOff, size, buf);
        *outSize = size;
    } else {
        char path[1024];
        FILE *f;
        long n;
        snprintf(path, sizeof path, "%s/sys/main.dol", port_disc_root());
        if (!(f = fopen(path, "rb")))
            return NULL;
        fseek(f, 0, SEEK_END);
        n = ftell(f);
        fseek(f, 0, SEEK_SET);
        buf = (u8 *)malloc((size_t)n);
        fread(buf, 1, (size_t)n, f);
        fclose(f);
        *outSize = (u32)n;
    }
    return buf;
}

static int child_named(int dir, const char *name, int len)
{
    int i = dir + 1;
    while (i < sEntries[dir].next) {
        if ((int)strlen(sEntries[i].name) == len && !strncasecmp(sEntries[i].name, name, len))
            return i;
        i = sEntries[i].isDir ? sEntries[i].next : i + 1;
    }
    return -1;
}

s32 DVDConvertPathToEntrynum(const char *path)
{
    int dir;
    DVDInit();
    dir = (*path == '/') ? 0 : sCurrentDir;
    while (*path) {
        const char *end;
        int len;
        while (*path == '/')
            path++;
        if (!*path)
            break;
        end = strchr(path, '/');
        len = end ? (int)(end - path) : (int)strlen(path);
        if (len == 1 && path[0] == '.') {
        } else if (len == 2 && path[0] == '.' && path[1] == '.') {
            dir = sEntries[dir].parent;
        } else {
            dir = child_named(dir, path, len);
            if (dir < 0)
                return -1;
        }
        path += len;
    }
    return dir;
}

/* Raw bytes of a file as stored on the disc. */
static u32 raw_read(Entry *e, u32 offset, u32 length, void *dst)
{
    if (offset >= e->rawSize)
        return 0;
    if (length > e->rawSize - offset)
        length = e->rawSize - offset;
    if (sImage)
        return image_read(e->discOffset + offset, length, dst);
    {
        FILE *f = fopen(e->hostPath, "rb");
        u32 got;
        if (!f)
            return 0;
        fseek(f, (long)offset, SEEK_SET);
        got = (u32)fread(dst, 1, length, f);
        fclose(f);
        return got;
    }
}

/* Disc formats are big-endian; serve converted (host-order) copies of the
 * formats port/conv knows. Media streams are read piecewise and stay raw. */
static void load_converted(Entry *e)
{
    u8 *raw;
    unsigned outSize;
    if (e->convState == 2 || e->conv)
        return;
    if (!port_conv_wants(e->name)) {
        e->convState = 2;
        return;
    }
    {
        double t0 = port_perf_enabled ? port_perf_now() : 0, t1 = 0;
        raw = (u8 *)malloc(e->rawSize ? e->rawSize : 1);
        raw_read(e, 0, e->rawSize, raw);
        if (port_perf_enabled)
            t1 = port_perf_now();
        e->conv = port_conv_file(e->name, raw, e->rawSize, &outSize);
        free(raw);
        e->size = outSize;
        e->convState = 1;
        if (port_perf_enabled)
            fprintf(stderr, "[perf] dvd: %s %u -> %u bytes: read %.1f ms, convert %.1f ms\n", e->name, e->rawSize, outSize,
                    t1 - t0, port_perf_now() - t1);
    }
}

BOOL DVDFastOpen(s32 entrynum, DVDFileInfo *fi)
{
    DVDInit();
    if (entrynum < 0 || entrynum >= sNumEntries || sEntries[entrynum].isDir)
        return FALSE;
    if (port_perf_enabled) {
        double t0 = port_perf_now();
        load_converted(&sEntries[entrynum]);
        port_perf_add(PERF_DVD, port_perf_now() - t0);
    } else {
        load_converted(&sEntries[entrynum]);
    }
    if (sEntries[entrynum].conv)
        sEntries[entrynum].convOpen++;
    memset(fi, 0, sizeof *fi);
    fi->startAddr = (u32)entrynum; /* host: the entry number stands in for the disc offset */
    fi->length = sEntries[entrynum].size;
    fi->cb.state = DVD_STATE_END;
    return TRUE;
}

BOOL DVDOpen(const char *fileName, DVDFileInfo *fi)
{
    s32 n = DVDConvertPathToEntrynum(fileName);
    if (n < 0) {
        fprintf(stderr, "[dvd] not found: %s\n", fileName);
        return FALSE;
    }
    return DVDFastOpen(n, fi);
}

BOOL DVDClose(DVDFileInfo *fi)
{
    Entry *e;
    if (fi->startAddr >= (u32)sNumEntries)
        return TRUE;
    e = &sEntries[fi->startAddr];
    if (e->conv && --e->convOpen <= 0) {
        free(e->conv);
        e->conv = NULL;
        e->convOpen = 0;
    }
    return TRUE;
}

static s32 read_entry_(DVDFileInfo *fi, void *addr, s32 length, s32 offset);
static s32 read_entry(DVDFileInfo *fi, void *addr, s32 length, s32 offset)
{
    double t0;
    s32 r;
    if (!port_perf_enabled)
        return read_entry_(fi, addr, length, offset);
    t0 = port_perf_now();
    r = read_entry_(fi, addr, length, offset);
    port_perf_add(PERF_DVD, port_perf_now() - t0);
    return r;
}

static s32 read_entry_(DVDFileInfo *fi, void *addr, s32 length, s32 offset)
{
    Entry *e = &sEntries[fi->startAddr];
    u32 got;
    if (e->convState == 1 && !e->conv)
        load_converted(e); /* read after close: reload */
    if (e->conv) {
        got = offset < 0 || (u32)offset >= e->size ? 0 : e->size - (u32)offset;
        if (got > (u32)length)
            got = (u32)length;
        memcpy(addr, e->conv + offset, got);
    } else {
        got = raw_read(e, (u32)offset, (u32)length, addr);
    }
    /* the drive transfers whole 32-byte units; zero what lies past EOF */
    if ((s32)got < length)
        memset((u8 *)addr + got, 0, (size_t)length - got);
    return length;
}

s32 DVDReadPrio(DVDFileInfo *fi, void *addr, s32 length, s32 offset, s32 prio)
{
    (void)prio;
    return read_entry(fi, addr, length, offset);
}

static void read_done(void *a, void *b)
{
    DVDFileInfo *fi = (DVDFileInfo *)a;
    s32 result = (s32)(uintptr_t)b;
    fi->cb.state = result < 0 ? DVD_STATE_FATAL_ERROR : DVD_STATE_END;
    fi->cb.transferredSize = result < 0 ? 0 : (u32)result;
    if (fi->callback)
        fi->callback(result, fi);
}

BOOL DVDReadAsyncPrio(DVDFileInfo *fi, void *addr, s32 length, s32 offset, DVDCallback callback, s32 prio)
{
    s32 r;
    (void)prio;
    fi->callback = callback;
    fi->cb.state = DVD_STATE_BUSY;
    fi->cb.addr = addr;
    fi->cb.length = (u32)length;
    fi->cb.offset = (u32)offset;
    r = read_entry(fi, addr, length, offset);
    port_defer(read_done, fi, (void *)(uintptr_t)r);
    return TRUE;
}

s32 DVDGetCommandBlockStatus(const DVDCommandBlock *block)
{
    port_poll();
    return block->state;
}

s32 DVDGetDriveStatus(void)
{
    port_poll();
    return DVD_STATE_END;
}

int DVDCheckDisk(void) { return 1; }
DVDDiskID *DVDGetCurrentDiskID(void) { return &sDiskID; }

BOOL DVDOpenDir(char *dirName, DVDDir *dir)
{
    s32 n = DVDConvertPathToEntrynum(dirName);
    if (n < 0 || !sEntries[n].isDir)
        return FALSE;
    dir->entryNum = (u32)n;
    dir->location = (u32)n + 1;
    dir->next = (u32)sEntries[n].next;
    return TRUE;
}

BOOL DVDReadDir(DVDDir *dir, DVDDirEntry *dirent)
{
    u32 i = dir->location;
    Entry *e;
    if (i <= dir->entryNum || i >= dir->next)
        return FALSE;
    e = &sEntries[i];
    dirent->entryNum = i;
    dirent->isDir = e->isDir;
    dirent->name = e->name;
    dir->location = e->isDir ? (u32)e->next : i + 1;
    return TRUE;
}

BOOL DVDCloseDir(DVDDir *dir)
{
    (void)dir;
    return TRUE;
}
