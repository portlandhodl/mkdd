#include <dolphin/ip/ipppp.h>
#include <dolphin/ip/ip.h>
#include <string.h>

extern void PPPInitLCP(PPPPPP* lcp, void* ifp);
extern void PPPInitIPCP(PPPPPP* ipcp, void* ifp);
extern void IPSetConfigError(void* unused, s32 error);
extern s32 IPIn(void* ifp, void* data, s32 len, s32 flag);
static s32 LCPOut(PPPPPP* ppp, u8 code, u8 id, s32 len, void* data);
static void LCPIn(PPPPPP* ppp, u8* data, s32 len, s32 flag);
extern void PAPIn(PPPPPP* ppp, u8* data, s32 len, s32 flag);
extern s32 PAPOpen(PPPPPP* ppp);
extern s32 PAPUp(PPPPPP* ppp);
extern void PAPDown(PPPPPP* ppp);
extern void PAPClose(PPPPPP* ppp);
extern s32 PAPTimeout(PPPPPP* ppp);
extern void CHAPIn(PPPPPP* ppp, u8* data, s32 len, s32 flag);
extern s32 CHAPOpen(PPPPPP* ppp);
extern s32 CHAPUp(PPPPPP* ppp);
extern void CHAPDown(PPPPPP* ppp);
extern void CHAPClose(PPPPPP* ppp);
extern s32 CHAPTimeout(PPPPPP* ppp);

PPPAuth __PPPAuth;

s32 PPPLayerUp(PPPPPP* ppp)
{
    s32 result = ppp->up(ppp);

    if (result) {
        PPPPPP* next = ppp->next;

        if (next)
            PPPUp(next);
    }
    return result;
}

void PPPLayerDown(PPPPPP* ppp)
{
    PPPPPP* next = ppp->next;

    if (next)
        PPPDown(next);
    ppp->down(ppp);
}

s32 PPPLayerStarted(PPPPPP* ppp)
{
    s32 result = ppp->started(ppp);

    if (result) {
        PPPPPP* prev = ppp->prev;

        if (prev)
            PPPOpen(prev);
    }
    return result;
}

s32 PPPLayerFinished(PPPPPP* ppp)
{
    s32 result = ppp->finished(ppp);

    if (result) {
        PPPPPP* prev = ppp->prev;

        if (prev)
            PPPClose(prev);
    }
    return result;
}

void PPPInitializeRestartCount(PPPPPP* ppp)
{
    ppp->restartCount0 = 0;
    ppp->restartCount1 = 10;
    ppp->restartCount2 = 2;
    ppp->restartCount3 = 5;
    ppp->restartCount++;
    OSCancelAlarm((OSAlarm*)&ppp->alarm);
}

void PPPSetState(PPPPPP* ppp, s32 state)
{
    ppp->state = state;
    if (ppp->setState)
        ppp->setState(ppp);
    switch (ppp->state) {
    case 0:
    case 1:
    case 2:
    case 9:
        OSCancelAlarm((OSAlarm*)&ppp->alarm);
        break;
    }
}

s32 PPPGetState(PPPPPP* ppp)
{
    return ppp->state;
}

typedef struct PPPInterface {
    u8 unk00[0x40];
    s32 mtu;
    u8 unk44[0x60 - 0x44];
    void (*out)(struct PPPInterface* ifp, void* packet);
    u8 unk64[4];
    void* (*alloc)(struct PPPInterface* ifp, s32 size);
} PPPInterface;

typedef struct PPPPacket {
    u32 unk00;
    u32 unk04;
    u8 unk08[8];
    u16 type;
    u8 unk12[0x25 - 0x12];
    u8 unk25;
    u8 unk26[2];
    u32 unk28;
    u32 unk2C;
    s32 count;
    void* data;
    s32 len;
    u8 buf[1];
} PPPPacket;

typedef struct PPPHeader {
    u8 code;
    u8 id;
    u16 len;
    u8 data[1];
} PPPHeader;

#define PPP_SET_STATE(ppp, s)                                                  \
    do {                                                                       \
        (ppp)->state = (s);                                                    \
        if ((ppp)->setState)                                                   \
            (ppp)->setState(ppp);                                              \
        switch ((ppp)->state) {                                                \
        case 0:                                                                \
        case 1:                                                                \
        case 2:                                                                \
        case 9:                                                                \
            OSCancelAlarm((OSAlarm*)&(ppp)->alarm);                            \
            break;                                                             \
        }                                                                      \
    } while (0)

#define PPP_IRC(ppp)                                                           \
    do {                                                                       \
        (ppp)->restartCount0 = 0;                                              \
        (ppp)->restartCount1 = 10;                                             \
        (ppp)->restartCount2 = 2;                                              \
        (ppp)->restartCount3 = 5;                                              \
        (ppp)->restartCount++;                                                 \
        OSCancelAlarm((OSAlarm*)&(ppp)->alarm);                                \
    } while (0)

#define PPP_ZRC(ppp)                                                           \
    do {                                                                       \
        (ppp)->restartCount0 = 0;                                              \
        (ppp)->restartCount1 = 0;                                              \
        (ppp)->restartCount2 = 0;                                              \
        (ppp)->restartCount3 = 0;                                              \
        OSCancelAlarm((OSAlarm*)&(ppp)->alarm);                                \
    } while (0)

#define PPP_SCR(ppp)                                                           \
    do {                                                                       \
        if ((ppp)->restartCount1 > 0) {                                        \
            (ppp)->restartCount1--;                                            \
            LCPOut(ppp, 1, (ppp)->restartCount, (ppp)->optLen, (ppp)->opt);    \
        }                                                                      \
    } while (0)

#define PPP_SCA(ppp, hdr)                                                      \
    do {                                                                       \
        (ppp)->restartCount3 = 5;                                              \
        LCPOut(ppp, 2, (hdr)->id, (hdr)->len - 4, (hdr)->data);                \
    } while (0)

#define PPP_SCJ(ppp, hdr) LCPOut(ppp, 4, (hdr)->id, (hdr)->len - 4, (hdr)->data)

#define PPP_SCN(ppp, hdr)                                                      \
    do {                                                                       \
        (ppp)->restartCount3--;                                                \
        LCPOut(ppp, 3, (hdr)->id, (hdr)->len - 4, (hdr)->data);                \
    } while (0)

#define PPP_STA(ppp) LCPOut(ppp, 6, (ppp)->unk09, 0, NULL)

#define PPP_TLU(ppp)                                                           \
    do {                                                                       \
        if ((ppp)->up(ppp) && (ppp)->next)                                     \
            PPPUp((ppp)->next);                                                \
    } while (0)

#define PPP_TLD(ppp)                                                           \
    do {                                                                       \
        if ((ppp)->next)                                                       \
            PPPDown((ppp)->next);                                              \
        (ppp)->down(ppp);                                                      \
    } while (0)

#define PPP_TLF(ppp)                                                           \
    do {                                                                       \
        if ((ppp)->finished(ppp) && (ppp)->prev)                               \
            PPPClose((ppp)->prev);                                             \
    } while (0)

inline void SetStateInline(PPPPPP* ppp, s32 state)
{
    PPP_SET_STATE(ppp, state);
}

inline void IRCInline(PPPPPP* ppp)
{
    PPP_IRC(ppp);
}

inline s32 TLFInline(PPPPPP* ppp)
{
    s32 result = ppp->finished(ppp);

    if (result) {
        PPPPPP* prev = ppp->prev;

        if (prev)
            PPPClose(prev);
    }
    return result;
}

static s32 LCPOut(PPPPPP* ppp, u8 code, u8 id, s32 len, void* data)
{
    PPPInterface* ifp;
    BOOL enabled;
    PPPPacket* pkt;
    u8* p;
    PPPHeader* hdr;

    enabled = OSDisableInterrupts();
    ifp = ppp->ifp;
    if (code == 1 || code == 5) {
        OSCancelAlarm((OSAlarm*)&ppp->alarm);
        OSSetAlarm((OSAlarm*)&ppp->alarm, OSSecondsToTicks((OSTime)3), PPPTimeout);
    }
    if (ifp->mtu < len + 4) {
        len = ifp->mtu - 4;
    }
    pkt = ifp->alloc(ifp, len + 66);
    if (pkt) {
        p = pkt->buf;
        *(u16*)p = ppp->protocol;
        hdr = (PPPHeader*)(p + 2);
        hdr->code = code;
        hdr->id = id;
        hdr->len = len + 4;
        memmove(hdr->data, data, len);
        pkt->count = 1;
        pkt->data = p;
        pkt->len = len + 6;
        pkt->unk28 = 0;
        pkt->unk2C = 0;
        pkt->type = 0x8864;
        pkt->unk25 = 0;
        pkt->unk04 = 0;
        ifp->out(ifp, pkt);
    }
    return OSRestoreInterrupts(enabled);
}

static void ReceiveConfigureRequest(PPPPPP* ppp, PPPHeader* hdr)
{
    s32 result;

    switch (ppp->receiveConfigureRequest(ppp, (u8*)hdr)) {
    case 0:
        switch (ppp->state) {
        case 2:
            PPP_STA(ppp);
            break;
        case 3:
            PPP_IRC(ppp);
            PPP_SCR(ppp);
            PPP_SCA(ppp, hdr);
            PPP_SET_STATE(ppp, 8);
            break;
        case 6:
            PPP_SCA(ppp, hdr);
            PPP_SET_STATE(ppp, 8);
            break;
        case 7:
            PPP_SCA(ppp, hdr);
            PPP_SET_STATE(ppp, 9);
            PPP_TLU(ppp);
            break;
        case 8:
            PPP_SCA(ppp, hdr);
            break;
        case 9:
            PPP_SCR(ppp);
            PPP_SCA(ppp, hdr);
            PPP_SET_STATE(ppp, 8);
            PPP_TLD(ppp);
            break;
        }
        break;
    case -1:
        switch (ppp->state) {
        case 2:
            PPP_STA(ppp);
            break;
        case 3:
            PPP_IRC(ppp);
            PPP_SCR(ppp);
            PPP_SCJ(ppp, hdr);
            PPP_SET_STATE(ppp, 6);
            break;
        case 6:
            PPP_SCJ(ppp, hdr);
            break;
        case 7:
            PPP_SCJ(ppp, hdr);
            break;
        case 8:
            PPP_SCJ(ppp, hdr);
            PPP_SET_STATE(ppp, 6);
            break;
        case 9:
            PPP_SCR(ppp);
            PPP_SCJ(ppp, hdr);
            PPP_SET_STATE(ppp, 6);
            PPP_TLD(ppp);
            break;
        }
        break;
    case -2:
        switch (ppp->state) {
        case 2:
            PPP_STA(ppp);
            break;
        case 3:
            PPP_IRC(ppp);
            PPP_SCR(ppp);
            PPP_SCN(ppp, hdr);
            PPP_SET_STATE(ppp, 6);
            break;
        case 6:
            PPP_SCN(ppp, hdr);
            break;
        case 7:
            PPP_SCN(ppp, hdr);
            break;
        case 8:
            PPP_SCN(ppp, hdr);
            PPP_SET_STATE(ppp, 6);
            break;
        case 9:
            PPP_SCR(ppp);
            PPP_SCN(ppp, hdr);
            PPP_SET_STATE(ppp, 6);
            PPP_TLD(ppp);
            break;
        }
        break;
    case -3:
        break;
    }
}

static void ReceiveConfigureAck(PPPPPP* ppp, PPPHeader* hdr)
{
    if (!ppp->receiveConfigureAck(ppp, (u8*)hdr)) {
        return;
    }
    switch (ppp->state) {
    case 2:
        PPP_STA(ppp);
        break;
    case 3:
        PPP_STA(ppp);
        break;
    case 6:
        PPP_IRC(ppp);
        PPP_SET_STATE(ppp, 7);
        break;
    case 7:
        PPP_SCR(ppp);
        break;
    case 8:
        PPP_IRC(ppp);
        PPP_SET_STATE(ppp, 9);
        PPP_TLU(ppp);
        break;
    case 9:
        PPP_SCR(ppp);
        PPP_SET_STATE(ppp, 6);
        PPP_TLD(ppp);
        break;
    }
}

static void ReceiveConfigureNak(PPPPPP* ppp, PPPHeader* hdr)
{
    if (!ppp->receiveConfigureNak(ppp, (u8*)hdr)) {
        return;
    }
    switch (ppp->state) {
    case 2:
        PPP_STA(ppp);
        break;
    case 3:
        PPP_STA(ppp);
        break;
    case 6:
        PPP_IRC(ppp);
        PPP_SCR(ppp);
        break;
    case 7:
        PPP_SCR(ppp);
        break;
    case 8:
        PPP_IRC(ppp);
        PPP_SCR(ppp);
        break;
    case 9:
        PPP_SCR(ppp);
        PPP_SET_STATE(ppp, 6);
        PPP_TLD(ppp);
        break;
    }
}

static void ReceiveConfigureReject(PPPPPP* ppp, PPPHeader* hdr)
{
    if (!ppp->receiveConfigureReject(ppp, (u8*)hdr)) {
        return;
    }
    switch (ppp->state) {
    case 2:
        PPP_STA(ppp);
        break;
    case 3:
        PPP_STA(ppp);
        break;
    case 6:
        PPP_IRC(ppp);
        PPP_SCR(ppp);
        break;
    case 7:
        PPP_SCR(ppp);
        break;
    case 8:
        PPP_IRC(ppp);
        PPP_SCR(ppp);
        break;
    case 9:
        PPP_SCR(ppp);
        PPP_SET_STATE(ppp, 6);
        PPP_TLD(ppp);
        break;
    }
}

static void ReceiveTerminateRequest(PPPPPP* ppp, PPPHeader* hdr)
{
    ppp->unk09 = hdr->id;
    switch (ppp->state) {
    case 2:
        PPP_STA(ppp);
        break;
    case 3:
        PPP_STA(ppp);
        break;
    case 4:
        PPP_STA(ppp);
        break;
    case 5:
        PPP_STA(ppp);
        break;
    case 6:
        PPP_STA(ppp);
        break;
    case 7:
        PPP_STA(ppp);
        PPP_SET_STATE(ppp, 6);
        break;
    case 8:
        PPP_STA(ppp);
        PPP_SET_STATE(ppp, 6);
        break;
    case 9:
        PPP_ZRC(ppp);
        PPP_STA(ppp);
        PPP_SET_STATE(ppp, 5);
        PPP_TLD(ppp);
        break;
    }
}

static void ReceiveTerminateAck(PPPPPP* ppp, PPPHeader* hdr)
{
    switch (ppp->state) {
    case 4:
        PPP_SET_STATE(ppp, 2);
        PPP_TLF(ppp);
        break;
    case 5:
        PPP_SET_STATE(ppp, 3);
        PPP_TLF(ppp);
        break;
    case 7:
        PPP_SET_STATE(ppp, 6);
        break;
    case 9:
        PPP_SCR(ppp);
        PPP_SET_STATE(ppp, 6);
        PPP_TLD(ppp);
        break;
    }
}

static void ReceiveUnknownCode(PPPPPP* ppp, PPPHeader* hdr)
{
    switch (ppp->state) {
    case 2:
        ppp->codeRejectCount++;
        LCPOut(ppp, 7, ppp->codeRejectCount, hdr->len, hdr);
        break;
    case 3:
        ppp->codeRejectCount++;
        LCPOut(ppp, 7, ppp->codeRejectCount, hdr->len, hdr);
        break;
    case 4:
        ppp->codeRejectCount++;
        LCPOut(ppp, 7, ppp->codeRejectCount, hdr->len, hdr);
        break;
    case 5:
        ppp->codeRejectCount++;
        LCPOut(ppp, 7, ppp->codeRejectCount, hdr->len, hdr);
        break;
    case 6:
        ppp->codeRejectCount++;
        LCPOut(ppp, 7, ppp->codeRejectCount, hdr->len, hdr);
        break;
    case 7:
        ppp->codeRejectCount++;
        LCPOut(ppp, 7, ppp->codeRejectCount, hdr->len, hdr);
        break;
    case 8:
        ppp->codeRejectCount++;
        LCPOut(ppp, 7, ppp->codeRejectCount, hdr->len, hdr);
        break;
    case 9:
        ppp->codeRejectCount++;
        LCPOut(ppp, 7, ppp->codeRejectCount, hdr->len, hdr);
        break;
    }
}

static void ReceiveCodeReject(PPPPPP* ppp, PPPHeader* hdr)
{
    u8* rejected;

    switch (ppp->state) {
    case 2:
        PPP_TLF(ppp);
        break;
    case 3:
        PPP_TLF(ppp);
        break;
    case 4:
        PPP_SET_STATE(ppp, 2);
        PPP_TLF(ppp);
        break;
    case 5:
        PPP_SET_STATE(ppp, 3);
        PPP_TLF(ppp);
        break;
    case 6:
        PPP_SET_STATE(ppp, 3);
        PPP_TLF(ppp);
        break;
    case 7:
        PPP_SET_STATE(ppp, 3);
        PPP_TLF(ppp);
        break;
    case 8:
        PPP_SET_STATE(ppp, 3);
        PPP_TLF(ppp);
        break;
    case 9:
        PPP_IRC(ppp);
        PPP_SET_STATE(ppp, 5);
        PPP_TLD(ppp);
        break;
    }
}

static void ReceiveProtocolReject(PPPPPP* ppp, PPPHeader* hdr)
{
    u16 protocol;

    switch (ppp->state) {
    case 2:
        TLFInline(ppp);
        break;
    case 3:
        TLFInline(ppp);
        break;
    case 4:
        SetStateInline(ppp, 2);
        TLFInline(ppp);
        break;
    case 5:
        SetStateInline(ppp, 3);
        TLFInline(ppp);
        break;
    case 6:
        SetStateInline(ppp, 3);
        TLFInline(ppp);
        break;
    case 7:
        SetStateInline(ppp, 3);
        TLFInline(ppp);
        break;
    case 8:
        SetStateInline(ppp, 3);
        TLFInline(ppp);
        break;
    case 9:
        IRCInline(ppp);
        SetStateInline(ppp, 5);
        PPP_TLD(ppp);
        break;
    }
}

static void LCPIn(PPPPPP* ppp, u8* data, s32 len, s32 flag)
{
    PPPHeader* hdr = (PPPHeader*)data;

    if (ppp == NULL || len < 4 || len < hdr->len || hdr->len < 4) {
        return;
    }
    switch (hdr->code) {
    case 1:
        ReceiveConfigureRequest(ppp, hdr);
        break;
    case 2:
        if (hdr->id == ppp->restartCount)
            ReceiveConfigureAck(ppp, hdr);
        break;
    case 3:
        if (hdr->id == ppp->restartCount)
            ReceiveConfigureNak(ppp, hdr);
        break;
    case 4:
        if (hdr->id == ppp->restartCount)
            ReceiveConfigureReject(ppp, hdr);
        break;
    case 5:
        ReceiveTerminateRequest(ppp, hdr);
        break;
    case 6:
        if (hdr->id == ppp->restartCount)
            ReceiveTerminateAck(ppp, hdr);
        break;
    case 7:
        ReceiveCodeReject(ppp, hdr);
        break;
    case 8:
        ReceiveProtocolReject(ppp, hdr);
        break;
    case 9:
        ppp->unk0B = hdr->id;
        switch (ppp->state) {
        case 9:
            LCPOut(ppp, 10, ppp->unk0B, hdr->len - 4, hdr->data);
            break;
        }
        break;
    case 10:
    case 11:
        break;
    default:
        ReceiveUnknownCode(ppp, hdr);
        break;
    }
}

void PPPIn(PPPPPP* ifp, u8* data, s32 len, s32 flag)
{
    u16 protocol;
    PPPPPP* head;
    PPPPPP* ppp;
    PPPPPP* next;

    if (len < 2) {
        return;
    }
    protocol = *(u16*)data;
    if (protocol == 0x21) {
        if (ifp->tail->state == 9) {
            IPIn(ifp, data + 2, len - 2, flag);
        }
        return;
    }
    head = ifp->head;
    for (ppp = head, next = !ppp ? NULL : ppp->next; ppp; ppp = next, next = !ppp ? NULL : ppp->next) {
        if (protocol == ppp->protocol) {
            break;
        }
    }
    if (!ppp) {
        if (head->state == 9) {
            ++head->codeRejectCount;
            LCPOut(head, 8, head->codeRejectCount, *(u16*)(data + 4), data + 2);
        }
        return;
    }
    switch (protocol) {
    case 0xC021:
        LCPIn(ppp, data + 2, len - 2, flag);
        break;
    case 0x8021:
        LCPIn(ppp, data + 2, len - 2, flag);
        break;
    case 0xC023:
        PAPIn(ppp, data + 2, len - 2, flag);
        break;
    case 0xC223:
        CHAPIn(ppp, data + 2, len - 2, flag);
        break;
    case 0xC022:
        break;
    }
}

s32 PPPOpen(PPPPPP* ppp)
{
    s32 enabled = OSDisableInterrupts();

    switch (ppp->protocol) {
    case 0xC023:
        PAPOpen(ppp);
        break;
    case 0xC223:
        CHAPOpen(ppp);
        break;
    default:
        switch (ppp->state) {
        case 0:
            PPPSetState(ppp, 1);
            if (ppp->started(ppp) && ppp->prev)
                PPPOpen(ppp->prev);
            break;
        case 1:
        case 3:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            break;
        case 2:
            PPPInitializeRestartCount(ppp);
            if (ppp->restartCount1 > 0) {
                ppp->restartCount1--;
                LCPOut(ppp, 1, ppp->restartCount, ppp->optLen, ppp->opt);
            }
            PPPSetState(ppp, 6);
            break;
        case 4:
            PPPSetState(ppp, 5);
            break;
        }
        break;
    }
    return OSRestoreInterrupts(enabled);
}

s32 PPPUp(PPPPPP* ppp)
{
    s32 enabled = OSDisableInterrupts();

    switch (ppp->protocol) {
    case 0xC023:
        PAPUp(ppp);
        break;
    case 0xC223:
        CHAPUp(ppp);
        break;
    default:
        switch (ppp->state) {
        case 0:
            PPPSetState(ppp, 2);
            break;
        case 1:
            PPPInitializeRestartCount(ppp);
            if (ppp->restartCount1 > 0) {
                ppp->restartCount1--;
                LCPOut(ppp, 1, ppp->restartCount, ppp->optLen, ppp->opt);
            }
            PPPSetState(ppp, 6);
            break;
        }
        break;
    }
    return OSRestoreInterrupts(enabled);
}

void PPPDown(PPPPPP* ppp)
{
    s32 enabled = OSDisableInterrupts();

    switch (ppp->protocol) {
    case 0xC023:
        PAPDown(ppp);
        break;
    case 0xC223:
        CHAPDown(ppp);
        break;
    default:
        switch (ppp->state) {
        case 0:
        case 1:
            break;
        case 2:
            ppp->state = 0;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            break;
        case 3:
            ppp->state = 1;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            if (ppp->started(ppp) && ppp->prev)
                PPPOpen(ppp->prev);
            break;
        case 4:
            ppp->state = 0;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            break;
        case 5:
            ppp->state = 1;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            break;
        case 6:
            ppp->state = 1;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            break;
        case 7:
            ppp->state = 1;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            break;
        case 8:
            ppp->state = 1;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            break;
        case 9:
            ppp->state = 1;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            if (ppp->next)
                PPPDown(ppp->next);
            ppp->down(ppp);
            break;
        }
        break;
    }
    OSRestoreInterrupts(enabled);
}

void PPPClose(PPPPPP* ppp)
{
    s32 enabled = OSDisableInterrupts();

    switch (ppp->protocol) {
    case 0xC023:
        PAPClose(ppp);
        break;
    case 0xC223:
        CHAPClose(ppp);
        break;
    default:
        switch (ppp->state) {
        case 0:
            break;
        case 1:
            ppp->state = 0;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            if (ppp->finished(ppp) && ppp->prev)
                PPPClose(ppp->prev);
            break;
        case 2:
            break;
        case 3:
            ppp->state = 2;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            break;
        case 4:
            break;
        case 5:
            ppp->state = 4;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            break;
        case 6:
            ppp->restartCount0 = 0;
            ppp->restartCount1 = 10;
            ppp->restartCount2 = 2;
            ppp->restartCount3 = 5;
            ppp->restartCount++;
            OSCancelAlarm((OSAlarm*)&ppp->alarm);
            if (ppp->restartCount2 > 0) {
                ppp->restartCount2--;
                LCPOut(ppp, 5, ppp->restartCount, 0, NULL);
            }
            ppp->state = 4;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            break;
        case 7:
            ppp->restartCount0 = 0;
            ppp->restartCount1 = 10;
            ppp->restartCount2 = 2;
            ppp->restartCount3 = 5;
            ppp->restartCount++;
            OSCancelAlarm((OSAlarm*)&ppp->alarm);
            if (ppp->restartCount2 > 0) {
                ppp->restartCount2--;
                LCPOut(ppp, 5, ppp->restartCount, 0, NULL);
            }
            ppp->state = 4;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            break;
        case 8:
            ppp->restartCount0 = 0;
            ppp->restartCount1 = 10;
            ppp->restartCount2 = 2;
            ppp->restartCount3 = 5;
            ppp->restartCount++;
            OSCancelAlarm((OSAlarm*)&ppp->alarm);
            if (ppp->restartCount2 > 0) {
                ppp->restartCount2--;
                LCPOut(ppp, 5, ppp->restartCount, 0, NULL);
            }
            ppp->state = 4;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            break;
        case 9:
            ppp->restartCount0 = 0;
            ppp->restartCount1 = 10;
            ppp->restartCount2 = 2;
            ppp->restartCount3 = 5;
            ppp->restartCount++;
            OSCancelAlarm((OSAlarm*)&ppp->alarm);
            if (ppp->restartCount2 > 0) {
                ppp->restartCount2--;
                LCPOut(ppp, 5, ppp->restartCount, 0, NULL);
            }
            ppp->state = 4;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            if (ppp->next)
                PPPDown(ppp->next);
            ppp->down(ppp);
            break;
        }
        break;
    }
    OSRestoreInterrupts(enabled);
}

void PPPTimeout(OSAlarm* alarm, OSContext* context)
{
    PPPPPP* ppp = (PPPPPP*)((u8*)alarm - 0x38);
    BOOL error;
    BOOL ret;

    ppp->restartCount0++;
    switch (ppp->protocol) {
    case 0xC023:
        error = !PAPTimeout(ppp);
        break;
    case 0xC223:
        error = !CHAPTimeout(ppp);
        break;
    default:
        switch (ppp->state) {
        case 4:
            if (ppp->restartCount2 > 0) {
                ppp->restartCount2--;
                LCPOut(ppp, 5, ppp->restartCount, 0, NULL);
                ret = TRUE;
            } else {
                ret = FALSE;
            }
            error = !ret;
            if (!error)
                break;
            ppp->state = 2;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            if (ppp->finished(ppp) && ppp->prev)
                PPPClose(ppp->prev);
            break;
        case 5:
            if (ppp->restartCount2 > 0) {
                ppp->restartCount2--;
                LCPOut(ppp, 5, ppp->restartCount, 0, NULL);
                ret = TRUE;
            } else {
                ret = FALSE;
            }
            error = !ret;
            if (!error)
                break;
            ppp->state = 3;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            if (ppp->finished(ppp) && ppp->prev)
                PPPClose(ppp->prev);
            break;
        case 6:
            if (ppp->restartCount1 > 0) {
                ppp->restartCount1--;
                LCPOut(ppp, 1, ppp->restartCount, ppp->optLen, ppp->opt);
                ret = TRUE;
            } else {
                ret = FALSE;
            }
            error = !ret;
            if (!error)
                break;
            ppp->state = 3;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            if (ppp->finished(ppp) && ppp->prev)
                PPPClose(ppp->prev);
            break;
        case 7:
            if (ppp->restartCount1 > 0) {
                ppp->restartCount1--;
                LCPOut(ppp, 1, ppp->restartCount, ppp->optLen, ppp->opt);
                ret = TRUE;
            } else {
                ret = FALSE;
            }
            error = !ret;
            if (error) {
                ppp->state = 3;
                if (ppp->setState)
                    ppp->setState(ppp);
                switch (ppp->state) {
                case 0:
                case 1:
                case 2:
                case 9:
                    OSCancelAlarm((OSAlarm*)&ppp->alarm);
                    break;
                }
                if (ppp->finished(ppp) && ppp->prev)
                    PPPClose(ppp->prev);
            } else {
                ppp->state = 6;
                if (ppp->setState)
                    ppp->setState(ppp);
                switch (ppp->state) {
                case 0:
                case 1:
                case 2:
                case 9:
                    OSCancelAlarm((OSAlarm*)&ppp->alarm);
                    break;
                }
            }
            break;
        case 8:
            if (ppp->restartCount1 > 0) {
                ppp->restartCount1--;
                LCPOut(ppp, 1, ppp->restartCount, ppp->optLen, ppp->opt);
                ret = TRUE;
            } else {
                ret = FALSE;
            }
            error = !ret;
            if (!error)
                break;
            ppp->state = 3;
            if (ppp->setState)
                ppp->setState(ppp);
            switch (ppp->state) {
            case 0:
            case 1:
            case 2:
            case 9:
                OSCancelAlarm((OSAlarm*)&ppp->alarm);
                break;
            }
            if (ppp->finished(ppp) && ppp->prev)
                PPPClose(ppp->prev);
            break;
        }
        break;
    }
    if (error)
        IPSetConfigError(ppp->ifp, -103);
}

s32 PPPDeleteOpt(u8* ppp, s32 offset, PPPOpt* opt)
{
    u8 len = opt->len;

    memmove(opt, (u8*)opt + len, ppp + (u16)offset - ((u8*)opt + len));
    return offset - len;
}

s32 PPPInsertOpt(u8* ppp, s32 offset, u8* opt, PPPOpt* newOpt)
{
    u8* p = opt + newOpt->len;

    memmove(p, opt, ppp + (u16)offset - opt);
    memmove(opt, newOpt, newOpt->len);
    return offset + newOpt->len;
}

s32 PPPInit(PPPPPP* ifp, PPPPPP* lcp, PPPPPP* ipcp, const char* user,
            const char* pass)
{
    u32 len;

    PPPInitLCP(lcp, ifp);
    PPPInitIPCP(ipcp, ifp);
    ifp->tail = NULL;
    ifp->head = NULL;
    {
        PPPPPP* p = ifp->tail;

        if (!p)
            ifp->head = lcp;
        else
            p->next = lcp;
        lcp->prev = p;
    }
    lcp->next = NULL;
    ifp->tail = lcp;
    {
        PPPPPP* p = ifp->tail;

        if (!p)
            ifp->head = ipcp;
        else
            p->next = ipcp;
        ipcp->prev = p;
    }
    ipcp->next = NULL;
    ifp->tail = ipcp;

    if (user) {
        len = strlen(user);
        if (len > 255) {
            IPSetConfigError(NULL, -108);
            return 0;
        }
        __PPPAuth.userLen = len;
        strncpy(__PPPAuth.user, user, 256);
    }
    if (pass) {
        len = strlen(pass);
        if (len > 255) {
            IPSetConfigError(NULL, -108);
            return 0;
        }
        __PPPAuth.passLen = len;
        strncpy(__PPPAuth.pass, pass, 256);
    }
    PPPOpen(ipcp);
    return 1;
}
