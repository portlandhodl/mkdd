#include <dolphin/ip/ipppp.h>
#include <dolphin/ip/ip.h>
#include <dolphin/md5.h>
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

static void CHAPOut(IFInterface* ifp, PPPHeader* data)
{
    u16* p;
    BOOL enabled;
    IFDatagram* datagram;

    enabled = OSDisableInterrupts();
    datagram = ifp->alloc(ifp, data->len + 62);
    if (datagram) {
        *(p = (u16*)datagram->buf) = 0xC223;
        memmove(&datagram->buf[2], data, data->len);
        datagram->nVec = 1;
        datagram->vec[0].data = p;
        datagram->vec[0].len = data->len + 2;
        datagram->callback = 0;
        datagram->param = 0;
        datagram->type = 0x8864;
        datagram->unk25 = 0;
        datagram->unk04 = 0;
        ifp->out(ifp, datagram);
    }
    OSRestoreInterrupts(enabled);
}

void CHAPOpen(PPPPPP* ppp)
{
    switch (ppp->state) {
    case 0:
        PPPSetState(ppp, 1);
        PPPLayerStarted(ppp);
        break;
    case 2:
        PPPInitializeRestartCount(ppp);
        PPPSetState(ppp, 8);
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

void CHAPUp(PPPPPP* ppp)
{
    switch (ppp->state) {
    case 0:
        break;
    case 1:
        PPPInitializeRestartCount(ppp);
        PPPSetState(ppp, 8);
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

void CHAPDown(PPPPPP* ppp)
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

void CHAPClose(PPPPPP* ppp)
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

s32 CHAPTimeout(PPPPPP* ppp)
{
    BOOL error = FALSE;

    switch (ppp->state) {
    case 7:
        if (ppp->restartCount1 <= 0) {
            error = TRUE;
            PPPSetState(ppp, 2);
            PPPLayerFinished(ppp);
        }
        break;
    case 8:
        if (ppp->restartCount1 <= 0) {
            error = TRUE;
            PPPSetState(ppp, 2);
            PPPLayerFinished(ppp);
        }
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 9:
        break;
    }
    return !error;
}

void CHAPIn(PPPPPP* ppp, PPPHeader* hdr, s32 len)
{
    MD5Context context;
    u8 size;
    u8* p;
    u8* value;

    if (len < 4 || len < hdr->len || hdr->len < 4) {
        return;
    }
    switch (hdr->code) {
    case 1:
        ppp->restartCount = hdr->id;
        p = (u8*)(hdr + 1);
        size = *p;
        if (hdr->len < size + 5u) {
            break;
        }
        hdr->code = 2;
        hdr->len = __PPPAuth.userLen + 21;
        *p = 16;
        MD5Init(&context);
        MD5Update(&context, &hdr->id, 1);
        MD5Update(&context, (u8*)__PPPAuth.pass, __PPPAuth.passLen);
        value = (u8*)hdr + 5;
        MD5Update(&context, value, size);
        MD5Final(value, &context);
        memmove((u8*)hdr + 21, __PPPAuth.user, __PPPAuth.userLen);
        CHAPOut(ppp->ifp, hdr);
        break;
    case 2:
        break;
    case 3:
        if (ppp->restartCount == hdr->id) {
            PPPSetState(ppp, 9);
            PPPLayerUp(ppp);
        }
        break;
    case 4:
        if (ppp->restartCount == hdr->id) {
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

void CHAPInit(PPPPPP* ppp, void* ifp)
{
    memset(ppp, 0, sizeof(PPPPPP));
    ppp->protocol = 0xC223;
    OSCreateAlarm((OSAlarm*)&ppp->alarm);
    ppp->ifp = ifp;
    ppp->state = 0;
    ppp->up = Up;
    ppp->down = Down;
    ppp->started = Started;
    ppp->finished = Finished;
}
