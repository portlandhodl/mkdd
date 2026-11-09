#include <dolphin/ip/ip.h>
#include <dolphin/os.h>
#include <dolphin/tcp/tcp.h>
#include <dolphin/types.h>

typedef struct TCPSocketTimer {
    u8 unk00[44];
    s32 error;
    s32 snd_una;
    s32 snd_nxt;
    s32 snd_wnd;
    u8 unk3C[12];
    s32 unk48;
    u8 unk4C[4];
    s32 snd_max;
    u8 unk54[120];
    s32 unkCC;
    s32 unkD0;
    s32 unkD4;
    s32 unkD8;
    s32 unkDC;
    u8 unkE0[4];
    s32 state;
    u32 flags;
    u8 unkEC[8];
    s32 mss;
    s32 outputPending;
    IPHeader ip;
    u8 unk110[112];
    s32 recvAvail;
    u8 unk184[24];
    u8 remoteAddr[4];
    u8 unk1A0[56];
    void* recvCallback;
    u8 unk1DC[12];
    s32 urgent;
    u8 unk1EC[32];
    s32 numDupAcks;
    u8 unk210[40];
    s32 nrxmit;
    u8 unk23C[4];
    s64 rxtcur;
    s64 unk248;
    s64 idleTimeout;
    OSAlarm rxmitAlarm;
    s32 cwnd;
    s32 ssthresh;
    s32 rttActive;
    u8 unk28C[4];
    s64 rttStart;
    s64 srtt;
    s64 rttvar;
    s64 rxtmin;
    s64 rxtmax;
} TCPSocketTimer;

typedef struct TCPStats {
    u32 unk00;
    u32 unk04;
    u32 timeouts;
} TCPStats;

extern TCPStats TCPStat;
extern s32 __TCPMaxPersist;
extern u16 IPCheckSum(void* data);
extern void ARPRevalidate(u8* addr);
extern void IPRecoverGateway(u8* addr);
extern s32 TCPAbort(TCPSocketTimer* so);
extern void TCPOutput(TCPSocketTimer* so, u16 flag);

static void TCPRxmitTimeOut(OSAlarm* alarm, OSContext* context)
{
    IPHeader* ip;
    TCPSocketTimer* so;
    s64 timeout;

    so = (TCPSocketTimer*)((u8*)alarm - 600);
    TCPStat.timeouts++;
    so->nrxmit++;
    if (so->nrxmit >= 16) {
        so->nrxmit = 16;
    }

    if (!(so->flags & 0x200)) {
        if (so->nrxmit == 4 && so->state >= 4) {
            ip = &so->ip;
            ip->frag &= ~0x4000;
            if (so->outputPending != 0) {
                ip->sum = 0;
                ip->sum = IPCheckSum(ip);
            }
            if (so->mss > 536) {
                so->mss = 536;
            }
        }

        if (so->nrxmit == 3) {
            if (IPIsLocalAddr(0, so->remoteAddr)) {
                ARPRevalidate(so->remoteAddr);
            }
            else {
                IPRecoverGateway(so->remoteAddr);
            }
        }
        else if (so->nrxmit > 3) {
            switch (so->state) {
            case 2:
            case 3:
                timeout = OSSecondsToTicks((s64)180);
                if (so->unk48 == so->snd_una && so->idleTimeout < timeout) {
                    break;
                }
            default:
                timeout = so->idleTimeout;
                break;
            }
            if (OSGetTime() - so->unk248 >= timeout) {
                if (so->error == 0) {
                    so->error = -10;
                }
                TCPAbort(so);
                return;
            }
        }
    }
    else {
        if (OSGetTime() - so->unk248 >= OSSecondsToTicks((s64)__TCPMaxPersist)) {
            if (so->error == 0) {
                so->error = -10;
            }
            TCPAbort(so);
            return;
        }
    }

    so->snd_nxt = so->snd_una;
    so->rttActive = 0;
    so->ssthresh = (so->cwnd < so->snd_wnd ? so->cwnd : so->snd_wnd) / 2;
    so->ssthresh = so->ssthresh > so->mss * 2 ? so->ssthresh : so->mss * 2;
    so->cwnd = so->mss;
    so->numDupAcks = 0;
    so->unkCC = 0;
    so->unkD0 = so->snd_una;
    so->unkD8 = 0;
    so->unkD4 = 0;
    so->unkDC = so->snd_max;
    TCPOutput(so, 0x10);
}

void TCPStartRxmitTimer(TCPSocketTimer* so)
{
    s64 timeout;

    if (so->rxmitAlarm.handler == NULL) {
        timeout = (1 << so->nrxmit) * so->rxtcur;
        if (timeout > OSSecondsToTicks((s64)240)) {
            timeout = OSSecondsToTicks((s64)240);
        }
        if (so->nrxmit == 0) {
            so->unk248 = OSGetTime();
        }
        OSSetAlarm(&so->rxmitAlarm, timeout, TCPRxmitTimeOut);
    }
}

static inline void DoTCPCancelRxmitTimer(TCPSocketTimer* so)
{
    so->flags &= ~0x200;
    so->nrxmit = 0;
    OSCancelAlarm(&so->rxmitAlarm);
}

void TCPStopRxmitTimer(TCPSocketTimer* so, TCPHeader* tcp)
{
    s32 recvAvail;

    recvAvail = so->recvAvail;
    if (so->recvCallback != NULL && so->urgent > 0) {
        recvAvail += so->urgent;
    }

    if (so->snd_wnd == 0 && recvAvail > 0) {
        so->flags |= 0x200;
        so->unk248 = OSGetTime();
        TCPStartRxmitTimer(so);
    }
    else {
        if (so->flags & 0x200) {
            DoTCPCancelRxmitTimer(so);
        }
        if (so->snd_una == so->snd_max) {
            DoTCPCancelRxmitTimer(so);
        }
        else {
            TCPStartRxmitTimer(so);
        }
    }
}

void TCPCancelRxmitTimer(TCPSocketTimer* so)
{
    DoTCPCancelRxmitTimer(so);
}

static s64 CalcRto(TCPSocketTimer* so)
{
    so->rxtcur = so->srtt + 4 * so->rttvar;
    if (so->rxtcur < so->rxtmin) {
        so->rxtcur = so->rxtmin;
    }
    else if (so->rxtcur > so->rxtmax) {
        so->rxtcur = so->rxtmax;
    }
    return so->rxtcur;
}

void TCPUpdateRtt(TCPSocketTimer* so, s64 rtt)
{
    s64 delta;

    so->rttStart = rtt;
    if (so->srtt != 0) {
        delta = so->rttStart - so->srtt;
        so->srtt += delta / 8;
        if (delta < 0) {
            delta = -delta;
        }
        so->rttvar += (delta - so->rttvar) / 4;
    }
    else {
        so->srtt = so->rttStart;
        so->rttvar = so->rttStart / 2;
    }
    so->nrxmit = 0;
    CalcRto(so);
}

void TCPInitRtt(TCPSocketTimer* so)
{
    so->rttStart = 0;
    so->srtt = 0;
    so->rttvar = OSMillisecondsToTicks((s64)750);
    so->rxtmin = OSMillisecondsToTicks((s64)200);
    so->rxtmax = OSSecondsToTicks((s64)240);
    so->nrxmit = 0;
    CalcRto(so);
}
