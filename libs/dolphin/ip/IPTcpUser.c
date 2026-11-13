#include <dolphin/ip/ip.h>
#include <dolphin/os.h>
#include <dolphin/tcp/tcp.h>
#include <dolphin/types.h>

#include <string.h>

#define SO_EPROTONOSUPPORT -69
#define SO_EBADR -63
#define SO_ENOMEM -12
#define SO_EINVAL -28
#define SO_EACCES -13
#define SO_EWOULDBLOCK -6
#define SO_EINTR -4
#define SO_EBADF -8
#define SO_EAGAIN -35
#define SO_ENOENT -2

typedef struct TCPInfoList {
    TCPSocket* head;
    TCPSocket* tail;
} TCPInfoList;

TCPInfoList TCPInfoQueue;
static u16 Port = 1024;

extern BOOL __IPIsMember(TCPInfoList* queue, TCPSocket* so);
extern s32 IPBind(TCPInfoList* queue, TCPSocket* so, u8* addr, s32 flag);
extern u16 IPGetAnonPort(TCPInfoList* queue, u16* port);
extern s32 IPGetSockOpt(TCPSocket* so, s32 level, s32 opt, void* val, s32* len);
extern s32 IPSetSockOpt(TCPSocket* so, s32 level, s32 opt, void* val, s32 len);
extern s32 IPLookupInfo(TCPInfoList* queue, const u8* dstAddr,
                        const u8* srcAddr, u16 dstPort, u16 srcPort, u32 flag);
extern s32 IPGetRemoteSocket(void* socket, u8* addr);
extern void IPCancel(void* packet);
extern void IPClose(TCPSocket* so);
extern void __IPWakeupPollingThreads(void);

extern void TCPRespond(void* interface, TCPSocket* info, u8* dstAddr,
                       u16 dstPort, u8* srcAddr, u16 srcPort, s32 seq,
                       s32 ack, u16 flag, u16 win);
extern s32 TCPSendIn(TCPSocket* so, s32 reset);
extern s32 TCPRecvOut(TCPSocket* so);
extern void TCPCancelRxmitTimer(TCPSocket* so);
extern void TCPInitRtt(TCPSocket* so);
extern void TCPOutput(TCPSocket* so, u16 flag);
extern s32 TCPCancel(TCPSocket* so);
extern s32 TCPAbort(TCPSocket* so);

s32 TCPEnumInfoQueue(TCPSocket* so);

static void NullCallback(TCPSocket* so, s32 result);
static void SyncCallback(TCPSocket* so, s32 result);
static s32 SendAsync(TCPSocket* so, const void* buf, s32 len, s32 flags,
                     void (*callback)(TCPSocket*, s32), s32* arg);

s32 TCPEnumInfoQueue(TCPSocket* so)
{
}


static void NullCallback(TCPSocket* so, s32 result)
{
}

static void SyncCallback(TCPSocket* so, s32 result)
{
    OSWakeupThread((OSThreadQueue*)so->unk20);
}

TCPSocket* TCPLookupInfo(IPHeader* ip, TCPHeader* tcp)
{
    return (TCPSocket*)IPLookupInfo(&TCPInfoQueue, ip->src, ip->dst,
                                    tcp->src, tcp->dst, 0);
}

s32 TCPGetStatus(TCPSocket* so)
{
    if (so->unk00[0] != IP_PROTO_TCP) {
        return SO_ENOMEM;
    }
    return so->state;
}

s32 TCPGetRemoteSocket(TCPSocket* so, u8* addr)
{
    BOOL enabled;
    s32 result;

    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP) {
        result = SO_ENOMEM;
    } else {
        result = IPGetRemoteSocket(so, addr);
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPGetLocalSocket(TCPSocket* so)
{
}

s32 TCPBind(TCPSocket* so, u8* addr)
{
    BOOL enabled;
    s32 result;

    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP) {
        result = SO_ENOMEM;
    } else if (so->state != 0) {
        result = SO_ENOMEM;
    } else if ((addr[4] & 0xF0) == 0xE0) {
        result = SO_EACCES;
    } else {
        result = IPBind(&TCPInfoQueue, so, addr,
                        (so->flags & 0x10000) ? 1 : 0);
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPAbort(TCPSocket* so)
{
    s32 state;
    void (*handlers[5])(TCPSocket*, s32);

    if (so->unk00[0] != IP_PROTO_TCP) {
        return 0;
    }

    state = so->state;
    so->state = 0;
    IPCancel(so->packet);
    TCPCancelRxmitTimer(so);
    OSCancelAlarm((OSAlarm*)((u8*)so + 0x2E0));

    {
        TCPSocket* child;
        while ((child = so->unk2C4) != NULL) {
            child->error = so->error;
            TCPAbort(child);
        }
    }

    if (so->unk2B8 != 0) {
        TCPSocket* prev = so->unk2CC;
        TCPSocket* next = so->unk2D0;
        TCPSocket* head;

        if (prev == NULL) {
            so->unk2B8->unk2C8 = next;
        } else {
            prev->unk2D0 = next;
        }
        if (next == NULL) {
            so->unk2B8->unk2C4 = prev;
        } else {
            next->unk2CC = prev;
        }
        so->unk2B8 = NULL;

        head = TCPInfoQueue.head;

        if (head == NULL) {
            TCPInfoQueue.tail = so;
        } else {
            head->unk1C = so;
        }
        so->unk18 = head;
        so->unk1C = NULL;
        TCPInfoQueue.head = so;
    } else {
        if (state == 1) {
            so->unk2D4 = (s32 (*)(TCPSocket*, s32))NullCallback;
        }
    }

    if (so->flags & 1) {
        TCPSocket* prev = so->unk18;
        TCPSocket* next = so->unk1C;

        if (prev == NULL) {
            TCPInfoQueue.tail = next;
        } else {
            prev->unk1C = next;
        }
        if (next == NULL) {
            TCPInfoQueue.head = prev;
        } else {
            next->unk18 = prev;
        }
        so->unk00[0] = 0;
        IPClose(so);
    }

    if (so->unk2D8 != NULL) {
        *so->unk2D8 = so->error;
        so->unk2D8 = NULL;
    }

    if (so->recvArg != NULL) {
        *so->recvArg = so->error;
        so->recvArg = NULL;
    }
    if (so->recvOutputArg != NULL) {
        *so->recvOutputArg = so->error;
        so->recvOutputArg = NULL;
    }
    if (so->urgentArg != NULL) {
        *so->urgentArg = so->error;
        so->urgentArg = NULL;
    }
    if (so->closeArg != 0) {
        *so->closeArg = 0;
        so->closeArg = NULL;
    }

    handlers[0] = (void (*)(TCPSocket*, s32))so->unk2D4;
    so->unk2D4 = NULL;
    handlers[1] = (void (*)(TCPSocket*, s32))so->recvCallback;
    so->recvCallback = NULL;
    handlers[2] = so->recvOutputCallback;
    so->recvOutputCallback = NULL;
    handlers[3] = so->urgentCallback;
    so->urgentCallback = NULL;
    handlers[4] = so->closeCallback;
    so->closeCallback = NULL;

    {
        s32 i;
        for (i = 0; i < 4; i++) {
            void (*handler)(TCPSocket*, s32) = handlers[i];

            if (handler != NULL) {
                handler(so, so->error);
            }
        }
        if (handlers[4] != NULL) {
            handlers[4](so, 0);
        }
    }

    if (so->unk04 > 0) {
        __IPWakeupPollingThreads();
    }
    return 1;
}

s32 TCPSetSendBuff(TCPSocket* so, u8* buffer, u32 size)
{
    BOOL enabled;
    s32 result;

    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP || so->state != 0) {
        result = SO_ENOMEM;
    } else {
        so->unk178 = size;
        result = 0;
        so->unk174 = (s32)buffer;
        so->ringBase = (s32)buffer;
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPSetRecvBuff(TCPSocket* so, u8* buffer, u32 size)
{
    BOOL enabled;
    s32 result;

    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP || so->state != 0) {
        result = SO_ENOMEM;
    } else {
        so->unk1FC = size;
        result = 0;
        so->unk1F8 = (u32)buffer;
        so->unk204 = (u32)buffer;
        so->unk58 = so->unk1FC;
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPGetSendBuff(TCPSocket* so, u8** buffer, u32* size)
{
    BOOL enabled;
    s32 result;

    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP) {
        result = SO_ENOMEM;
    } else {
        result = 0;
        if (size != NULL) {
            *size = so->unk178;
        }
        if (buffer != NULL) {
            *buffer = (u8*)so->unk174;
        }
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPGetRecvBuff(TCPSocket* so, u8** buffer, u32* size)
{
    BOOL enabled;
    s32 result;

    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP) {
        result = SO_ENOMEM;
    } else {
        result = 0;
        if (size != NULL) {
            *size = so->unk1FC;
        }
        if (buffer != NULL) {
            *buffer = (u8*)so->unk1F8;
        }
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPOpen(TCPSocket* so, u8* send, u32 sendSize, u8* recv, u32 recvSize)
{
    BOOL enabled;
    s64 tick;
    s32 member;

    if (so == NULL) {
        return SO_ENOMEM;
    }

    enabled = OSDisableInterrupts();
    member = __IPIsMember(&TCPInfoQueue, so);
    OSRestoreInterrupts(enabled);
    if (member != 0) {
        return -5;
    }

    memset(so, 0, 0x330);
    so->unk00[0] = IP_PROTO_TCP;
    so->unk00[2] = 0;
    so->unk00[1] = 0xFF;
    so->unk00[3] = 1;
    so->unk04 = 0;
    *(u16*)&so->unk06[0] = 0x8000;
    so->unk06[2] = 8;
    so->unk06[3] = 2;
    so->unk0C[4] = 8;
    so->unk0C[5] = 2;
    OSInitThreadQueue((OSThreadQueue*)so->unk20);
    so->state = 0;
    so->error = 0;
    so->mss = 536;
    so->cwnd = so->mss;
    so->ssthresh = 0xFFFF;
    tick = OSGetTime();
    so->lastSend = tick;
    so->flags = 0x82;
    so->snd_wnd = 536;

    TCPSetSendBuff(so, send, sendSize);
    TCPSetRecvBuff(so, recv, recvSize);

    so->idleTimeout.value = (s64)(OS_BUS_CLOCK / 4) * 100;

    TCPInitRtt(so);
    OSCreateAlarm(&so->rxmitAlarm);
    *(s32*)((u8*)so + 0x2DC) = 0;
    OSCreateAlarm((OSAlarm*)((u8*)so + 0x2E0));
    *(s32*)((u8*)so + 0x2C8) = 0;
    *(s32*)((u8*)so + 0x2C4) = 0;
    *(s32*)((u8*)so + 0x310) = 0;
    *(s32*)((u8*)so + 0x30C) = 0;
    *(s32*)((u8*)so + 0x318) = 0;
    *(s32*)((u8*)so + 0x314) = 0;
    *(s32*)((u8*)so + 0x324) = 0;
    so->ip.verlen = 0x45;
    so->ip.tos = so->unk00[2];
    so->ip.len = ((so->ip.verlen & 0xF) << 2) + 20;
    so->ip.ttl = so->unk00[1];
    so->ip.proto = IP_PROTO_TCP;
    so->ip.frag = 0x4000;

    enabled = OSDisableInterrupts();
    {
        TCPSocket* tail = TCPInfoQueue.tail;

        if (tail == NULL) {
            TCPInfoQueue.head = so;
        } else {
            tail->unk18 = so;
        }
        so->unk1C = tail;
        so->unk18 = NULL;
        TCPInfoQueue.tail = so;
    }
    OSRestoreInterrupts(enabled);
    return 0;
}

s32 TCPListen(TCPSocket* so, u8* addr, s32 localPort,
              void (*callback)(TCPSocket*, s32), s32* arg)
{
    BOOL enabled;
    s32 result;

    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP) {
        result = SO_ENOMEM;
    } else if (so->state != 0) {
        result = -5;
    } else {
        if (so->localPort == 0) {
            so->localPort = IPGetAnonPort(&TCPInfoQueue, &Port);
        }
        if (so->localPort == 0) {
            result = -7;
        } else {
            so->state = 1;
            result = 0;
            so->unk2D4 = (s32 (*)(TCPSocket*, s32))(callback != NULL
                                                       ? callback
                                                       : NullCallback);
            so->unk2D8 = arg;
            so->remoteAddrInfo = addr;
            so->localAddrInfo = (u8*)localPort;
        }
    }
    if (arg != NULL) {
        *arg = result;
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPAcceptAsync(TCPSocket* so, TCPSocket* listener,
                   void (*callback)(TCPSocket*, s32), s32* arg)
{
    s32 result;
    BOOL enabled;

    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP || so->unk178 == 0 || so->unk1FC == 0) {
        result = SO_ENOMEM;
    } else if (so->state != 0) {
        result = -5;
    } else if (listener->state != 1) {
        result = SO_EINTR;
    } else {
        TCPSocket* prev;
        TCPSocket* next;
        TCPSocket* pending;

        result = 0;
        callback = callback != NULL ? callback : NullCallback;
        prev = so->unk18;
        next = so->unk1C;

        if (prev == NULL) {
            TCPInfoQueue.tail = next;
        } else {
            prev->unk1C = next;
        }
        if (next == NULL) {
            TCPInfoQueue.head = prev;
        } else {
            next->unk18 = prev;
        }

        so->unk2B8 = listener;
        so->state = 1;
        so->unk2D4 = (s32 (*)(TCPSocket*, s32))callback;
        so->unk2D8 = arg;

        pending = listener->unk2C8;
        if (pending == NULL) {
            listener->unk2C4 = so;
        } else {
            pending->unk2CC = so;
        }
        so->unk2D0 = pending;
        so->unk2CC = NULL;
        listener->unk2C8 = so;
    }
    if (arg != NULL) {
        *arg = (result == 0) ? -1 : result;
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPAccept(TCPSocket* so)
{
}

s32 TCPConnectAsync(TCPSocket* so)
{
}

s32 TCPConnect(TCPSocket* so)
{
}

static s32 SendAsync(TCPSocket* so, const void* buf, s32 len, s32 flags,
                     void (*callback)(TCPSocket*, s32), s32* arg)
{
    BOOL enabled;
    s32 result;
    u32 urgent;

    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP || len < 0) {
        result = SO_ENOMEM;
    } else if (so->recvCallback != NULL) {
        result = -1;
    } else if ((urgent = flags & 1) && len == 0) {
        result = SO_ENOMEM;
    } else {
        result = 0;
        callback = callback ? callback : NullCallback;
        switch (so->state) {
        case 0:
            result = SO_EINTR;
            break;
        case 1:
            result = SO_EWOULDBLOCK;
            break;
        case 2:
        case 3:
        case 4:
        case 7:
            if (so->flags & 8) {
                result = SO_EBADF;
                break;
            }
            so->recvWritten = 0;
            so->data = (u8*)buf;
            so->dataLen = len;
            so->recvCallback = (void*)callback;
            so->recvArg = arg;
            if (arg != NULL) {
                *arg = -1;
            }
            TCPSendIn(so, flags & 4);
            if (urgent) {
                so->unk3C = so->snd_una + so->ringSize + so->dataLen;
                if (so->state < 4 && so->unk48 == so->snd_una) {
                    so->unk3C++;
                }
            }
            if (so->dataLen <= 0) {
                if (so->recvArg != NULL) {
                    *so->recvArg = so->recvWritten;
                    so->recvArg = NULL;
                }
                so->recvCallback = NULL;
                callback(so, so->recvWritten);
            }
            if (so->state == 4 || so->state == 7) {
                TCPOutput(so, 0);
            }
            break;
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        default:
            result = SO_EBADF;
            break;
        }
    }
    if (arg != NULL && result < 0) {
        *arg = result;
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPSendAsync(TCPSocket* so)
{
}

s32 TCPSendNonblock(TCPSocket* so, const void* buf, s32 len)
{
    s32 result;
    s32 output;

    result = SendAsync(so, buf, len, 4, NULL, &output);
    if (result == 0) {
        result = output;
    }
    if (result == 0 && len > 0) {
        result = -9;
    }
    return result;
}

s32 TCPSendUrgAsync(TCPSocket* so)
{
}

s32 TCPSendUrgNonblock(TCPSocket* so, const void* buf, s32 len)
{
    s32 result;
    s32 output;

    result = SendAsync(so, buf, len, 5, NULL, &output);
    if (result == 0) {
        result = output;
    }
    if (result == 0 && len > 0) {
        result = -9;
    }
    return result;
}

s32 Send(TCPSocket* so)
{
}

s32 TCPSend(TCPSocket* so, const void* buf, s32 len)
{
    s32 result;
    s32 output;
    BOOL enabled;

    result = SendAsync(so, buf, len, 0, SyncCallback, &output);
    if (result < 0) {
        return result;
    }
    enabled = OSDisableInterrupts();
    while (output == -1) {
        OSSleepThread((OSThreadQueue*)so->unk20);
    }
    OSRestoreInterrupts(enabled);
    return output;
}

s32 TCPSendUrg(TCPSocket* so, const void* buf, s32 len)
{
    s32 result;
    s32 output;
    BOOL enabled;

    result = SendAsync(so, buf, len, 1, SyncCallback, &output);
    if (result < 0) {
        return result;
    }
    enabled = OSDisableInterrupts();
    while (output == -1) {
        OSSleepThread((OSThreadQueue*)so->unk20);
    }
    OSRestoreInterrupts(enabled);
    return output;
}

s32 TCPReceiveExAsync(TCPSocket* so, void* buf, s32 len, s32 flags,
                             void (*callback)(TCPSocket*, s32), s32* arg)
{
    BOOL enabled;
    s32 result;

    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP || len < 0) {
        result = SO_ENOMEM;
    } else if (so->recvOutputCallback != NULL && !(flags & 4)) {
        result = -1;
    } else if (so->flags & 0x11) {
        result = SO_EBADF;
    } else {
        result = 0;
        callback = callback ? callback : NullCallback;
        switch (so->state) {
        case 0:
            if (so->flags & 8) {
                result = 0;
                if (arg != NULL) {
                    *arg = 0;
                }
                callback(so, 0);
            } else {
                result = SO_EINTR;
            }
            break;
        case 1:
        case 2:
        case 3:
            if (len == 0) {
                result = 0;
                if (arg != NULL) {
                    *arg = 0;
                }
                callback(so, 0);
                break;
            }
            if (flags & 4) {
                result = -9;
                break;
            }
            if (flags & 2) {
                so->flags |= 0x400;
            } else {
                so->flags &= ~0x400;
            }
            so->recvOutputStart = (s32)buf;
            so->recvOutputEnd = len;
            so->recvOutputOffset = 0;
            so->recvOutputCallback = callback;
            so->recvOutputArg = arg;
            if (arg != NULL) {
                *arg = -1;
            }
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            if (len == 0) {
                result = 0;
                if (arg != NULL) {
                    *arg = 0;
                }
                callback(so, 0);
                break;
            }
            if (flags & 2) {
                so->flags |= 0x400;
            } else {
                so->flags &= ~0x400;
            }
            so->recvOutputStart = (s32)buf;
            so->recvOutputEnd = len;
            so->recvOutputOffset = 0;
            result = TCPRecvOut(so);
            if (result > 0) {
                so->recvOutputOffset = 0;
                if (arg != NULL) {
                    *arg = result;
                }
                callback(so, result);
                TCPOutput(so, 0);
                break;
            }
            if (so->state == 7 && so->unk200 == 0) {
                result = 0;
                if (arg != NULL) {
                    *arg = 0;
                }
                callback(so, 0);
                break;
            }
            if (flags & 4) {
                result = -9;
                break;
            }
            so->recvOutputCallback = callback;
            so->recvOutputArg = arg;
            result = 0;
            if (arg != NULL) {
                *arg = -1;
            }
            break;
        case 8:
        case 9:
        case 10:
        default:
            result = 0;
            if (arg != NULL) {
                *arg = 0;
            }
            callback(so, 0);
            break;
        }
    }
    if (arg != NULL && result < 0) {
        *arg = result;
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPReceiveEx(TCPSocket* so, void* buf, s32 len, s32 flags)
{
    s32 result;
    s32 output;
    BOOL enabled;

    result = TCPReceiveExAsync(so, buf, len, flags, SyncCallback, &output);
    if (result < 0) {
        return result;
    }
    enabled = OSDisableInterrupts();
    while (output == -1) {
        OSSleepThread((OSThreadQueue*)so->unk20);
    }
    OSRestoreInterrupts(enabled);
    return output;
}

s32 TCPReceiveAsync(TCPSocket* so)
{
}

s32 TCPReceiveNonblock(TCPSocket* so)
{
}

s32 TCPReceive(TCPSocket* so)
{
}

s32 TCPPeek(TCPSocket* so)
{
}

s32 TCPCloseAsync(TCPSocket* so, void (*callback)(TCPSocket*, s32), s32* arg)
{
    BOOL enabled;
    s32 result;

    enabled = OSDisableInterrupts();
    callback = callback != NULL ? callback : NullCallback;
    if (so->unk00[0] != IP_PROTO_TCP) {
        result = SO_ENOMEM;
    } else if (so->flags & 1) {
        result = SO_EBADF;
    } else if (so->unk200 > 0) {
        so->closeCallback = callback;
        so->closeArg = arg;
        if (arg != NULL) {
            *arg = -1;
        }
        result = TCPCancel(so);
    } else {
        so->flags |= 0x19;
        result = 0;
        so->closeCallback = callback;
        so->closeArg = arg;
        if (arg != NULL) {
            *arg = -1;
        }
        switch (so->state) {
        case 0:
        case 1:
        case 2:
            so->error = SO_EBADF;
            TCPAbort(so);
            break;
        case 3:
            break;
        case 4:
            so->state = 5;
            TCPOutput(so, 0);
            break;
        case 5:
        case 6:
            break;
        case 7:
            so->state = 9;
            TCPOutput(so, 0);
            break;
        case 8:
        case 9:
        case 10:
            break;
        default:
            result = SO_ENOMEM;
            break;
        }
        if (so->unk04 > 0) {
            __IPWakeupPollingThreads();
        }
    }
    if (arg != NULL && result < 0) {
        *arg = result;
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPClose(TCPSocket* so)
{
    s32 result;
    s32 output;
    BOOL enabled;

    result = TCPCloseAsync(so, SyncCallback, &output);
    if (result < 0) {
        return result;
    }
    enabled = OSDisableInterrupts();
    while (output == -1) {
        OSSleepThread((OSThreadQueue*)so->unk20);
    }
    OSRestoreInterrupts(enabled);
    return output;
}

s32 TCPShutdown(TCPSocket* so, u32 how)
{
    BOOL enabled;
    s32 changed;
    s32 result2;
    s32 result;
    void (*fn)(TCPSocket*, s32);

    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP) {
        OSRestoreInterrupts(enabled);
        return SO_ENOMEM;
    }
    changed = 0;
    result = 0;
    result2 = 0;
    if (how == 0 || how == 2) {
        if (so->unk200 > 0) {
            TCPCancel(so);
            OSRestoreInterrupts(enabled);
            return SO_EBADF;
        }
        so->flags |= 0x10;
        switch (so->state) {
        case 0:
            result = SO_EINTR;
            break;
        case 1:
        case 2:
        case 3:
            {
                fn = so->recvOutputCallback;

                if (fn != NULL) {
                    if (so->recvOutputArg != NULL) {
                        *so->recvOutputArg = SO_EBADF;
                        so->recvOutputArg = NULL;
                    }
                    so->recvOutputCallback = NULL;
                    fn(so, SO_EBADF);
                }
            }
            fn = so->urgentCallback;
            if (fn != NULL) {
                if (so->urgentArg != NULL) {
                    *so->urgentArg = SO_EBADF;
                    so->urgentArg = NULL;
                }
                so->urgentCallback = NULL;
                fn(so, SO_EBADF);
            }
            so->unk200 = 0;
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            {
                fn = so->recvOutputCallback;

                if (fn != NULL) {
                    if (so->recvOutputArg != NULL) {
                        *so->recvOutputArg = SO_EBADF;
                        so->recvOutputArg = NULL;
                    }
                    so->recvOutputCallback = NULL;
                    fn(so, SO_EBADF);
                }
            }
            fn = so->urgentCallback;
            if (fn != NULL) {
                if (so->urgentArg != NULL) {
                    *so->urgentArg = SO_EBADF;
                    so->urgentArg = NULL;
                }
                so->urgentCallback = NULL;
                fn(so, SO_EBADF);
            }
            if (so->unk200 > 0) {
                so->unk200 = 0;
                changed = 1;
            }
            break;
        case 8:
        case 9:
        case 10:
        default:
            result = SO_EBADF;
            break;
        }
    }

    if (how == 1 || how == 2) {
        so->flags |= 8;
        switch (so->state) {
        case 0:
            result2 = SO_EINTR;
            break;
        case 1:
        case 2:
            so->error = SO_EBADF;
            TCPAbort(so);
            break;
        case 3:
            break;
        case 4:
            so->state = 5;
            changed = 1;
            break;
        case 5:
        case 6:
            result2 = SO_EBADF;
            break;
        case 7:
            so->state = 9;
            changed = 1;
            break;
        case 8:
        case 9:
        case 10:
        default:
            result2 = SO_EBADF;
            break;
        }
        if (so->unk04 > 0) {
            __IPWakeupPollingThreads();
        }
    }

    if (changed) {
        TCPOutput(so, 0);
    }
    OSRestoreInterrupts(enabled);
    if (result2 < result) {
        return result2;
    }
    return result;
}

s32 TCPCancel(TCPSocket* so)
{
    BOOL enabled;
    s32 result;

    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP) {
        result = SO_ENOMEM;
    } else {
        result = 0;
        so->flags |= 0x19;
        so->error = -3;
        switch (so->state) {
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            TCPRespond(so->iface, NULL, so->connRemoteAddr, so->remotePort,
                       so->unk0C, so->localPort, so->snd_max, 0, 4, 0);
            break;
        case 8:
        case 9:
        case 10:
            break;
        }
        TCPAbort(so);
        if (so->unk04 > 0) {
            __IPWakeupPollingThreads();
        }
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPReceiveUrgExAsync(TCPSocket* so, void* buf, s32 len, s32 flags,
                                void (*callback)(TCPSocket*, s32), s32* arg)
{
    BOOL enabled;
    s32 result;

    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP || len < 1) {
        result = SO_ENOMEM;
    } else if (so->urgentCallback != NULL) {
        result = -1;
    } else if (so->flags & 0x11) {
        result = SO_EBADF;
    } else {
        result = 0;
        callback = callback ? callback : NullCallback;
        switch (so->state) {
        case 0:
            if (so->flags & 8) {
                result = 0;
            } else {
                result = SO_EINTR;
            }
            break;
        case 1:
        case 2:
        case 3:
            if (flags & 4) {
                result = SO_ENOMEM;
                break;
            }
            if (flags & 2) {
                so->flags |= 0x800;
            } else {
                so->flags &= ~0x800;
            }
            so->urgentData = (u8*)buf;
            so->urgentCallback = callback;
            so->urgentArg = arg;
            if (arg != NULL) {
                *arg = -1;
            }
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            if (flags & 2) {
                so->flags |= 0x800;
            } else {
                so->flags &= ~0x800;
            }
            so->urgentData = (u8*)buf;
            if (so->urgentLen > 0 || (so->flags & 0x20)) {
                if (!(so->flags & 0x40)) {
                    if (so->flags & 0x20) {
                        result = 1;
                        if (buf != NULL) {
                            *so->urgentData = so->urgentByte;
                        }
                        if (so->flags & 0x800) {
                            so->flags ^= 0x60;
                        }
                        if (arg != NULL) {
                            *arg = 1;
                        }
                        callback(so, 1);
                        break;
                    }
                    result = -9;
                    break;
                }
            }
            if (so->state == 7) {
                result = 0;
                if (arg != NULL) {
                    *arg = 0;
                }
                callback(so, 0);
                break;
            }
            if (flags & 4) {
                result = SO_ENOMEM;
                break;
            }
            so->urgentCallback = callback;
            so->urgentArg = arg;
            result = 0;
            if (arg != NULL) {
                *arg = -1;
            }
            break;
        case 8:
        case 9:
        case 10:
        default:
            result = 0;
            if (arg != NULL) {
                *arg = 0;
            }
            callback(so, 0);
            break;
        }
    }
    if (arg != NULL && result < 0) {
        *arg = result;
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPReceiveUrgAsync(TCPSocket* so)
{
}

s32 TCPReceiveUrgNonblock(TCPSocket* so)
{
}

s32 TCPReceiveUrgEx(TCPSocket* so, void* buf, s32 len, s32 flags)
{
    s32 result;
    s32 output;
    BOOL enabled;

    result = TCPReceiveUrgExAsync(so, buf, len, flags, SyncCallback, &output);
    if (result < 0) {
        return result;
    }
    enabled = OSDisableInterrupts();
    while (output == -1) {
        OSSleepThread((OSThreadQueue*)so->unk20);
    }
    OSRestoreInterrupts(enabled);
    return output;
}

s32 TCPReceiveUrg(TCPSocket* so)
{
}

s32 TCPPeekUrg(TCPSocket* so)
{
}

s32 TCPGetUrgOffset(TCPSocket* so)
{
}

s32 TCPGetSockOpt(TCPSocket* so, s32 level, s32 opt, void* val, s32* len)
{
    BOOL enabled;
    s32 result;

    result = -14;
    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP) {
        result = SO_ENOMEM;
    } else if (level == 0xFFFF) {
        switch (opt) {
        case 4:
            if ((u32)*len >= 4) {
                *(s32*)val = (so->flags & 0x10000) ? 1 : 0;
                *len = 4;
                result = 0;
            } else {
                result = SO_ENOMEM;
            }
            break;
        case 256:
            if ((u32)*len >= 4) {
                *(s32*)val = (so->flags & 0x80) ? 0 : 1;
                *len = 4;
                result = 0;
            } else {
                result = SO_ENOMEM;
            }
            break;
        case 4104:
            if ((u32)*len >= 4) {
                *(s32*)val = 1;
                *len = 4;
                result = 0;
            } else {
                result = SO_ENOMEM;
            }
            break;
        case 128:
            if ((u32)*len >= 8) {
                *(s32*)val = (so->flags & 0x20000) ? 1 : 0;
                ((s32*)val)[1] = so->unk2DC;
                *len = 8;
                result = 0;
            } else {
                result = SO_ENOMEM;
            }
            break;
        }
    } else if (level == 0) {
        result = IPGetSockOpt(so, level, opt, val, len);
    } else if (level == IP_PROTO_TCP) {
        switch (opt) {
        case 8193:
            if ((u32)*len >= 4) {
                *(s32*)val = (so->flags & 2) ? 0 : 1;
                *len = 4;
                result = 0;
            } else {
                result = SO_ENOMEM;
            }
            break;
        case 8194:
            if ((u32)*len >= 4) {
                *(s32*)val = so->mss;
                *len = 4;
                result = 0;
            } else {
                result = SO_ENOMEM;
            }
            break;
        }
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPSetSockOpt(TCPSocket* so, s32 level, s32 opt, void* val, u32 len)
{
    BOOL enabled;
    s32 result;

    result = -14;
    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP) {
        result = SO_ENOMEM;
    } else if (level == 0xFFFF) {
        switch (opt) {
        case 4:
            if (len >= 4u) {
                if (*(s32*)val != 0) {
                    so->flags |= 0x10000;
                } else {
                    so->flags &= ~0x10000;
                }
                result = 0;
            } else {
                result = SO_ENOMEM;
            }
            break;
        case 256:
            if (len >= 4u) {
                if (*(s32*)val != 0) {
                    so->flags &= ~0x80;
                } else {
                    so->flags |= 0x80;
                }
                result = 0;
            } else {
                result = SO_ENOMEM;
            }
            break;
        case 128:
            if (len >= 8u) {
                if (*(s32*)val != 0) {
                    so->flags |= 0x20000;
                } else {
                    so->flags &= ~0x20000;
                }
                {
                    s32 linger = ((s32*)val)[1];

                    so->unk2DC = linger > 0 ? linger : 0;
                }
                result = 0;
            } else {
                result = SO_ENOMEM;
            }
            break;
        }
    } else if (level == 0) {
        result = IPSetSockOpt(so, level, opt, val, len);
    } else if (level == IP_PROTO_TCP) {
        switch (opt) {
        case 8193:
            if (len >= 4u) {
                if (*(s32*)val != 0) {
                    so->flags &= ~2;
                } else {
                    so->flags |= 2;
                }
                result = 0;
            } else {
                result = SO_ENOMEM;
            }
            break;
        }
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPSetTimeout(TCPSocket* so, s32 unused, s32 rxt, s32 idle)
{
    BOOL enabled;
    s32 result;

    enabled = OSDisableInterrupts();
    if (so->unk00[0] != IP_PROTO_TCP) {
        result = SO_ENOMEM;
    } else {
        so->idleTimeout.words.lo = idle;
        so->idleTimeout.words.hi = rxt;
        result = 0;
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 TCPControlNagle(TCPSocket* so)
{
}

s32 TCPSetUrgInLine(TCPSocket* so)
{
}

s32 TCPSetOption(TCPSocket* so)
{
}

BOOL TCPOnReset(BOOL final)
{
    TCPSocket* so;

    if (TCPInfoQueue.head == NULL) {
        return TRUE;
    }
    while (TCPInfoQueue.head != NULL) {
        so = TCPInfoQueue.head;
        TCPCancel(so);
    }
    return FALSE;
}

s32 __TCPPoll(TCPSocket* so)
{
    s32 state;
    s32 state2;
    s32 flags;

    if (so->unk00[0] != IP_PROTO_TCP) {
        return 0x80;
    }
    state = *(u32*)&so->state;
    flags = 0;
    if (state == 1 && *(u32*)((u8*)so + 0x314) != 0) {
        flags |= 1;
    }
    state2 = so->state;
    switch (state2) {
    case 0:
        flags |= 8;
        break;
    case 1:
        break;
    case 2:
    case 3:
    case 4:
    case 7:
        if (so->dataLen <= 0 && so->unk178 - so->ringSize > 0) {
            flags |= 0x18;
        }
        break;
    case 5:
    case 6:
    case 8:
    case 9:
    case 10:
    default:
        flags |= 8;
        break;
    }

    switch (state2) {
    case 0:
        flags |= 1;
        break;
    case 1:
    case 2:
    case 3:
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        if (so->unk200 > 0) {
            flags |= 1;
        } else if (state == 7) {
            flags |= 1;
        }
        if (so->urgentLen > 0 || (so->flags & 0x20)) {
            if (!(so->flags & 0x40)) {
                flags |= 2;
            }
        }
        break;
    case 8:
    case 9:
    case 10:
    default:
        flags |= 1;
        break;
    }

    if (so->flags & 8) {
        flags |= 8;
    }
    if (so->flags & 0x10) {
        flags |= 1;
    }
    if (state == 0 && (so->flags & 0x18) == 0x18 && so->error != 0) {
        flags |= 0x20;
    }
    return flags;
}
