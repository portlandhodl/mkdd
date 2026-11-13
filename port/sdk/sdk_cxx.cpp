// SDK libraries whose only declarations live in decomp C++ headers or units:
// THP movie player, Logitech wheel, screenshot service, sockets/UPnP.
// No host implementation yet: report "not available" so the game takes its fallback path.
#include "types.h"
extern "C" void port_stub(const char *name);
#define MKDD_STUB(name) port_stub(name)
#include "Kaneshige/THP/THPPlayer.h"
#include "JSystem/Logitech/LGForceEffect.h"
#include "Osako/screenshot.h"
#include "Osako/NetGameMgr.h"
#include "Osako/BBAMgr.h"
#include <string.h>

extern "C" {
// THP (movies): no decoder yet. The player opens the file to learn its frame
// count and "plays" blank frames at the display rate, so movie scenes run their
// natural length and end the way they do on hardware.
static struct {
    BOOL open, playing;
    u32 totalFrames;
    s32 frame;
    u32 lastRetrace;
} sThp;
}
#include <dolphin/dvd.h>
#include <dolphin/vi.h>
extern "C" {

BOOL THPPlayerInit() { memset(&sThp, 0, sizeof sThp); return TRUE; }
void THPPlayerQuit() { sThp.open = FALSE; }

BOOL THPPlayerOpen(const char *fileName, BOOL)
{
    DVDFileInfo fi;
    u8 hdr[0x40] __attribute__((aligned(32)));
    if (!DVDOpen(fileName, &fi))
        return FALSE;
    DVDReadPrio(&fi, hdr, sizeof hdr, 0, 2);
    DVDClose(&fi);
    if (memcmp(hdr, "THP", 3) != 0)
        return FALSE;
    sThp.totalFrames = (u32)hdr[0x14] << 24 | (u32)hdr[0x15] << 16 | (u32)hdr[0x16] << 8 | hdr[0x17];
    sThp.open = TRUE;
    sThp.frame = -1;
    MKDD_STUB("THPPlayerOpen (no decoder: blank frames)");
    return TRUE;
}

BOOL THPPlayerClose() { BOOL o = sThp.open; sThp.open = FALSE; return o; }
BOOL THPPlayerPlay() { sThp.playing = sThp.open; sThp.lastRetrace = VIGetRetraceCount(); return sThp.open; }
void THPPlayerStop() { sThp.playing = FALSE; sThp.frame = -1; }
BOOL THPPlayerPause() { sThp.playing = FALSE; return sThp.open; }
BOOL THPPlayerPrepare(s32 offset, u8, s32) { sThp.frame = offset - 1; return sThp.open; }
BOOL THPPlayerSetBuffer(u8 *) { return sThp.open; }
u32 THPPlayerCalcNeedMemory() { return 32; }

BOOL THPPlayerGetVideoInfo(THPVideoInfo *info)
{
    info->xSize = 640;
    info->ySize = 448;
    info->videoType = 0;
    return sThp.open;
}

BOOL THPPlayerSetVolume(s32, s32) { return sThp.open; }

// advance at 29.97 fps (every second retrace) while playing; returns the frame shown
s32 THPPlayerDrawCurrentFrame(GXRenderModeObj *, u32, u32, u32, u32)
{
    if (!sThp.open)
        return -1;
    if (sThp.playing) {
        u32 now = VIGetRetraceCount();
        while (now - sThp.lastRetrace >= 2 && sThp.frame + 1 < (s32)sThp.totalFrames) {
            sThp.frame++;
            sThp.lastRetrace += 2;
        }
    }
    return sThp.frame;
}

u32 THPPlayerGetTotalFrame() { return sThp.totalFrames; }
s32 THPPlayerGetState() { return sThp.playing ? 2 : 1; }
void THPPlayerPostDrawDone() {}

// Logitech wheel: no device
void LGInit() {}
s32 LGOpen(s32, u32 *) { return -1; }
s32 LGRead(struct LGPosition *) { return -1; }
s32 LGDownloadForceEffect(u32, u32 *, LGForceEffect *) { return -1; }
s32 LGUpdateForceEffect(u32, LGForceEffect *) { return -1; }
s32 LGStartForceEffect(u32) { return -1; }
s32 LGStopForceEffect(u32) { return -1; }
s32 LGDestroyForceEffect(u32) { return -1; }

void SCREENSHOTService(void *, SCREENSHOTAllocFunc, SCREENSHOTFreeFunc) {}

// network (broadband adapter): signatures as BBAMgr.cpp / NetGameMgr.cpp declare them
void SOInit() {}
s32 SOStartup(SOConfig *) { return -1; }
s32 SOCleanup() { return 0; }
char *SOInetNtoA(SOInAddr) { static char s[] = "0.0.0.0"; return s; }
s32 SSDPStop() { return 0; }
void UPnPCreateUuidFromName(void *, const void *, const void *, s32) {}
char *UPnPGetUuidString(void *, char *buf) { if (buf) buf[0] = 0; return buf; }
int UPnPScanUuid(const char *, UPnPUuid *) { return 0; }
s32 IPGetLinkState(s32, s32 *state) { if (state) *state = 0; return -1; }
s32 IPGetConfigError(s32) { return 0; }
void IPClearConfigError(s32) {}
void IPGetMacAddr(s32, void *mac) { memset(mac, 0, 6); }
u8 UPnPUdnNameSpace[16];
}
