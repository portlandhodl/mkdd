#include <dolphin/ip/ipppp.h>
#include <dolphin/ip/ip.h>
#include <string.h>

typedef struct PPPInterface {
    u8 unk00[0x40];
    s32 mtu;
    u8 unk44[0x58 - 0x44];
    PPPPPP* head;
    PPPPPP* tail;
} PPPInterface;

extern PPPPPP PPPAuthConf;

extern void PAPInit(PPPPPP* ppp, void* ifp);
extern void CHAPInit(PPPPPP* ppp, void* ifp);
extern s32 PPPoEInit(void* ifp);
extern void PPPoETerminate(void* ifp);

typedef struct PPPHeader {
    u8 code;
    u8 id;
    u16 len;
    u8 data[1];
} PPPHeader;

static const u8 OptPAP[4] = { 3, 4, 0xC0, 0x23 };

static s32 ReceiveConfigureRequest(PPPPPP* ppp, PPPHeader* data)
{
    u8* opts;
    u8* end;
    u8* p;
    int len;
    u16 mru;
    BOOL reject;
    u8 alg;
    BOOL nak;
    u16 auth;

    reject = FALSE;
    opts = p = data->data;
    nak = FALSE;
    auth = 0;
    mru = ppp->mru;
    len = (u16)(data->len - 4);
    end = opts + len;
    while (p < end && !reject) {
        switch (p[0]) {
        case 1:
            if (p[1] != 4) {
                return -3;
            }
            mru = *(u16*)(p + 2);
            if (mru < 128) {
                nak = TRUE;
            }
            break;
        case 3:
            if (p[1] < 4) {
                return -3;
            }
            auth = *(u16*)(p + 2);
            switch (auth) {
            case 0xC023:
                if (p[1] != 4) {
                    return -3;
                }
                break;
            case 0xC223:
                if (p[1] != 5) {
                    return -3;
                }
                alg = p[4];
                if (alg != 5) {
                    nak = TRUE;
                }
                break;
            default:
                nak = TRUE;
                break;
            }
            break;
        case 5:
        default:
            reject = TRUE;
            break;
        }
        p += p[1];
    }

    if (reject) {
        p = opts;
        while (p < opts + (u16)len) {
            switch (p[0]) {
            case 1:
            case 3:
                len = PPPDeleteOpt(opts, len, (PPPOpt*)p);
                continue;
            case 2:
                break;
            }
            p += p[1];
        }
        data->len = len + 4;
        return -1;
    }

    if (nak && ppp->restartCount3 <= 0) {
        p = opts;
        while (p < opts + (u16)len) {
            switch (p[0]) {
            case 1:
                if (mru >= 128) {
                    len = PPPDeleteOpt(opts, len, (PPPOpt*)p);
                    continue;
                }
                break;
            case 3:
                switch (auth) {
                case 0xC023:
                    len = PPPDeleteOpt(opts, len, (PPPOpt*)p);
                    continue;
                case 0xC223:
                    if (alg == 5) {
                        len = PPPDeleteOpt(opts, len, (PPPOpt*)p);
                        continue;
                    }
                    break;
                }
                break;
            case 2:
                break;
            }
            p += p[1];
        }
        data->len = len + 4;
        return -1;
    }

    if (nak) {
        p = opts;
        while (p < opts + (u16)len) {
            switch (p[0]) {
            case 1:
                if (mru < 128) {
                    *(u16*)(p + 2) = 128;
                    p += p[1];
                    continue;
                }
                break;
            case 3:
                switch (auth) {
                case 0xC223:
                    if (alg != 5) {
                        p[4] = 5;
                        p += p[1];
                        continue;
                    }
                    break;
                case 0xC023:
                    break;
                default:
                    len = PPPDeleteOpt(opts, len, (PPPOpt*)p);
                    len = PPPInsertOpt(opts, len, p, (PPPOpt*)OptPAP);
                    p += p[1];
                    continue;
                }
                break;
            }
            len = PPPDeleteOpt(opts, len, (PPPOpt*)p);
        }
        data->len = len + 4;
        return -2;
    }

    ppp->mru = mru;
    ppp->unk92 = auth;
    return 0;
}

static s32 ReceiveConfigureAck(PPPPPP* ppp, u8* data)
{
    if (*(u16*)(data + 2) != ppp->optLen + 4 || memcmp(data + 4, ppp->opt, ppp->optLen) != 0) {
        return 0;
    }
    return 1;
}

static s32 ReceiveConfigureNak(PPPPPP* ppp, PPPHeader* data)
{
    u8* opts;
    u8* p;
    u8* end;

    opts = data->data;
    end = opts + (u16)(data->len - 4);
    for (p = opts; p < end; p += p[1]) {
    }
    return 1;
}

static s32 ReceiveConfigureReject(PPPPPP* ppp, PPPHeader* data)
{
    u8* end;
    u8* q;
    u8* p;
    u8* opts;

    q = ppp->opt;
    opts = data->data;
    end = opts + (u16)(data->len - 4);
    for (p = opts; p < end; p += p[1]) {
        u8 type = *p;
        u8* oend = ppp->opt + ppp->optLen;

        for (; type != *q; q += q[1]) {
            if (oend <= q) {
                return 0;
            }
        }
        ppp->optLen = PPPDeleteOpt(ppp->opt, ppp->optLen, (PPPOpt*)q);
    }
    return 1;
}

static s32 Up(PPPPPP* ppp)
{
    PPPInterface* ifp;
    PPPPPP* auth;

    ifp = ppp->ifp;
    if (ppp->mru < ifp->mtu) {
        ifp->mtu = ppp->mru;
    }
    switch (ppp->unk92) {
    case 0xC023:
        auth = &PPPAuthConf;
        PAPInit(auth, ifp);
        break;
    case 0xC223:
        auth = &PPPAuthConf;
        CHAPInit(auth, ifp);
        break;
    default:
        auth = NULL;
        break;
    }
    if (auth) {
        auth->prev = ppp;
        auth->next = ppp->next;
        ppp->next = auth;
        if (auth->next == NULL) {
            ifp->tail = auth;
        } else {
            auth->next->prev = auth;
        }
        PPPOpen(auth);
    }
    return 1;
}

static s32 Down(PPPPPP* ppp)
{
    PPPPPP* auth;
    PPPInterface* ifp;
    PPPPPP* next;
    PPPPPP* prev;

    auth = ppp->next;
    if (auth == &PPPAuthConf) {
        next = auth->next;
        ifp = ppp->ifp;
        prev = auth->prev;
        if (next == NULL) {
            ifp->tail = prev;
        } else {
            next->prev = prev;
        }
        if (prev == NULL) {
            ifp->head = next;
        } else {
            prev->next = next;
        }
    }
    return 1;
}

static s32 Started(PPPPPP* ppp)
{
    return PPPoEInit(ppp->ifp);
}

static s32 Finished(PPPPPP* ppp)
{
    PPPoETerminate(ppp->ifp);
    return 1;
}

s32 PPPInitLCP(PPPPPP* ppp, void* ifp)
{
    memset(ppp, 0, sizeof(PPPPPP));
    ppp->protocol = 0xC021;
    OSCreateAlarm((OSAlarm*)&ppp->alarm);
    ppp->ifp = ifp;
    ppp->receiveConfigureRequest = (void*)ReceiveConfigureRequest;
    ppp->receiveConfigureAck = ReceiveConfigureAck;
    ppp->receiveConfigureNak = (void*)ReceiveConfigureNak;
    ppp->receiveConfigureReject = (void*)ReceiveConfigureReject;
    ppp->up = Up;
    ppp->down = Down;
    ppp->started = Started;
    ppp->finished = Finished;
    ppp->mru = 1500;
    return 1;
}
