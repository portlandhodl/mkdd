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
    u8 unk00[10];
    u16 localPort;
    u8 unk0C[6];
    u16 remotePort;
    u8 unk14[24];
    s32 error;
    s32 snd_una;
    s32 snd_nxt;
    s32 snd_wnd;
    s32 unk3C;
    u8 unk40[8];
    s32 unk48;
    s32 unk4C;
    s32 snd_max;
    s32 unk54;
    s32 unk58;
    u8 unk5C[4];
    s32 unk60;
    u8 unk64[8];
    TCPSackEntry sack[4];
    TCPSackHole holes[4];
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
    u8 unk1DC[4];
    u8 unk1E0[4];
    u8* data;
    s32 dataLen;
    u8 unk1EC[4];
    s64 lastSend;
    u8 unk1F8[4];
    s32 unk1FC;
    s32 unk200;
    u32 unk204;
    s32 rxmitNext;
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
    s32 rttSeq;
    s64 rttStart;
    s64 srtt;
    s64 rttvar;
    s64 rxtmin;
    s64 rxtmax;
};

void TCPOutput(TCPSocket* so, u16 flag);
void TCPStartRxmitTimer(TCPSocket* so);

#endif
