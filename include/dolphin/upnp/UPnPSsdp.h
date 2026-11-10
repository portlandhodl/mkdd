#ifndef _DOLPHIN_UPNP_UPNPSSDP_H_
#define _DOLPHIN_UPNP_UPNPSSDP_H_

#include <dolphin/types.h>
#include <dolphin/os.h>
#include <dolphin/ip/IPSocket.h>

typedef struct SSDPEntry {
    u32 mSendCount;   // 0x00
    u32 mState;       // 0x04
    OSTime mTime;     // 0x08
    OSTime mNextTime; // 0x10
    u8 mAddr[8];      // 0x18
} SSDPEntry;

typedef struct SSDPInfo {
    char* mName;             // 0x00
    char* mUuidName;         // 0x04
    OSTime mTime;            // 0x08
    u32 mSendCount;          // 0x10
    u32 mState;              // 0x14
    OSTime mEndTime;         // 0x18
    OSTime mNextTime;        // 0x20
    SSDPEntry mEntries[16];  // 0x28
} SSDPInfo;

typedef struct SSDPConfig {
    OSThread* mThread;   // 0x00
    void* mStack;        // 0x04
    u32 mStackSize;      // 0x08
    u32 mPriority;       // 0x0C
    void (*mCallback)(const char*, const void*); // 0x10
    u8 mTtl;             // 0x14
} SSDPConfig;

typedef struct SSDPSearchInfo {
    const char* mTarget;                                        // 0x00
    void (*mCallback)(const char*, const void*, const void*);   // 0x04
    struct SSDPSearchInfo* mNext;                               // 0x08
    struct SSDPSearchInfo* mPrev;                               // 0x0C
    u32 mSendCount;                                             // 0x10
    u32 mState;                                                 // 0x14
    OSTime mTime;                                               // 0x18
    OSTime mNextTime;                                           // 0x20
} SSDPSearchInfo;

void SSDPInit(SSDPConfig* config);
void SSDPStart(SSDPInfo* info, s32 numInfo);
void SSDPStop(void);
s32 SSDPGetState(void);
void SSDPSearch(SSDPSearchInfo* info);
const char* SSDPGetServerName(void);
s32 SSDPGetCacheControlMaxAge(void* http);

#endif
