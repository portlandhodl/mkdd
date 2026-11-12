#ifndef _DOLPHIN_TCP_H_
#define _DOLPHIN_TCP_H_

#include <dolphin/ip/ip.h>
#include <dolphin/os/OSAlarm.h>
#include <dolphin/types.h>

typedef struct TCPHeader {
    u16 src;
    u16 dst;
    s32 seq;
    s32 ack;
    u16 flag;
    u16 win;
    u16 sum;
    u16 urg;
} TCPHeader;

typedef struct TCPResetInfo {
    u8 addr[6];
    u8 dstAddr[4];
    u8 srcAddr[4];
    u16 dstPort;
    u16 srcPort;
    s32 seq;
    s32 ack;
    u16 flag;
    u16 id;
} TCPResetInfo;

typedef struct TCPSackEntry {
    u32 start;
    u32 end;
} TCPSackEntry;

typedef struct TCPSackHole {
    s32 unk0;
    s32 start;
    s32 dupAcks;
    s32 end;
} TCPSackHole;

typedef struct TCPSegment {
    void* data;
    s32 len;
} TCPSegment;

typedef struct TCPSocket TCPSocket;
struct TCPSocket {
    u8 unk00[4];
    s16 unk04;
    u8 unk06[4];
    u16 localPort;
    u8 unk0C[6];
    u16 remotePort;
    u8 connRemoteAddr[4];
    struct TCPSocket* unk18;
    struct TCPSocket* unk1C;
    u8 unk20[8];
    void* iface;
    s32 error;
    s32 snd_una;
    s32 snd_nxt;
    s32 snd_wnd;
    s32 unk3C;
    s32 unk40;
    s32 unk44;
    s32 unk48;
    s32 unk4C;
    s32 snd_max;
    s32 unk54;
    s32 unk58;
    s32 unk5C;
    s32 unk60;
    s32 unk64;
    u8* unk68;
    TCPSackEntry sack[4];
    TCPSackHole holes[4];
    s32 holeIndex;
    s32 unkD0;
    s32 unkD4;
    s32 unkD8;
    s32 unkDC;
    s32 unkE0;
    s32 state;
    u32 flags;
    void (*closeCallback)(TCPSocket* so, s32 result);
    s32* closeArg;
    s32 mss;
    s32 outputPending;
    IPHeader ip;
    u8 unk110[100];
    s32 unk174;
    s32 unk178;
    s32 ringBase;
    s32 ringSize;
    u8 packet[24];
    u8 remoteAddr[4];
    u8 unk1A0[9];
    u8 immediate;
    u8 unk1AA[2];
    void (*outputCallback)(TCPSocket* so, s32 result);
    TCPSocket* callbackArg;
    s32 fragmentCount;
    TCPSegment fragments[4];
    void* recvCallback;
    s32* recvArg;
    s32 recvWritten;
    u8* data;
    s32 dataLen;
    u8 unk1EC[4];
    s64 lastSend;
    u32 unk1F8;
    s32 unk1FC;
    s32 unk200;
    u32 unk204;
    s32 rxmitNext;
    s32 numDupAcks;
    void (*recvOutputCallback)(TCPSocket* so, s32 result);
    s32* recvOutputArg;
    s32 recvOutputStart;
    s32 recvOutputEnd;
    s32 recvOutputOffset;
    u8 urgentByte;
    u8 unk225[3];
    s32 urgentLen;
    void (*urgentCallback)(TCPSocket* so, s32 arg);
    s32* urgentArg;
    u8* urgentData;
    s32 nrxmit;
    u8 unk23C[4];
    s64 rxtcur;
    s64 unk248;
    union {
        s64 value;
        struct {
            s32 hi;
            s32 lo;
        } words;
    } idleTimeout;
    OSAlarm rxmitAlarm;
    s32 cwnd;
    s32 ssthresh;
    s32 rttActive;
    s32 rttSeq;
    s64 rttStart;
    s64 srtt;
    s64 rttvar;
    s64 rxtmin;
    s64 rxtmax;
    TCPSocket* unk2B8;
    u8* remoteAddrInfo;
    u8* localAddrInfo;
    TCPSocket* unk2C4;
    TCPSocket* unk2C8;
    TCPSocket* unk2CC;
    TCPSocket* unk2D0;
    s32 (*unk2D4)(TCPSocket* so, s32 arg);
    s32* unk2D8;
    s32 unk2DC;
};

void TCPOutput(TCPSocket* so, u16 flag);
void TCPStartRxmitTimer(TCPSocket* so);
s32 TCPSetSendBuff(TCPSocket* so, u8* buffer, u32 size);
s32 TCPSetRecvBuff(TCPSocket* so, u8* buffer, u32 size);
s32 TCPGetSendBuff(TCPSocket* so, u8** buffer, u32* size);
s32 TCPGetRecvBuff(TCPSocket* so, u8** buffer, u32* size);

#endif
