#include <dolphin/types.h>
#include <dolphin/ip/ip.h>
#include <dolphin/tcp/tcp.h>
#include <dolphin/os.h>

#include <string.h>

typedef struct IPInterface IPInterface;

typedef struct TCPTimeWaitInfo {
    u8 remoteAddr[4];
    u8 localAddr[4];
    u16 remotePort;
    u16 localPort;
    s32 seq;
    s32 ack;
    u32 expire;
} TCPTimeWaitInfo;

typedef struct TCPTimeWaitControl {
    TCPTimeWaitInfo* buf;
    TCPTimeWaitInfo* end;
    TCPTimeWaitInfo* head;
    s32 count;
    s32 max;
} TCPTimeWaitControl;

extern OSTime __OSGetSystemTime(void);
extern void TCPCancelRxmitTimer(TCPSocket* so);
extern s32 TCPAbort(TCPSocket* so);
extern void TCPRespond(IPInterface* interface, TCPSocket* info, u8* dstAddr,
                       u16 dstPort, u8* srcAddr, u16 srcPort, s32 seq, s32 ack,
                       u16 flag, u16 win);

static TCPTimeWaitControl Control;

#define NEXT(p) (((p) + 1 == Control.end) ? Control.buf : (p) + 1)

static TCPTimeWaitInfo* CreateTimeWaitInfo(void)
{
    TCPTimeWaitInfo* info;

    if (Control.count < Control.max) {
        info = Control.head + Control.count;
        Control.count++;
        if (Control.end <= info) {
            info -= Control.max;
        }
    } else if (Control.buf) {
        info = Control.head;
        Control.head = NEXT(Control.head);
    } else {
        info = NULL;
    }
    return info;
}

s32 TCPSetTimeWaitBuffer(void* buf, u32 len)
{
    if (Control.buf || (buf && len < sizeof(TCPTimeWaitInfo))) {
        return -12;
    }
    Control.buf = buf;
    Control.head = buf;
    Control.count = 0;
    Control.max = len / sizeof(TCPTimeWaitInfo);
    Control.end = Control.head + Control.max;
    return 0;
}

static void DiscardExpiredTimeWaitInfo(u32 now)
{
    TCPTimeWaitInfo* info;
    s32 n;
    s32 i;

    n = 0;
    info = Control.head;
    for (i = Control.count; 0 < i; i--) {
        if (info->expire <= now) {
            n++;
        }
        info = NEXT(info);
    }
    if (0 < n) {
        Control.head += n;
        if (Control.end <= Control.head) {
            Control.head -= Control.max;
        }
        Control.count -= n;
    }
}

static TCPTimeWaitInfo* LookupTimeWaitInfo(const u8* remoteAddr, u16 remotePort,
                                      const u8* localAddr, u16 localPort)
{
    TCPTimeWaitInfo* info;
    s32 i;

    info = Control.head;
    for (i = Control.count; 0 < i; i--) {
        if (info->remotePort == remotePort && info->localPort == localPort &&
            *(u32*)info->remoteAddr == *(u32*)remoteAddr &&
            *(u32*)info->localAddr == *(u32*)localAddr) {
            return info;
        }
        info = NEXT(info);
    }
    return NULL;
}

BOOL TCPLookupTimeWaitInfo(const u8* remoteAddr, u16 remotePort,
                           const u8* localAddr, u16 localPort)
{
    u32 now;

    now = (u32)(__OSGetSystemTime() / OS_TIMER_CLOCK);
    DiscardExpiredTimeWaitInfo(now);
    return LookupTimeWaitInfo(remoteAddr, remotePort, localAddr, localPort) ? TRUE : FALSE;
}

BOOL TCPTestTimeWait(IPInterface* interface, IPHeader* ip, TCPHeader* tcp)
{
    u32 now;
    TCPTimeWaitInfo* info;
    TCPTimeWaitInfo* newInfo;

    now = (u32)(__OSGetSystemTime() / OS_TIMER_CLOCK);
    DiscardExpiredTimeWaitInfo(now);
    info = LookupTimeWaitInfo(ip->src, tcp->src, ip->dst, tcp->dst);
    if (info == NULL) {
        return FALSE;
    }
    newInfo = CreateTimeWaitInfo();
    memmove(newInfo, info, sizeof(TCPTimeWaitInfo));
    newInfo->expire = now + 240;
    info->expire = 0;
    if (!(tcp->flag & 4)) {
        TCPRespond(interface, NULL, ip->src, tcp->src, ip->dst, tcp->dst,
                   newInfo->seq, newInfo->ack, 0x5010, 0);
    }
    return TRUE;
}

void TCPStartTimeWait(IPInterface* interface, TCPSocket* so)
{
    u32 now;
    TCPTimeWaitInfo* info;

    now = (u32)(__OSGetSystemTime() / OS_TIMER_CLOCK);
    TCPCancelRxmitTimer(so);
    info = CreateTimeWaitInfo();
    if (info) {
        memmove(info->remoteAddr, so->connRemoteAddr, 4);
        memmove(info->localAddr, &so->unk0C[0], 4);
        info->remotePort = so->remotePort;
        info->localPort = so->localPort;
        info->seq = so->snd_max;
        info->ack = so->unk54;
        info->expire = now + 240;
    }
    if (so->unk54 - so->rxmitNext > 0) {
        TCPRespond(interface, NULL, so->connRemoteAddr, so->remotePort,
                   so->unk0C, so->localPort, so->snd_max, so->unk54, 0x5010, 0);
    }
    TCPAbort(so);
}
