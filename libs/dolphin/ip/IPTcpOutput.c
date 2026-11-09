#include <dolphin/types.h>
#include <dolphin/ip/ip.h>
#include <dolphin/tcp/tcp.h>

#include <string.h>

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

typedef struct TCPSackEntryOutput {
    u32 start;
    u32 end;
} TCPSackEntryOutput;

typedef struct TCPSackHoleOutput {
    s32 unk0;
    s32 start;
    s32 dupAcks;
    s32 end;
} TCPSackHoleOutput;

typedef struct TCPSegmentOutput {
    void* data;
    s32 len;
} TCPSegmentOutput;

typedef struct TCPSocketOutput TCPSocketOutput;
struct TCPSocketOutput {
    u8 unk00[10];
    u16 localPort;
    u8 unk0C[6];
    u16 remotePort;
    u8 unk14[24];
    s32 error;
    s32 unk30;
    s32 unk34;
    s32 unk38;
    s32 unk3C;
    u8 unk40[8];
    s32 unk48;
    s32 unk4C;
    s32 unk50;
    s32 unk54;
    s32 unk58;
    u8 unk5C[4];
    s32 unk60;
    u8 unk64[8];
    TCPSackEntryOutput sack[4];
    TCPSackHoleOutput holes[4];
    s32 holeIndex;
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
    u8 unk110[100];
    s32 unk174;
    s32 unk178;
    s32 ringBase;
    s32 ringSize;
    u8 packet[37];
    u8 immediate;
    u8 unk1AA[2];
    void (*outputCallback)(TCPSocketOutput* so, s32 result);
    TCPSocketOutput* callbackArg;
    s32 fragmentCount;
    TCPSegmentOutput fragments[4];
    u32 unk1D8;
    u8 unk1DC[4];
    u8 unk1E0[4];
    u8* data;
    s32 dataLen;
    u8 unk1EC[4];
    s32 lastSendHi;
    s32 lastSendLo;
    u8 unk1F8[4];
    s32 unk1FC;
    s32 unk200;
    u32 unk204;
    s32 rxmitNext;
    s32 rxmitCount;
    u8 unk210[48];
    s32 rtoHi;
    s32 rtoLo;
    u8 unk248[16];
    u32 unk258;
    u8 unk25C[36];
    s32 maxCwnd;
    u8 unk284[4];
    s32 rttActive;
    s32 rttSeq;
    s32 rttStartHi;
    s32 rttStartLo;
};

void TCPOutput(TCPSocketOutput* so, u16 flag);
void TCPStartRxmitTimer(TCPSocketOutput* so);
s32 IFRingGet(s32 arg0, s32 arg1, void* ringData, s32 size,
              TCPSegmentOutput* fragments, s32 len);
s32 IPOut(void* packet);
s64 OSGetTime(void);

typedef struct TCPStatOutput {
    s32 sentPackets;
    u8 pad04[8];
    s32 sentDataPackets;
    s32 sentBytes;
} TCPStatOutput;

extern TCPStatOutput TCPStat;

static inline s32 TCPOutputMin(s32 a, s32 b)
{
    return a < b ? a : b;
}


static TCPSackHoleOutput* TCPSackOutput(const TCPSocketOutput* so)
{
    const TCPSackHoleOutput* hole;
    const TCPSackHoleOutput* holeEnd;

    holeEnd = &so->holes[so->holeIndex];
    for (hole = so->holes; hole < holeEnd; hole++) {
        if (hole->start - hole->end > 0 &&
            ((so->flags & 0x1000) || hole->dupAcks >= 3) &&
            so->unk30 - hole->end <= 0) {
            return (TCPSackHoleOutput*)hole;
        }
    }
    return NULL;
}

s32 TCPMakeOption(TCPHeader* tcp, TCPSocketOutput* so, u16 flag)
{
    s32 optlen;
    s32 padding;
    u8* opt;
    TCPSackEntryOutput* block;
    s32* edge;

    if (so == NULL) {
        tcp->flag &= ~0xF000;
        tcp->flag |= 0x5000;
        return 0;
    }

    opt = (u8*)tcp + 20;
    if (flag & 2) {
        *opt++ = 2;
        *opt++ = 4;
        *(u16*)opt = so->mss;
        opt += 2;
        *opt++ = 4;
        *opt++ = 2;
    }

    if ((so->flags & 0x2000) && so->sack[0].start != 0) {
        while ((opt - (u8*)tcp) % 4 != 2) {
            *opt++ = 1;
        }
        opt[0] = 5;
        edge = (s32*)(opt + 2);
        for (block = &so->sack[3]; so->sack <= block; block--) {
            if (block->start) {
                if (so->unk204 <= block->start) {
                    edge[0] = block->start - so->unk204;
                } else {
                    edge[0] = block->start + so->unk1FC - so->unk204;
                }
                edge[0] += so->unk54 - so->unk200;
                edge[1] = edge[0] + block->end;
                edge += 2;
            }
        }
        opt[1] = (u8*)edge - opt;
        opt = (u8*)edge;
    }

    optlen = opt - ((u8*)tcp + 20);
    if (optlen % 4 != 0) {
        padding = 4 - optlen % 4;
        *opt++ = 0;
        memset(opt, 1, padding - 1);
        optlen += padding;
    }
    tcp->flag &= ~0xF000;
    tcp->flag |= (20 + optlen) << 10;
    return optlen;
}

static s32 TCPCalcSendSize(TCPSocketOutput* so, s32 maxSize, u16* flag,
                           TCPSackHoleOutput* hole)
{
    s32 available;
    s32 sendSize;
    s32 size;
    s32 payload;
    s32 nextSeq;
    s32 cwnd;
    u16 flagBits;
    s32 done;
    s32 copySize;

    flagBits = *flag;

restart:
    size = 0;
    switch (so->state) {
    case 2:
    case 3:
        if (so->unk48 == so->unk34) {
            flagBits |= 2;
            size = 1;
        } else if (so->unk30 == so->unk48) {
            done = so->unk34 - so->unk30 - 1;
            break;
        }
    default:
        done = so->unk34 - so->unk30;
        break;
    }

    size += so->ringSize;
    if (so->unk1D8 != 0 && so->dataLen > 0) {
        size += so->dataLen;
    }
    nextSeq = so->unk30 + size;
    size -= done;

    if (hole != NULL) {
        size = MIN(size, hole->start - hole->end);
    }

    switch (so->state) {
    case 5:
    case 8:
    case 9:
        if (size >= 0 && hole == NULL) {
            flagBits |= 1;
            size++;
            nextSeq++;
        }
        break;
    }

    if (flagBits & 2) {
        flagBits &= ~1;
        nextSeq = so->unk30 + 1;
        done = 0;
        size = 1;
    }

    if ((so->flags & 0x2000) && so->rxmitCount >= 3) {
        cwnd = so->unk38;
    } else {
        cwnd = MIN(so->unk38, so->maxCwnd);
    }

    if (cwnd == 0 && nextSeq - so->unk30 > 0 && so->unk258 == 0) {
        if (so->unk34 != so->unk30) {
            so->unk34 = so->unk30;
            size = 0;
            so->rttActive = 0;
            so->rxmitCount = 0;
            so->holeIndex = 0;
            so->unkD0 = so->unk30;
            so->unkD8 = 0;
            so->unkD4 = 0;
            so->unkDC = so->unk50;
            hole = NULL;
            goto restart;
        }
        flagBits |= 0x10;
        cwnd = 1;
        so->flags |= 0x200;
    }

    available = so->unk30 + cwnd - so->unk34;
    so->fragmentCount = 1;
    if ((so->flags & 0x2000) && !(so->flags & 0x1000) && size > 0 &&
        so->unkDC - so->unk30 > 0 && so->maxCwnd <= so->unkD4) {
        size = 0;
    }

    sendSize = MIN(maxSize, MIN(size, available));

    if (sendSize == size && hole == NULL && sendSize > 0 && (flagBits & 7) == 0) {
        flagBits |= 8;
    }
    if ((flagBits & 1) && nextSeq - (so->unk34 + sendSize) > 0) {
        flagBits &= ~1;
    }
    if (sendSize <= 0) {
        *flag &= ~0xB;
        return 0;
    }

    payload = sendSize;
    if (flagBits & 2) {
        payload--;
    }
    if (flagBits & 1) {
        payload--;
    }
    if (payload > 0) {
        if (so->ringSize > 0 && done < so->ringSize) {
            copySize = so->ringSize - done;
            copySize = MIN(copySize, payload);
            so->fragmentCount += IFRingGet(so->unk174, so->unk178, (void*)(so->ringBase + done),
                                           so->ringSize - done, &so->fragments[1], copySize);
            payload -= copySize;
            done += copySize;
        }
        if (payload > 0) {
            done -= so->ringSize;
            so->fragments[so->fragmentCount].data = so->data + done;
            so->fragments[so->fragmentCount].len = payload;
            so->fragmentCount++;
        }
    }

    *flag = flagBits;
    return sendSize;
}

static BOOL DoSwsAvoidance(TCPSocketOutput* so, s32 size, s32 maxSize, u16 flag)
{
    register s32 window;
    BOOL check;

    window = TCPOutputMin(so->unk1FC / 2, maxSize);

    if (window <= so->unk1FC - so->unk200 - so->unk58) {
        so->unk58 = so->unk1FC - so->unk200;
        return FALSE;
    }

    if (flag & 0x17) {
        return FALSE;
    }
    if (size <= 0) {
        return TRUE;
    }
    if (maxSize <= size) {
        return FALSE;
    }

    check = (so->flags & 2) ? so->unk34 == so->unk30 : TRUE;
    if (check) {
        if (flag & 8) {
            return FALSE;
        }
        if (so->unk4C / 2 <= size) {
            return FALSE;
        }
    }

    if (so->unk50 - so->unk34 > 0) {
        return FALSE;
    }
    if (so->unk3C - so->unk30 > 0) {
        return FALSE;
    }
    if (so->flags & 0x1000) {
        return FALSE;
    }
    return TRUE;
}

static void TCPOutputCallback(TCPSocketOutput* so, s32 result)
{
    so->outputPending = 0;
    if (result < 0) {
        so->error = result;
    }
    TCPOutput(so, 0);
}

void TCPOutput(TCPSocketOutput* so, u16 flags)
{
    TCPHeader* ip;
    IPHeader* tcp;
    s32 sendSize;
    TCPSegmentOutput* fragment;
    TCPSackHoleOutput* hole;
    s32 optionLen;
    s32 oldNxt;
    s32 result;
    s32 difference;

    hole = NULL;
    if (so->state == 0) {
        return;
    }
    if (so->outputPending == 1) {
        return;
    }

    oldNxt = so->unk34;
    if ((so->flags & 0x2000) && so->rxmitCount >= 3) {
        hole = TCPSackOutput(so);
        if (hole != NULL) {
            so->unk34 = hole->end;
        }
    }

    switch (so->state) {
    case 2:
        flags &= ~0x10;
        break;
    case 3:
        if (so->unk60 + 1 - so->rxmitNext > 0) {
            flags |= 0x10;
        }
        break;
    default:
        if (so->unk54 - so->rxmitNext > 0) {
            flags |= 0x10;
        }
        break;
    }
    switch (so->state) {
    case 2:
    case 3:
        if (so->unk48 == so->unk34) {
            flags |= 2;
        }
        break;
    }

    if (so->unk30 == so->unk50 &&
        *(s64*)&so->lastSendHi + *(s64*)&so->rtoHi < OSGetTime()) {
        so->maxCwnd = so->mss;
    }

    tcp = &so->ip;
    tcp->len = (tcp->verlen & 0xF) << 2;
    ip = (TCPHeader*)((u8*)so + ((tcp->verlen & 0xF) << 2));
    *(u16*)((u8*)ip + 264) = 0;
    ip = (TCPHeader*)((u8*)ip + 252);
    optionLen = TCPMakeOption(ip, so, flags);
    sendSize = TCPCalcSendSize(so, so->mss - optionLen, &flags, hole);

    if (hole != NULL && so->unk34 != hole->end) {
        oldNxt = so->unk34;
        hole = NULL;
        so->flags &= ~0x1000;
    }
    if (DoSwsAvoidance(so, sendSize, so->mss - optionLen, flags)) {
        if (sendSize > 0) {
            TCPStartRxmitTimer(so);
        }
        return;
    }

    if (so->state != 3) {
        difference = so->unk54;
        so->rxmitNext = difference;
        ip->ack = difference;
    } else {
        difference = so->unk60 + 1;
        so->rxmitNext = difference;
        ip->ack = difference;
    }
    ip->src = so->localPort;
    ip->dst = so->remotePort;
    if (sendSize > 0) {
        ip->seq = so->unk34;
    } else {
        ip->seq = so->unk50;
    }

    if (so->unk3C - ip->seq > 0) {
        flags |= 0x20;
        difference = so->unk3C - ip->seq;
        ip->urg = (u16)(65535 - 20 - 20 < difference ? 65535 : difference);
    } else {
        ip->urg = 0;
        so->unk3C = so->unk30;
    }
    if (so->state != 2) {
        flags |= 0x10;
    }

    ip->flag &= ~0x3F;
    ip->flag |= flags & 0x3F;
    ip->win = so->unk58;
    ip->sum = 0;
    tcp->len += (ip->flag & 0xF000) >> 10;

    fragment = &so->fragments[1];
    while (fragment < &so->fragments[so->fragmentCount]) {
        tcp->len += (u16)fragment->len;
        fragment++;
    }

    so->unk34 += sendSize;
    if (so->unk34 - so->unk50 > 0) {
        so->unk50 = so->unk34;
        if (so->rttActive == 0) {
            so->rttActive = 1;
            so->rttSeq = ip->seq;
            *(s64*)&so->rttStartHi = OSGetTime();
        }
    } else if (sendSize > 0) {
        TCPStat.sentDataPackets++;
        TCPStat.sentBytes += tcp->len - (((tcp->verlen << 2) & 0x3C) +
                                        ((ip->flag >> 10) & 0x3C));
    }
    if (so->unk34 != so->unk30) {
        TCPStartRxmitTimer(so);
    }

    so->outputPending = 1;
    so->immediate = 0;
    so->fragments[0].data = tcp;
    so->fragments[0].len = ((tcp->verlen << 2) & 0x3C) + ((ip->flag >> 10) & 0x3C);
    so->outputCallback = TCPOutputCallback;
    so->callbackArg = so;

    if (hole != NULL) {
        hole->end += sendSize;
        so->unkD8 += sendSize;
        if (oldNxt - so->unk34 > 0) {
            so->unk34 = oldNxt;
        }
    }

    so->unkD4 = so->unkD8 + (so->unk34 - so->unkD0);
    so->flags &= ~0x1000;
    result = IPOut(so->packet);
    if (result < 0) {
        so->outputPending = 0;
        if (result < 0) {
            so->error = result;
        }
        TCPOutput(so, 0);
        return;
    }

    *(s64*)&so->lastSendHi = OSGetTime();
    TCPStat.sentPackets++;
}
