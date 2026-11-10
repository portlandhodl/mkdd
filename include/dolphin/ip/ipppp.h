#ifndef _DOLPHIN_IP_PPP_H_
#define _DOLPHIN_IP_PPP_H_

#include <dolphin/os.h>
#include <dolphin/os/OSAlarm.h>
#include <dolphin/types.h>

typedef struct PPPAlarm {
    void* handler;
    u32 tag;
    s64 fire;
    struct PPPAlarm* prev;
    struct PPPAlarm* next;
    s64 period;
} PPPAlarm;

typedef struct PPPPPP {
    u16 protocol;
    u16 unk02;
    s32 state;
    u8 restartCount;
    u8 unk09;
    u8 codeRejectCount;
    u8 unk0B;
    s32 restartCount0;
    s32 restartCount1;
    s32 restartCount2;
    s32 restartCount3;
    u8 opt[0x14];
    u16 optLen;
    u16 unk32;
    u32 unk34;
    PPPAlarm alarm;
    struct PPPPPP* head;
    struct PPPPPP* tail;
    void* ifp;
    s32 (*receiveConfigureRequest)(struct PPPPPP*, u8*);
    s32 (*receiveConfigureAck)(struct PPPPPP*, u8*);
    s32 (*receiveConfigureNak)(struct PPPPPP*, u8*);
    s32 (*receiveConfigureReject)(struct PPPPPP*, u8*);
    s32 (*up)(struct PPPPPP*);
    s32 (*down)(struct PPPPPP*);
    s32 (*started)(struct PPPPPP*);
    s32 (*finished)(struct PPPPPP*);
    void (*setState)(struct PPPPPP*);
    struct PPPPPP* next;
    struct PPPPPP* prev;
    u16 mru;
    u16 unk92;
    u32 unk94;
    u32 ipAddr;
    u32 unk9C;
} PPPPPP;

typedef struct PPPOpt {
    u8 code;
    u8 len;
    u8 data[];
} PPPOpt;

typedef struct PPPAuth {
    u8 userLen;
    u8 passLen;
    char user[256];
    char pass[256];
} PPPAuth;

extern PPPAuth __PPPAuth;

s32 PPPLayerUp(PPPPPP* ppp);
void PPPLayerDown(PPPPPP* ppp);
s32 PPPLayerStarted(PPPPPP* ppp);
s32 PPPLayerFinished(PPPPPP* ppp);
void PPPInitializeRestartCount(PPPPPP* ppp);
void PPPSetState(PPPPPP* ppp, s32 state);
s32 PPPGetState(PPPPPP* ppp);
void PPPIn(PPPPPP* ifp, u8* data, s32 len);
s32 PPPOpen(PPPPPP* ppp);
s32 PPPUp(PPPPPP* ppp);
void PPPDown(PPPPPP* ppp);
void PPPClose(PPPPPP* ppp);
void PPPTimeout(OSAlarm* alarm, OSContext* context);
s32 PPPDeleteOpt(u8* ppp, s32 offset, PPPOpt* opt);
s32 PPPInsertOpt(u8* ppp, s32 offset, u8* opt, PPPOpt* newOpt);
s32 PPPInit(PPPPPP* ifp, PPPPPP* lcp, PPPPPP* ipcp, const char* user,
            const char* pass);

#endif
