#include <dolphin/types.h>
#include <dolphin/ip/ip.h>
#include <dolphin/tcp/tcp.h>

#include <string.h>

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

s32 IFRingGet(s32 arg0, s32 arg1, void* ringData, s32 size,
              TCPSegment* fragments, s32 len);
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


static TCPSackHole* TCPSackOutput(const TCPSocket* so)
{
    const TCPSackHole* hole;
    const TCPSackHole* holeEnd;

    holeEnd = &so->holes[so->holeIndex];
    for (hole = so->holes; hole < holeEnd; hole++) {
        if (hole->start - hole->end > 0 &&
            ((so->flags & 0x1000) || hole->dupAcks >= 3) &&
            so->snd_una - hole->end <= 0) {
            return (TCPSackHole*)hole;
        }
    }
    return NULL;
}

s32 TCPMakeOption(TCPHeader* tcp, TCPSocket* so, u16 flag)
{
    s32 optlen;
    s32 padding;
    u8* opt;
    TCPSackEntry* block;
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

static s32 TCPCalcSendSize(TCPSocket* so, s32 maxSize, u16* flag,
                           TCPSackHole* hole)
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
        if (so->unk48 == so->snd_nxt) {
            flagBits |= 2;
            size = 1;
        } else if (so->snd_una == so->unk48) {
            done = so->snd_nxt - so->snd_una - 1;
            break;
        }
    default:
        done = so->snd_nxt - so->snd_una;
        break;
    }

    size += so->ringSize;
    if (so->recvCallback != 0 && so->dataLen > 0) {
        size += so->dataLen;
    }
    nextSeq = so->snd_una + size;
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
        nextSeq = so->snd_una + 1;
        done = 0;
        size = 1;
    }

    if ((so->flags & 0x2000) && so->numDupAcks >= 3) {
        cwnd = so->snd_wnd;
    } else {
        cwnd = MIN(so->snd_wnd, so->cwnd);
    }

    if (cwnd == 0 && nextSeq - so->snd_una > 0 && so->rxmitAlarm.handler == 0) {
        if (so->snd_nxt != so->snd_una) {
            so->snd_nxt = so->snd_una;
            size = 0;
            so->rttActive = 0;
            so->numDupAcks = 0;
            so->holeIndex = 0;
            so->unkD0 = so->snd_una;
            so->unkD8 = 0;
            so->unkD4 = 0;
            so->unkDC = so->snd_max;
            hole = NULL;
            goto restart;
        }
        flagBits |= 0x10;
        cwnd = 1;
        so->flags |= 0x200;
    }

    available = so->snd_una + cwnd - so->snd_nxt;
    so->fragmentCount = 1;
    if ((so->flags & 0x2000) && !(so->flags & 0x1000) && size > 0 &&
        so->unkDC - so->snd_una > 0 && so->cwnd <= so->unkD4) {
        size = 0;
    }

    sendSize = MIN(maxSize, MIN(size, available));

    if (sendSize == size && hole == NULL && sendSize > 0 && (flagBits & 7) == 0) {
        flagBits |= 8;
    }
    if ((flagBits & 1) && nextSeq - (so->snd_nxt + sendSize) > 0) {
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

static BOOL DoSwsAvoidance(TCPSocket* so, s32 size, s32 maxSize, u16 flag)
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

    check = (so->flags & 2) ? so->snd_nxt == so->snd_una : TRUE;
    if (check) {
        if (flag & 8) {
            return FALSE;
        }
        if (so->unk4C / 2 <= size) {
            return FALSE;
        }
    }

    if (so->snd_max - so->snd_nxt > 0) {
        return FALSE;
    }
    if (so->unk3C - so->snd_una > 0) {
        return FALSE;
    }
    if (so->flags & 0x1000) {
        return FALSE;
    }
    return TRUE;
}

static void TCPOutputCallback(TCPSocket* so, s32 result)
{
    so->outputPending = 0;
    if (result < 0) {
        so->error = result;
    }
    TCPOutput(so, 0);
}

void TCPOutput(TCPSocket* so, u16 flags)
{
    IPHeader* tcp;
    TCPHeader* ip;
    s32 sendSize;
    TCPSegment* fragment;
    TCPSackHole* hole;
    s32 optionLen;
    s32 oldNxt;
    s32 result;
    s32 difference;
    s32 urgent;

    hole = NULL;
    if (so->state == 0) {
        return;
    }
    if (so->outputPending == 1) {
        return;
    }

    oldNxt = so->snd_nxt;
    if ((so->flags & 0x2000) && so->numDupAcks >= 3) {
        hole = TCPSackOutput(so);
        if (hole != NULL) {
            so->snd_nxt = hole->end;
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
        if (so->unk48 == so->snd_nxt) {
            flags |= 2;
        }
        break;
    }

    if (so->snd_una == so->snd_max &&
        *(s64*)&so->lastSend + *(s64*)&so->rxtcur < OSGetTime()) {
        so->cwnd = so->mss;
    }

    tcp = &so->ip;
    tcp->len = (tcp->verlen & 0xF) << 2;
    ip = (TCPHeader*)((u8*)&so->ip + ((so->ip.verlen & 0xF) << 2));
    ip->flag = 0;
    optionLen = TCPMakeOption(ip, so, flags);
    sendSize = TCPCalcSendSize(so, so->mss - optionLen, &flags, hole);

    if (hole != NULL && so->snd_nxt != hole->end) {
        oldNxt = so->snd_nxt;
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
        ip->seq = so->snd_nxt;
    } else {
        ip->seq = so->snd_max;
    }

    if (so->unk3C - ip->seq > 0) {
        flags |= 0x20;
        urgent = so->unk3C - ip->seq;
        ip->urg = (u16)(65535 - 20 - 20 < urgent ? 65535 : urgent);
    } else {
        ip->urg = 0;
        so->unk3C = so->snd_una;
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

    so->snd_nxt += sendSize;
    if (so->snd_nxt - so->snd_max > 0) {
        so->snd_max = so->snd_nxt;
        if (so->rttActive == 0) {
            so->rttActive = 1;
            so->rttSeq = ip->seq;
            *(s64*)&so->rttStart = OSGetTime();
        }
    } else if (sendSize > 0) {
        TCPStat.sentDataPackets++;
        TCPStat.sentBytes += tcp->len - (((tcp->verlen & 0xF) << 2) +
                                         ((ip->flag & 0xF000) >> 10));
    }
    if (so->snd_nxt != so->snd_una) {
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
        if (oldNxt - so->snd_nxt > 0) {
            so->snd_nxt = oldNxt;
        }
    }

    so->unkD4 = so->unkD8 + (so->snd_nxt - so->unkD0);
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

    *(s64*)&so->lastSend = OSGetTime();
    TCPStat.sentPackets++;
}
