#ifndef _DOLPHIN_SISTEERING
#define _DOLPHIN_SISTEERING

#include "dolphin/os.h"
#include "dolphin/sipriv.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*SISteeringCallback)(s32 chan, s32 result);

typedef struct SISteeringControl
{
    u8 output[3];
    u8 input[8];
    u32 outputBytes;
    u32 inputBytes;
    SISteeringCallback callback;
    s32 ret;
    OSThreadQueue threadQueue;
    void (*proc)(s32 chan);
} SISteeringControl;

typedef struct SISteeringStatus
{
    u16 button;
    u8 misc;
    s8 steering;
    u8 gas;
    u8 brake;
    u8 left;
    u8 right;
    s8 err;
} SISteeringStatus;

extern SISteeringControl __SISteering[4];
extern u32 __SISteeringEnableBits;
extern BOOL __SIResetSteering;

void SIInitSteering(void);
s32 SIResetSteeringAsync(s32 chan, SISteeringCallback callback);
s32 SIResetSteering(s32 chan);

s32 SIReadSteering(s32 chan, SISteeringStatus *status);
void (*SISetSteeringSamplingCallback(void (*callback)()))();
void SIControlSteering(s32 chan, u32 control, s32 level);

void __SISteeringEnable(s32 chan);
void __SISteeringDisable(s32 chan);
void __SISteeringSyncCallback(s32 chan, s32 result);
s32 __SISteeringSync(s32 chan);
s32 __SISteeringTransfer(s32 chan, u32 outputBytes, u32 inputBytes, void (*proc)(s32 chan));

#ifdef __cplusplus
}
#endif

#endif // _DOLPHIN_SISTEERING
