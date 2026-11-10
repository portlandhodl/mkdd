#include <dolphin/ip/ip.h>
#include <dolphin/os.h>
#include <dolphin/tcp/tcp.h>
#include <dolphin/types.h>

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
extern s32 TCPAbort(TCPSocket* so);

static void TCPRxmitTimeOut(OSAlarm* alarm, OSContext* context)
{
    IPHeader* ip;
    TCPSocket* so;
    s64 timeout;

    so = (TCPSocket*)((u8*)alarm - 600);
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
                if (so->unk48 == so->snd_una && so->idleTimeout.value < timeout) {
                    break;
                }
            default:
                timeout = so->idleTimeout.value;
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
    so->holeIndex = 0;
    so->unkD0 = so->snd_una;
    so->unkD8 = 0;
    so->unkD4 = 0;
    so->unkDC = so->snd_max;
    TCPOutput(so, 0x10);
}

void TCPStartRxmitTimer(TCPSocket* so)
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

static inline void DoTCPCancelRxmitTimer(TCPSocket* so)
{
    so->flags &= ~0x200;
    so->nrxmit = 0;
    OSCancelAlarm(&so->rxmitAlarm);
}

void TCPStopRxmitTimer(TCPSocket* so, TCPHeader* tcp)
{
    s32 recvAvail;

    recvAvail = so->ringSize;
    if (so->recvCallback != NULL && so->dataLen > 0) {
        recvAvail += so->dataLen;
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

void TCPCancelRxmitTimer(TCPSocket* so)
{
    DoTCPCancelRxmitTimer(so);
}

static s64 CalcRto(TCPSocket* so)
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

void TCPUpdateRtt(TCPSocket* so, s64 rtt)
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

void TCPInitRtt(TCPSocket* so)
{
    so->rttStart = 0;
    so->srtt = 0;
    so->rttvar = OSMillisecondsToTicks((s64)750);
    so->rxtmin = OSMillisecondsToTicks((s64)200);
    so->rxtmax = OSSecondsToTicks((s64)240);
    so->nrxmit = 0;
    CalcRto(so);
}
