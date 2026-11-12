#include <dolphin/ip/ipppp.h>
#include <dolphin/ip/ip.h>
#include <dolphin/os.h>
#include <string.h>

typedef struct IFVec {
    void* data;
    s32 len;
} IFVec;

typedef struct IFDatagram {
    u8 unk00[4];
    u32 unk04;
    u8 unk08[8];
    u16 type;
    u8 unk12[0x25 - 0x12];
    u8 unk25;
    u8 unk26[2];
    u32 callback;
    u32 param;
    s32 nVec;
    IFVec vec[1];
    u8 buf[1];
} IFDatagram;

typedef struct IFInterface IFInterface;
struct IFInterface {
    u8 unk00[0x60];
    void (*out)(IFInterface* ifp, IFDatagram* datagram);
    void (*cancel)(IFInterface* ifp, IFDatagram* datagram);
    IFDatagram* (*alloc)(IFInterface* ifp, s32 len);
};

typedef struct PPPHeader {
    u8 code;
    u8 id;
    u16 len;
} PPPHeader;

extern void IPSetConfigError(void* unused, s32 error);

static void PAPOut(PPPPPP* ppp)
{
    u16* hdr;
    IFInterface* ifp;
    BOOL enabled;
    IFDatagram* datagram;
    s32 len;
    u8* p;

    enabled = OSDisableInterrupts();
    ifp = (IFInterface*)ppp->ifp;
    len = __PPPAuth.userLen + 1 + __PPPAuth.passLen + 1;
    OSCancelAlarm((OSAlarm*)&ppp->alarm);
    OSSetAlarm((OSAlarm*)&ppp->alarm, OSSecondsToTicks((OSTime)3), PPPTimeout);
    datagram = ifp->alloc(ifp, len + 66);
    if (datagram) {
        *(hdr = (u16*)datagram->buf) = 0xC023;
        datagram->buf[2] = 1;
        datagram->buf[3] = ppp->restartCount;
        *(u16*)&datagram->buf[4] = len + 4;
        p = &datagram->buf[6];
        *p++ = __PPPAuth.userLen;
        memmove(p, __PPPAuth.user, __PPPAuth.userLen);
        p += __PPPAuth.userLen;
        *p++ = __PPPAuth.passLen;
        memmove(p, __PPPAuth.pass, __PPPAuth.passLen);
        datagram->nVec = 1;
        datagram->vec[0].data = hdr;
        datagram->vec[0].len = len + 6;
        datagram->callback = 0;
        datagram->param = 0;
        datagram->type = 0x8864;
        datagram->unk25 = 0;
        datagram->unk04 = 0;
        ifp->out(ifp, datagram);
    }
    OSRestoreInterrupts(enabled);
}

void PAPOpen(PPPPPP* ppp)
{
    switch (ppp->state) {
    case 0:
        PPPSetState(ppp, 1);
        PPPLayerStarted(ppp);
        break;
    case 2:
        PPPInitializeRestartCount(ppp);
        if (ppp->restartCount1 > 0) {
            ppp->restartCount1--;
            PAPOut(ppp);
        }
        PPPSetState(ppp, 6);
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        return;
    }
}

void PAPUp(PPPPPP* ppp)
{
    switch (ppp->state) {
    case 0:
        break;
    case 1:
        PPPInitializeRestartCount(ppp);
        if (ppp->restartCount1 > 0) {
            ppp->restartCount1--;
            PAPOut(ppp);
        }
        PPPSetState(ppp, 6);
        break;
    case 2:
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        return;
    }
}

void PAPDown(PPPPPP* ppp)
{
    switch (ppp->state) {
    case 0:
    case 1:
    case 3:
    case 4:
    case 5:
        break;
    case 2:
        PPPSetState(ppp, 0);
        break;
    case 6:
        PPPSetState(ppp, 1);
        break;
    case 7:
        PPPSetState(ppp, 1);
        break;
    case 8:
        PPPSetState(ppp, 1);
        break;
    case 9:
        PPPSetState(ppp, 1);
        PPPLayerDown(ppp);
        break;
    }
}

void PAPClose(PPPPPP* ppp)
{
    switch (ppp->state) {
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
        break;
    case 1:
        PPPSetState(ppp, 2);
        PPPLayerFinished(ppp);
        break;
    case 6:
        PPPSetState(ppp, 2);
        PPPLayerFinished(ppp);
        break;
    case 7:
        PPPSetState(ppp, 2);
        PPPLayerFinished(ppp);
        break;
    case 8:
        PPPSetState(ppp, 2);
        PPPLayerFinished(ppp);
        break;
    case 9:
        PPPSetState(ppp, 2);
        PPPLayerDown(ppp);
        PPPLayerFinished(ppp);
        break;
    }
}

s32 PAPTimeout(PPPPPP* ppp)
{
    BOOL error = FALSE;
    BOOL ret;

    switch (ppp->state) {
    case 6:
        if (ppp->restartCount1 > 0) {
            ppp->restartCount1--;
            PAPOut(ppp);
            ret = TRUE;
        } else {
            ret = FALSE;
        }
        if (!ret) {
            error = TRUE;
            PPPSetState(ppp, 2);
            PPPLayerFinished(ppp);
        }
        break;
    case 7:
        if (ppp->restartCount1 <= 0) {
            error = TRUE;
            PPPSetState(ppp, 2);
            PPPLayerFinished(ppp);
        }
        break;
    case 8:
        if (ppp->restartCount1 > 0) {
            ppp->restartCount1--;
            PAPOut(ppp);
            ret = TRUE;
        } else {
            ret = FALSE;
        }
        if (!ret) {
            error = TRUE;
            PPPSetState(ppp, 2);
            PPPLayerFinished(ppp);
        }
        break;
    case 3:
    case 4:
    case 5:
    case 9:
        ret = FALSE;
        break;
    }
    return !error;
}

void PAPIn(PPPPPP* ppp, u8* data, s32 len, s32 flag)
{
    PPPHeader* hdr = (PPPHeader*)data;

    if (len < 4 || len < hdr->len || hdr->len < 4) {
        return;
    }
    switch (hdr->code) {
    case 1:
        break;
    case 2:
        if (hdr->id == ppp->restartCount) {
            PPPInitializeRestartCount(ppp);
            PPPSetState(ppp, 9);
            PPPLayerUp(ppp);
        }
        break;
    case 3:
        if (hdr->id == ppp->restartCount) {
            IPSetConfigError(NULL, -108);
            PPPSetState(ppp, 2);
            PPPLayerFinished(ppp);
        }
        break;
    }
}

static s32 Up(PPPPPP* ppp)
{
    return 1;
}

static s32 Down(PPPPPP* ppp)
{
    return 1;
}

static s32 Started(PPPPPP* ppp)
{
    return 1;
}

static s32 Finished(PPPPPP* ppp)
{
    return 1;
}

void PAPInit(PPPPPP* ppp, void* ifp)
{
    memset(ppp, 0, sizeof(PPPPPP));
    ppp->protocol = 0xC023;
    OSCreateAlarm((OSAlarm*)&ppp->alarm);
    ppp->ifp = ifp;
    ppp->state = 0;
    ppp->up = Up;
    ppp->down = Down;
    ppp->started = Started;
    ppp->finished = Finished;
}
