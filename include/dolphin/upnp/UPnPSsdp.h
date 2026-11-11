#ifndef _DOLPHIN_UPNP_UPNPSSDP_H_
#define _DOLPHIN_UPNP_UPNPSSDP_H_

#include <dolphin/types.h>
#include <dolphin/os.h>
#include <dolphin/ip/IPSocket.h>

typedef struct SSDPTimer {
    s32 mCount;     // 0x00
    s32 mState;     // 0x04
    OSTime mStart;  // 0x08
    OSTime mNext;   // 0x10
} SSDPTimer;

typedef struct SSDPEntry {
    SSDPTimer mTimer; // 0x00
    u8 mAddr[8];      // 0x18
} SSDPEntry;

typedef struct SSDPInfo {
    char* mName;             // 0x00
    char* mUuidName;         // 0x04
    OSTime mTime;            // 0x08
    SSDPTimer mTimer;        // 0x10
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

typedef struct SSDPStat {
    u32 mNotify;      // 0x00
    u32 mResponse;    // 0x04
    u32 mSearch;      // 0x08
    u32 mExpire;      // 0x0C
    u32 mNotifySent;  // 0x10
    u32 mOkSent;      // 0x14
    u32 mSearchSent;  // 0x18
} SSDPStat;

typedef struct SSDPSearchInfo {
    const char* mTarget;                                        // 0x00
    void (*mCallback)(const char*, const void*, const void*);   // 0x04
    struct SSDPSearchInfo* mNext;                               // 0x08
    struct SSDPSearchInfo* mPrev;                               // 0x0C
    SSDPTimer mTimer;                                           // 0x10
} SSDPSearchInfo;

void SSDPInit(SSDPConfig* config);
void SSDPStart(SSDPInfo* info, s32 numInfo);
void SSDPStop(void);
s32 SSDPGetState(void);
void SSDPSearch(SSDPSearchInfo* info);
const char* SSDPGetServerName(void);
s32 SSDPGetCacheControlMaxAge(char* http);
void SSDPGetStat(SSDPStat* stat);
void SSDPClearStat(void);

#endif
