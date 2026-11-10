#include <dolphin/ip/ipppp.h>
#include <dolphin/ip/ip.h>
#include <string.h>

extern void PPPInitLCP(PPPPPP* lcp, void* ifp);
extern void PPPInitIPCP(PPPPPP* ipcp, void* ifp);
extern void IPSetConfigError(void* unused, s32 error);
extern s32 IPIn(void* ifp, void* data, s32 len);
extern s32 LCPOut(PPPPPP* ppp, u8 code, u8 id, u16 len, u8* data);
extern s32 LCPIn(PPPPPP* ppp, u8* data, s32 len);
extern s32 PAPIn(PPPPPP* ppp, u8* data, s32 len);
extern s32 PAPOpen(PPPPPP* ppp);
extern s32 PAPUp(PPPPPP* ppp);
extern void PAPDown(PPPPPP* ppp);
extern void PAPClose(PPPPPP* ppp);
extern s32 PAPTimeout(PPPPPP* ppp);
extern s32 CHAPIn(PPPPPP* ppp, u8* data, s32 len);
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

void PPPIn(PPPPPP* ifp, u8* data, s32 len)
{
    u16 protocol;
    u8* p = data;
    PPPPPP* head;
    PPPPPP* ppp;
    PPPPPP* next;

    if (len < 2)
        return;
    protocol = *(u16*)p;
    if (protocol == IP_PROTO_UDP + 16) {
        if (ifp->tail->state == 9)
            IPIn(ifp, p + 2, len - 2);
        return;
    }
    ppp = ifp->head;
    head = ppp;
    next = !ppp ? NULL : ppp->next;
    while (ppp) {
        if (ppp->protocol == protocol)
            break;
        ppp = next;
        next = !ppp ? NULL : ppp->next;
    }
    if (!ppp) {
        if (head->state == 9) {
            u8 id = ++head->codeRejectCount;

            LCPOut(head, 8, id, *(u16*)(p + 4), p + 2);
        }
        return;
    }
    switch (protocol) {
    case 0xC021:
    case 0x8021:
        LCPIn(ppp, data + 2, len - 2);
        break;
    case 0xC023:
        PAPIn(ppp, p + 2, len - 2);
        break;
    case 0xC223:
        CHAPIn(ppp, p + 2, len - 2);
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
