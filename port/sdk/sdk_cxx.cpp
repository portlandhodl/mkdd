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
// THP (movies)
BOOL THPPlayerInit() { MKDD_STUB("THPPlayerInit"); return FALSE; }
void THPPlayerQuit() {}
BOOL THPPlayerOpen(const char *, BOOL) { return FALSE; }
BOOL THPPlayerClose() { return FALSE; }
BOOL THPPlayerPlay() { return FALSE; }
void THPPlayerStop() {}
BOOL THPPlayerPause() { return FALSE; }
BOOL THPPlayerPrepare(s32, u8, s32) { return FALSE; }
BOOL THPPlayerSetBuffer(u8 *) { return FALSE; }
u32 THPPlayerCalcNeedMemory() { return 32; }
BOOL THPPlayerGetVideoInfo(THPVideoInfo *) { return FALSE; }
BOOL THPPlayerSetVolume(s32, s32) { return FALSE; }
s32 THPPlayerDrawCurrentFrame(GXRenderModeObj *, u32, u32, u32, u32) { return -1; }
u32 THPPlayerGetTotalFrame() { return 0; }
s32 THPPlayerGetState() { return 0; }
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
