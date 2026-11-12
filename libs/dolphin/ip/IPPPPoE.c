#include <dolphin/ip/ipppp.h>
#include <dolphin/ip/ip.h>
#include <dolphin/os.h>
#include <string.h>

typedef struct IFVec {
    void* data;
    s32 len;
} IFVec;

typedef struct PPPoEHeader {
    u8 verType;
    u8 code;
    u16 session;
    u16 len;
    u8 data[1];
} PPPoEHeader;

typedef struct PPPoETag {
    u16 type;
    u16 len;
    u8 data[1];
} PPPoETag;

typedef struct IFDatagram {
    u8 unk00[4];
    u32 unk04;
    u8 unk08[8];
    u16 type;
    u8 dst[6];
    u8 nextHop[4];
    u8 hdr[6];
    u16 proto;
    u8 hdrLen;
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
    u8 unk00[0x40];
    s32 mtu;
    u8 unk44[0x58 - 0x44];
    PPPPPP* head;
    PPPPPP* tail;
    void (*out)(IFInterface* ifp, IFDatagram* datagram);
    void (*cancel)(IFInterface* ifp, IFDatagram* datagram);
    IFDatagram* (*alloc)(IFInterface* ifp, s32 len);
};

typedef struct PPPoEConf {
    u8 code;
    u16 session;
    u16 lastSession;
    u8 lastDst[6];
    OSAlarm alarm;
    u8 dst[6];
    PPPoEHeader hdr;
    u8 tags[0x5D6 - 2];
    u16 tagsLen;
    u16 retry;
    IFInterface* ifp;
    void (*out)(IFInterface* ifp, IFDatagram* datagram);
} PPPoEConf;

extern u8 __IFDefault[];
extern void IPSetConfigError(IFInterface* ifp, s32 error);
extern BOOL IPIsLoopbackAddr(IFInterface* ifp, const u8* addr);

PPPPPP PPPLcpConf;
PPPPPP PPPIpcpConf;
PPPPPP PPPAuthConf;

static PPPoEConf Conf = { 0, 0xFFFF, 0xFFFF };

static void TimeoutCallback(OSAlarm* alarm, OSContext* context);

static void OutPADT(IFInterface* ifp, u8* dst, u16 session)
{
    PPPoEConf* conf = &Conf;
    BOOL enabled;
    IFDatagram* datagram;
    PPPoEHeader* hdr;

    enabled = OSDisableInterrupts();
    datagram = ifp->alloc(ifp, 66);
    hdr = (PPPoEHeader*)datagram->buf;
    if (datagram) {
        hdr->verType = 0x11;
        hdr->code = 0xA7;
        hdr->session = session;
        hdr->len = 0;
        datagram->nVec = 1;
        datagram->vec[0].data = hdr;
        datagram->vec[0].len = 6;
        datagram->callback = 0;
        datagram->param = 0;
        datagram->type = 0x8863;
        datagram->unk25 = 0;
        memmove(datagram->dst, dst, 6);
        datagram->unk04 = 0;
        conf->out(ifp, datagram);
    }
    OSRestoreInterrupts(enabled);
}

static void PPPoEOut(IFInterface* ifp, PPPoEConf* conf)
{
    BOOL enabled;
    IFDatagram* datagram;
    PPPoEHeader* hdr;

    enabled = OSDisableInterrupts();
    OSCancelAlarm(&conf->alarm);
    OSSetAlarm(&conf->alarm, OSSecondsToTicks((OSTime)(3 << conf->retry)), TimeoutCallback);
    if (conf->code == 0xA7) {
        OutPADT(ifp, conf->dst, conf->session);
    } else {
        datagram = ifp->alloc(ifp, 60);
        hdr = &conf->hdr;
        hdr->code = conf->code;
        hdr->session = conf->session;
        hdr->len = conf->tagsLen;
        if (datagram) {
            datagram->nVec = 1;
            datagram->vec[0].data = hdr;
            datagram->vec[0].len = conf->tagsLen + 6;
            datagram->callback = 0;
            datagram->param = 0;
            datagram->type = 0x8863;
            datagram->unk25 = 0;
            memmove(datagram->dst, conf->dst, 6);
            datagram->unk04 = 0;
            conf->out(ifp, datagram);
        }
    }
    OSRestoreInterrupts(enabled);
}

static inline void Reset(PPPoEConf* conf, u16 lastSession)
{
    OSCancelAlarm(&conf->alarm);
    conf->code = 0;
    conf->session = 0xFFFF;
    conf->lastSession = lastSession;
    if (lastSession != 0xFFFF) {
        memmove(conf->lastDst, conf->dst, 6);
    }
    conf->ifp->mtu = 1500;
    if (conf->ifp->head) {
        PPPDown(conf->ifp->head);
    }
}

static void TimeoutCallback(OSAlarm* alarm, OSContext* context)
{
    PPPoEConf* conf = (PPPoEConf*)((u8*)alarm - 16);

    conf->retry++;
    if (conf->code == 0xA7) {
        Reset(conf, 0xFFFF);
    } else if (conf->retry >= 4) {
        IPSetConfigError(conf->ifp, -103);
        Reset(conf, 0xFFFF);
    } else {
        PPPoEOut(conf->ifp, conf);
    }
}

static void Out(IFInterface* ifp, IFDatagram* datagram)
{
    BOOL enabled;
    PPPoEHeader* hdr;
    IFVec* vec;
    IFVec* end;

    enabled = OSDisableInterrupts();
    if (datagram->type != 0x800 || !IPIsLoopbackAddr(ifp, datagram->nextHop)) {
        hdr = (PPPoEHeader*)datagram->hdr;
        hdr->verType = 0x11;
        hdr->code = Conf.code;
        hdr->session = Conf.session;
        hdr->len = 0;
        end = &datagram->vec[datagram->nVec];
        for (vec = datagram->vec; vec < end; vec++) {
            hdr->len += (u16)vec->len;
        }
        switch (datagram->type) {
        case 0x800:
            datagram->proto = 0x21;
            hdr->len += 2;
            datagram->hdrLen = 8;
            datagram->type = 0x8864;
            break;
        case 0x8864:
            datagram->hdrLen = 6;
            break;
        default:
            datagram->hdrLen = 0;
            break;
        }
        if (datagram->type == 0x8864) {
            memmove(datagram->dst, Conf.dst, 6);
        }
    }
    Conf.out(ifp, datagram);
    OSRestoreInterrupts(enabled);
}

s32 PPPoEInit(IFInterface* ifp)
{
    PPPoEConf* conf = &Conf;
    PPPoEHeader* hdr;
    PPPoETag* tag;

    if (conf->session != 0xFFFF) {
        return 0;
    }
    ifp = ifp ? ifp : (IFInterface*)__IFDefault;
    PPPLcpConf.mru = 1492;
    conf->code = 9;
    conf->tagsLen = 4;
    conf->session = 0;
    memset(conf->dst, 0xFF, 6);
    OSCreateAlarm(&conf->alarm);
    conf->retry = 0;
    conf->hdr.verType = 0x11;
    hdr = &conf->hdr;
    hdr->code = conf->code;
    hdr->session = conf->session;
    hdr->len = conf->tagsLen;
    tag = (PPPoETag*)hdr->data;
    tag->type = 0x101;
    tag->len = 0;
    conf->ifp = ifp;
    conf->out = ifp->out;
    ifp->out = Out;
    PPPoEOut(ifp, conf);
    return 1;
}

void PPPoETerminate(IFInterface* ifp)
{
    PPPoEConf* conf = &Conf;
    IFInterface* p;

    p = ifp ? ifp : (IFInterface*)__IFDefault;
    if (p->head && conf->session != 0xFFFF && conf->code == 0) {
        conf->code = 0xA7;
        conf->retry = 0;
        PPPoEOut(p, conf);
    }
    if (p->out == Out) {
        p->out = conf->out;
    }
}

static s32 ParseTags(PPPoEConf* conf, PPPoEHeader* hdr)
{
    s32 err;
    PPPoETag* tag;
    BOOL cookie;
    BOOL acName;
    u16 len;

    tag = (PPPoETag*)hdr->data;
    err = 0;
    cookie = FALSE;
    acName = FALSE;
    len = conf->tagsLen;
    while ((u8*)tag < (u8*)hdr + hdr->len && tag->type != 0) {
        switch (tag->type) {
        case 0x0000:
        case 0x0101:
        case 0x0103:
        case 0x0105:
        case 0x0110:
            break;
        case 0x0102:
            if (!acName && conf->code == 9) {
                acName = TRUE;
                memmove(&conf->hdr.data[len], tag, tag->len + 4);
                len += tag->len + 4;
            }
            break;
        case 0x0104:
            if (!cookie && conf->code == 9) {
                cookie = TRUE;
                memmove(&conf->hdr.data[len], tag, tag->len + 4);
                len += tag->len + 4;
            }
            break;
        case 0x0201:
            err = -104;
            break;
        case 0x0202:
            err = -105;
            break;
        case 0x0203:
            err = -106;
            break;
        }
        tag = (PPPoETag*)((u8*)tag + (tag->len + 4));
    }
    if (err == 0) {
        conf->tagsLen = len;
    }
    return err;
}

void PPPoEIn(IFInterface* ifp, u8* frame, s32 len, s32 flag)
{
    PPPoEConf* conf = &Conf;
    PPPoEHeader* hdr = (PPPoEHeader*)(frame + 14);
    PPPPPP* ppp;
    s32 err;
    u16 type;

    if (len < 20 || len < hdr->len + 20) {
        return;
    }
    if (hdr->verType != 0x11) {
        return;
    }
    ppp = ifp->head;
    if (!ppp) {
        return;
    }
    type = *(u16*)(frame + 12);
    switch (type) {
    case 0x8863:
        switch (hdr->code) {
        case 0x07:
            if (conf->code != 9 || hdr->session != 0) {
                break;
            }
            err = ParseTags(conf, hdr);
            if (err == 0) {
                conf->code = 0x19;
                conf->retry = 0;
                memmove(conf->dst, frame + 6, 6);
                PPPoEOut(ifp, conf);
            } else {
                IPSetConfigError(conf->ifp, err);
                Reset(conf, 0xFFFF);
            }
            break;
        case 0x65:
            if (conf->code != 0x19 || memcmp(conf->dst, frame + 6, 6) != 0) {
                break;
            }
            err = ParseTags(conf, hdr);
            if (err == 0 && hdr->session != 0xFFFF) {
                conf->code = 0;
                conf->retry = 0;
                conf->session = hdr->session;
                OSCancelAlarm(&conf->alarm);
                PPPUp(ppp);
            } else {
                IPSetConfigError(conf->ifp, err);
                Reset(conf, 0xFFFF);
            }
            break;
        case 0xA7:
            if (hdr->session == 0xFFFF) {
                break;
            }
            if (hdr->session == conf->session && memcmp(conf->dst, frame + 6, 6) == 0) {
                if (conf->code == 0xA7) {
                    Reset(conf, 0xFFFF);
                    break;
                }
                if (conf->code == 0) {
                    PPPDown(ppp);
                    PPPoETerminate(ifp);
                    IPSetConfigError(conf->ifp, -110);
                    Reset(conf, conf->session);
                    break;
                }
            }
            if (hdr->session == conf->lastSession && memcmp(conf->lastDst, frame + 6, 6) == 0) {
                OutPADT(ifp, frame + 6, hdr->session);
            }
            break;
        }
        break;
    case 0x8864:
        if (hdr->session != 0xFFFF && conf->code == 0 && hdr->code == 0 &&
            memcmp(conf->dst, frame + 6, 6) == 0 && hdr->session == conf->session)
        {
            PPPIn((PPPPPP*)ifp, hdr->data, hdr->len, flag);
        }
        break;
    }
}
