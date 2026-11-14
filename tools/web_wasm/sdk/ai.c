/* AI (audio interface): the DMA that streams the mixed 32 kHz stereo buffer to
 * the DAC. Each AIInitDMA'd block is latched when the previous one finishes,
 * the DMA interrupt (AIRegisterDMACallback) fires at every block start, and a
 * finished block goes to the host: a WAV file under node (MKDD_AUDIO_WAV=path)
 * and Module.audioPush in the browser (tools/web_wasm/web/audio.js). Block periods are
 * paced by wall-clock time, like the VI retrace. MKDD_NO_AUDIO=1 only drops the
 * output: the DMA, the audio thread and the DSP keep running, since sounds are
 * only released once the DSP has played them (a stopped audio thread leaves the
 * JAISe/JAISeq pools exhausted).
 *
 * Buffer format, as JASDriver::readDspBuffer writes it: s16 frames of
 * (right, left) in CPU (host) byte order. */
#include <stdint.h>
#include <emscripten.h>
#include "port_sdk.h"

#define AI_RATE 32000

static AIDCallback sDmaCallback;
static u32 sDmaStart, sDmaLength;   /* programmed by AIInitDMA */
static u32 sPlayStart, sPlayLength; /* block being played */
static BOOL sDmaEnabled;
static u32 sDspRate;
static OSAlarm sAlarm;
static OSTime sNextBlock; /* end of the block being played */
static BOOL sDmaFresh;    /* AIInitDMA since the last block started */
static int sMute;         /* MKDD_NO_AUDIO */
static int sDebug;        /* MKDD_DSP_DEBUG: report blocks the game did not refill in time */
static u32 sBlocks, sRepeats, sResyncs;

/* ---- host output ---- */

/* Node: append to a WAV file named by MKDD_AUDIO_WAV, rewriting the header as it
 * grows (and at exit) so the file stays playable. Done in JS on node's fs:
 * no stdio buffering under interrupt context. Returns 0 when not enabled. */
EM_JS(int, port_js_wav_open, (int rate), {
    if (typeof process === 'undefined' || !process.env || !process.env.MKDD_AUDIO_WAV)
        return 0;
    try {
        var fs = require('fs');
        var w = { fs: fs, fd: fs.openSync(process.env.MKDD_AUDIO_WAV, 'w'), bytes: 0, rate: rate };
        w.header = function() {
            var h = Buffer.alloc(44);
            h.write('RIFF', 0); h.writeUInt32LE(36 + w.bytes, 4); h.write('WAVEfmt ', 8);
            h.writeUInt32LE(16, 16); h.writeUInt16LE(1, 20); h.writeUInt16LE(2, 22);
            h.writeUInt32LE(w.rate, 24); h.writeUInt32LE(w.rate * 4, 28); h.writeUInt16LE(4, 32);
            h.writeUInt16LE(16, 34); h.write('data', 36); h.writeUInt32LE(w.bytes, 40);
            fs.writeSync(w.fd, h, 0, 44, 0);
        };
        w.header();
        process.on('exit', function() { try { w.header(); fs.closeSync(w.fd); } catch (e) {} });
        Module.portWav = w;
        console.error('[ai] writing audio to ' + process.env.MKDD_AUDIO_WAV);
        return 1;
    } catch (e) {
        console.error('[ai] cannot write ' + process.env.MKDD_AUDIO_WAV + ': ' + e);
        return 0;
    }
});

EM_JS(void, port_js_wav_write, (const s16 *lr, int frames), {
    var w = Module.portWav;
    var n = frames * 4;
    var b = Buffer.alloc(n);
    for (var i = 0; i < frames * 2; i++) b.writeInt16LE(HEAP16[(lr >> 1) + i], i * 2);
    w.fs.writeSync(w.fd, b, 0, n, 44 + w.bytes);
    w.bytes += n;
    if ((w.bytes / 4) % (w.rate * 2) < frames) w.header();
});

/* Browser: tools/web_wasm/web/audio.js feeds Module.audioPush to WebAudio. */
EM_JS(void, port_js_audio_push, (const s16 *lr, int frames), {
    if (typeof Module !== 'undefined' && Module.audioPush)
        Module.audioPush(HEAP16.subarray(lr >> 1, (lr >> 1) + frames * 2));
});

/* MKDD_DSP_DEBUG>=4 with MKDD_DSP_DUMP=path: raw DSP output, before the AI */
EM_JS(void, port_dsp_dbg_dump, (const s16 *l, const s16 *r, int n), {
    if (typeof process === 'undefined' || !process.env.MKDD_DSP_DUMP) return;
    var fs = require('fs');
    if (!Module.dspDumpFd) Module.dspDumpFd = fs.openSync(process.env.MKDD_DSP_DUMP, 'w');
    var b = Buffer.alloc(n * 4);
    for (var i = 0; i < n; i++) { b.writeInt16LE(HEAP16[(l >> 1) + i], i * 4); b.writeInt16LE(HEAP16[(r >> 1) + i], i * 4 + 2); }
    fs.writeSync(Module.dspDumpFd, b);
});

static int sWav = -1;

static void output_block(const s16 *rl, u32 frames)
{
    static s16 lr[4096 * 2];
    u32 i;
    if (!rl || !frames || sMute)
        return;
    if (frames > 4096)
        frames = 4096;
    for (i = 0; i < frames; i++) {
        lr[i * 2] = rl[i * 2 + 1];
        lr[i * 2 + 1] = rl[i * 2];
    }
    if (sWav < 0)
        sWav = port_js_wav_open(AI_RATE);
    if (sWav)
        port_js_wav_write(lr, (int)frames);
    port_js_audio_push(lr, (int)frames);
}

/* ---- DMA ---- */

static OSTime now_ticks(void) { return (OSTime)(emscripten_get_now() * (PORT_TICKS_PER_SEC / 1000)); }

static OSTime block_ticks(u32 length)
{
    OSTime t = (OSTime)(length / 4) * PORT_TICKS_PER_SEC / AI_RATE;
    return t > 0 ? t : PORT_TICKS_PER_SEC / 1000;
}

static void block_start(void)
{
    sBlocks++;
    if (!sDmaFresh)
        sRepeats++;
    sDmaFresh = FALSE;
    if (sDebug && sBlocks % 2000 == 0) {
        OSReport("[ai] %u blocks: %u replayed (audio thread late), %u host stalls\n", sBlocks, sRepeats, sResyncs);
        sRepeats = sResyncs = 0;
    }
    sPlayStart = sDmaStart;
    sPlayLength = sDmaLength;
    sNextBlock += block_ticks(sPlayLength);
    if (sDmaCallback)
        sDmaCallback();
}

static void dma_alarm(OSAlarm *alarm, OSContext *context)
{
    OSTime now = now_ticks();
    (void)alarm;
    (void)context;
    if (!sDmaEnabled)
        return;
    output_block((const s16 *)(uintptr_t)sPlayStart, sPlayLength / 4);
    if (now - sNextBlock > PORT_TICKS_PER_SEC / 10) {
        sNextBlock = now; /* the host stalled: resynchronise instead of bursting */
        sResyncs++;
    }
    block_start();
    {
        OSTime wait = sNextBlock - now_ticks();
        OSSetAlarm(&sAlarm, wait > 0 ? wait : 0, dma_alarm);
        sAlarm.fire = sNextBlock;
    }
}

static void dma_first(void *a, void *b)
{
    (void)a;
    (void)b;
    if (!sDmaEnabled)
        return;
    sNextBlock = now_ticks();
    block_start();
    OSSetAlarm(&sAlarm, block_ticks(sPlayLength), dma_alarm);
    sAlarm.fire = sNextBlock;
}

void AIInit(u8 *stack)
{
    (void)stack;
    OSCreateAlarm(&sAlarm);
}

BOOL AICheckInit(void) { return TRUE; }
void AIReset(void) {}

void AIInitDMA(u32 start_addr, u32 length)
{
    sDmaStart = start_addr;
    sDmaLength = length;
    sDmaFresh = TRUE;
}

AIDCallback AIRegisterDMACallback(AIDCallback callback)
{
    AIDCallback old = sDmaCallback;
    sDmaCallback = callback;
    return old;
}

void AIStartDMA(void)
{
    if (sDmaEnabled)
        return;
    sMute = port_env("MKDD_NO_AUDIO");
    sDmaEnabled = TRUE;
    sDebug = port_env("MKDD_DSP_DEBUG");
    port_defer(dma_first, NULL, NULL); /* the first block starts (and interrupts) at once */
}

void AIStopDMA(void)
{
    sDmaEnabled = FALSE;
    OSCancelAlarm(&sAlarm);
}

BOOL AIGetDMAEnableFlag(void) { return sDmaEnabled; }
u32 AIGetDMAStartAddr(void) { return sDmaStart; }
u32 AIGetDMALength(void) { return sDmaLength; }

u32 AIGetDMABytesLeft(void)
{
    OSTime total, left;
    if (!sDmaEnabled)
        return 0;
    total = block_ticks(sPlayLength);
    left = sNextBlock - now_ticks();
    if (left <= 0)
        return 0;
    if (left > total)
        left = total;
    return (u32)((u64)sPlayLength * (u64)left / (u64)total) & ~31u;
}

void AISetDSPSampleRate(u32 rate) { sDspRate = rate; }
u32 AIGetDSPSampleRate(void) { return sDspRate; }
