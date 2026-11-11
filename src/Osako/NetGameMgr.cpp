#include "Osako/NetGameMgr.h"
#include "Osako/BBAMgr.h"
#include "Osako/ErrorViewApp.h"
#include "Osako/system.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "dolphin/os.h"
#include <string.h>

extern "C" {
s32 IPGetConfigError(s32);
int HTTPGetStatus(const char *);
int HTTPGetHeader(const char *, int, char *, int);
int UPnPScanUuid(char *, char *);
int SSDPGetCacheControlMaxAge(const char *);
char *SSDPGetServerName(const char *);
}

static const char sNotifyStrings[] =
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "NetGameMgr.cpp\0"
    "cno(%d),pos\0"
    "halt(%d)\0"
    "\0\0\0\0"
    "::upnp:rootdevice\0"
    "\0\0"
    "ssdp:alive\0"
    "s"
    "ssdp:byebye\0";

NetGameMgr *NetGameMgr::mspNetGameMgr;

void NetGameMgr::create(JKRHeap *heap)
{
    if (mspNetGameMgr == nullptr)
    {
        mspNetGameMgr = new (heap, 0) NetGameMgr();
    }
}
NetGameMgr::NetGameMgr()
{
    mUnk984 = 0;
    f32 zero = 0.0f;
    mUnk988 = 0;
    mState = 0;
    *(f32 *)this = zero;
    mNetworkCubes = 1;
    mNetworkNum = 0;
}
void NetGameMgr::constructFifo()
{
    u32 zero = 0;
    for (u8 i = 0; i < 8; i++)
    {
        u8 idx = i;
        mPadConnArray[idx]._4 = zero;
        mPadConnArray[idx].mNext = nullptr;
        for (u8 j = 0; j < 5; j++)
        {
            mPadData[idx][j].mType = 0;
            mPadData[idx][j].mId = j;
        }
        for (u8 j = 0; j < 4; j++)
        {
            mPadData[idx][j].mNext = &mPadData[idx][j + 1];
            mPadData[idx][j].mPrev = &mPadData[idx][j];
        }
        mPadData[idx][4].mNext = &mPadData[idx][0];
        mPadData[idx][0].mPrev = &mPadData[idx][4];
    }

    for (u8 i = 0; i < 2; i++)
    {
        u8 idx = i;
        mPadDataLocal[idx].mType = 0;
        mPadDataLocal[idx].mNext = &mPadDataLocal[idx + 1];
        mPadDataLocal[idx + 1].mPrev = &mPadDataLocal[idx];
    }
    mPadDataLocal[2].mType = 0;
    mPadDataLocal[0].mPrev = &mPadDataLocal[2];
    mPadDataLocal[2].mNext = &mPadDataLocal[0];
    mPadDataNext = &mPadDataLocal[0];

    for (u8 i = 0; i < 2; i++)
    {
        u8 idx = i;
        *(u32 *)&mSendPkt[idx] = 0;
        mSendPkt[idx].mNext = &mSendPkt[idx + 1];
        mSendPkt[idx + 1].mPrev = &mSendPkt[idx];
    }
    *(u32 *)&mSendPkt[2] = 0;
    mSendPkt[0].mPrev = &mSendPkt[2];
    mSendPkt[2].mNext = &mSendPkt[0];
    mSendPktNext = &mSendPkt[0];
}
void NetGameMgr::initPadConv()
{
    for (int i = 0; i < 8; i++)
    {
        mPadConvData[i] = i * 4;
        mPadConvData[i + 8] = i * 4 + 1;
    }
}
void NetGameMgr::start()
{
    if (mState != 0)
    {
        return;
    }

    _970 = 0;
    _971 = 0;
    _972 = 0;
    mSleepReq = 0;
    _12ee = 0;
    mPadConnHead = nullptr;
    _12e9[0] = OSGetTick();
    constructFifo();

    mGCInfoNum = 0;
    _a99[0] = 0;
    int i = 0;
    for (; i < 16; i++)
    {
        GCInfo *info = (GCInfo *)((u8 *)mGCInfo + i * 16);
        info->mAddr = 0;
        info->mFlag = 0;
        info->_5[0] = 0;
        info->_5[1] = 0;
        info->_5[2] = 0;
        info->_8 = 0;
        info->_a = 0;
        info->_c = 0;
    }

    for (i = 0; i < 8; i++)
    {
        mPadConvData[i] = i * 4;
        mPadConvData[i + 8] = i * 4 + 1;
    }

    u8 *p;
    for (u8 n = 0; n < 2; n++)
    {
        p = (u8 *)this + n * 148;
        p[0xb29] = 2;
        p[0xb28] = 8;
        *(u16 *)(p + 0xb2a) = 0x6790;
    }

    *(u8 *)&mRecvPkt.mAddr = 8;
    initLogBuffer();
    mState = 1;
    doStart();
}
void NetGameMgr::end(bool flag)
{
    if (mState == 0x14 && !flag)
    {
        return;
    }

    BOOL enabled = OSDisableInterrupts();
    _974 = flag;
    PadMgr::setKartPadInput(PadMgr::Input_Local);
    mState = 0x14;
    OSRestoreInterrupts(enabled);
}
void NetGameMgr::doStart()
{
    if (BBAMgr::mspBBAMgr->getState() == 2)
    {
        mState = 2;
    }
}
void NetGameMgr::startSearch()
{
    BBAMgr::mspBBAMgr->startMSearch();
    mState = 7;
}
void NetGameMgr::distributePadData(NetPadPkt &pkt)
{
    u8 *base = (u8 *)&pkt;
    NetPadConn *conn = mPadConnHead;
    for (u8 i = 0; i < mNetworkCubes; i++)
    {
        NetPadData *data = conn->mData;
        for (u8 j = 0; j < 5; j++)
        {
            if (data->mType == base[3])
            {
                data->mType = 1;
                *(u32 *)((u8 *)data + 8) = *(u32 *)(base + i * 16 + 8);
                *(u32 *)((u8 *)data + 12) = *(u32 *)(base + i * 16 + 12);

                NetPadData *prev = data->mPrev;
                if (prev->mId == (u8)(base[3] - 1) && prev->mType != 1)
                {
                    prev->mType = 1;
                    *(u32 *)((u8 *)prev + 8) = *(u32 *)(base + i * 16 + 16);
                    *(u32 *)((u8 *)prev + 12) = *(u32 *)(base + i * 16 + 20);
                }
            }
            else
            {
                data = data->mNext;
            }
        }
        conn = conn->mNext;
    }

    if (_972 != 0)
    {
        NetPadData *data = mPadDataNext;
        for (u8 i = 0; i < 3; i++)
        {
            if (data->mType == 1 && data->mId == base[3] && memcmp(&data->mPad[0][0], base + 8 + mNetworkCubes * 16, 8) != 0)
            {
                if (mState != 0x14)
                {
                    BOOL enabled = OSDisableInterrupts();
                    _974 = 0;
                    PadMgr::setKartPadInput(PadMgr::Input_Local);
                    mState = 0x14;
                    OSRestoreInterrupts(enabled);
                }
                ErrorViewApp::call(ErrorViewApp::ERROR_LA_TRANSFER);
                break;
            }
            data = data->mPrev;
        }
    }
}
void NetGameMgr::recvTask(void *arg)
{
    int ret;
    ret = BBAMgr::mspBBAMgr->recvFrom(&mspNetGameMgr->mRecvPkt, 136, &mspNetGameMgr->mRecvPkt.mAddr);

    if (ret < 0)
    {
        NetGameMgr *mgr = mspNetGameMgr;
        if (mgr->mState != 0x14)
        {
            BOOL enabled = OSDisableInterrupts();
            mgr->_974 = 0;
            PadMgr::setKartPadInput(PadMgr::Input_Local);
            mgr->mState = 0x14;
            OSRestoreInterrupts(enabled);
        }
        if (ret == -56)
        {
            if (mgr->mState != 0x14)
            {
                ErrorViewApp::call(ErrorViewApp::ERROR_LA_CONNECTION);
            }
        }
        else
        {
            ErrorViewApp::call(ErrorViewApp::ERROR_LA_ROUTER);
        }
        return;
    }

    mspNetGameMgr->mRecvPkt.mLength = ret;
    mspNetGameMgr->recv();
    System::mspRecvTask->request(recvTask, nullptr, nullptr);
}
void NetGameMgr::sendTask(void *arg)
{
    int ret;
    int idx = (int)arg;
    ret = BBAMgr::mspBBAMgr->sendTo(&mspNetGameMgr->mSendBuf[idx], mspNetGameMgr->mSendBuf[idx].mLength, &mspNetGameMgr->mSendBuf[idx].mAddr);

    if (ret < 0)
    {
        NetGameMgr *mgr = mspNetGameMgr;
        if (mgr->mState != 0x14)
        {
            BOOL enabled = OSDisableInterrupts();
            mgr->_974 = 0;
            PadMgr::setKartPadInput(PadMgr::Input_Local);
            mgr->mState = 0x14;
            OSRestoreInterrupts(enabled);
        }
        if (ret == -38)
        {
            ErrorViewApp::call(ErrorViewApp::ERROR_LA_CONNECTION);
        }
        else
        {
            ErrorViewApp::call(ErrorViewApp::ERROR_LA_ROUTER);
        }
        return;
    }

    mspNetGameMgr->logNet((NetActionType)idx, mspNetGameMgr->mSendBuf[idx]);
}
void NetGameMgr::retraceCallback(u32)
{
    if (mspNetGameMgr != nullptr)
    {
        mspNetGameMgr->syncwork();
    }
}
void NetGameMgr::callbackNotify(const char *url, const SOInAddr *addr)
{
    const char *msg = sNotifyStrings;
    if (url == nullptr && addr == nullptr)
    {
        return;
    }

    BBAMgr *bba = BBAMgr::mspBBAMgr;
    if ((addr->mAddr & bba->mNetmask) != (bba->mHostAddr & bba->mNetmask))
    {
        return;
    }

    struct
    {
        char mNts[16];
        char mUuid[16];
        char mServer[64];
        char mHeader[80];
    } b;

    s8 ok = HTTPGetHeader(url, 0, b.mHeader, sizeof(b.mHeader)) > 0
        && strnicmp(b.mHeader, "uuid:", 5) == 0
        && UPnPScanUuid(b.mHeader + 5, b.mUuid) >= 0
        && stricmp(&b.mHeader[0x29], msg + 56) == 0;

    if (ok == 0)
    {
        return;
    }

    if (HTTPGetHeader(url, 0, b.mNts, sizeof(b.mNts)) <= 0)
    {
        return;
    }

    if (stricmp(b.mNts, msg + 76) == 0)
    {
        if (HTTPGetHeader(url, 0, b.mServer, sizeof(b.mServer)) > 0 && strncmp(SSDPGetServerName(url), b.mServer, 39) == 0 && SSDPGetCacheControlMaxAge(url) > 0)
        {
            mspNetGameMgr->appendGCInfo(addr);
        }
    }
    else if (stricmp(b.mNts, msg + 88) == 0)
    {
        mspNetGameMgr->removeGCInfo(addr);
    }
}

void NetGameMgr::callbackSearch(const char *url, const SOInAddr *addr, const SSDPSearchInfo *)
{
    const char *msg = sNotifyStrings;

    if (url == nullptr && addr == nullptr)
    {
        if (mspNetGameMgr->mState != 7)
        {
            return;
        }
        System::mspRecvTask->request((void (*)(void *))recvTask, nullptr, nullptr);
        mspNetGameMgr->mState = 8;
        return;
    }

    BBAMgr *bba = BBAMgr::mspBBAMgr;
    if ((addr->mAddr & bba->mNetmask) != (bba->mHostAddr & bba->mNetmask))
    {
        return;
    }

    char uuid[16];
    char server[64];
    char header[80];

    if (!(HTTPGetStatus(url) == 200
        && HTTPGetHeader(url, 0, header, sizeof(header)) > 0
        && strnicmp(header, "uuid:", 5) == 0
        && UPnPScanUuid(header + 5, uuid) >= 0
        && stricmp(&header[0x29], msg + 56) == 0))
    {
        return;
    }

    if (!(HTTPGetHeader(url, 0, server, sizeof(server)) > 0
        && strncmp(SSDPGetServerName(url), server, 39) == 0
        && SSDPGetCacheControlMaxAge(url) > 0))
    {
        return;
    }

    mspNetGameMgr->appendGCInfo(addr);
}
void NetGameMgr::appendGCInfo(const SOInAddr *addr)
{
    GCInfo *found = nullptr;
    GCInfo *empty = nullptr;
    BOOL enabled = OSDisableInterrupts();

    for (int i = 0; i < 16; i++)
    {
        GCInfo *info = &mGCInfo[i];
        if (empty == nullptr && info->mAddr == 0)
        {
            empty = info;
        }
        else if (info->mAddr == addr->mAddr)
        {
            found = info;
            break;
        }
    }

    if (found == nullptr && empty != nullptr)
    {
        empty->mAddr = addr->mAddr;
        empty->mFlag = 1;
        mGCInfoNum++;
    }

    OSRestoreInterrupts(enabled);
}
void NetGameMgr::removeGCInfo(const SOInAddr *addr)
{
    BOOL enabled = OSDisableInterrupts();

    for (int i = 0; i < 16; i++)
    {
        GCInfo *info = &mGCInfo[i];
        if (info->mAddr == addr->mAddr)
        {
            info->mAddr = 0;
            info->mFlag = 0;
            break;
        }
    }

    OSRestoreInterrupts(enabled);
}
void NetGameMgr::getKartPad(u8 chan, KartPadData *padData)
{
#line 1774
    JUT_MAX_ASSERT(chan, 16);
    getPadData(mPadConvData[chan], padData);
}
void NetGameMgr::getPadData(u8 chan, KartPadData *padData)
{
    int port = chan % 4;
    int idx = chan / 4;

    if (mState == 0xd && idx < mNetworkCubes)
    {
        NetPadData *data = mPadConn[idx]->mData;
        if ((int)data->mType == 1 && _971 >= 1)
        {
            u8 *src = &data->mPad[port][0];
            u8 *dst = (u8 *)padData;
            dst[0] = src[0];
            dst[1] = src[1];
            return;
        }
    }

    memset(padData, 0, sizeof(KartPadData));
}
void NetGameMgr::progressReadData()
{
    if (mState != 0xd)
    {
        return;
    }

    if (_971 < 1)
    {
        _971++;
        return;
    }

    BOOL enabled = OSDisableInterrupts();
    mLogHead->mTick = OSGetTick();
    mLogHead->mAction = 4;
    mLogHead->mType = 0;
    mLogHead->mArg0 = 0;
    mLogHead->mArg1 = 0;
    mLogHead->mArg2 = mPadConn[0]->mData->mId;
    mLogHead = mLogHead->mNext;

    for (int i = 0; i < mNetworkCubes; i++)
    {
        NetPadData *data = mPadConn[i]->mData;
        if (data->mType == 1 && data->mId == (u8)(_970 - 2))
        {
            data->mType = 2;
            data->mId += 5;
            mPadConn[i]->mData = data->mNext;
        }
        else
        {
            if (mState != 0x14)
            {
                BOOL enable = OSDisableInterrupts();
                _974 = 0;
                PadMgr::setKartPadInput(PadMgr::Input_Local);
                mState = 0x14;
                OSRestoreInterrupts(enable);
            }
            ErrorViewApp::call(ErrorViewApp::ERROR_LA_TRANSFER);
            break;
        }
    }

    OSRestoreInterrupts(enabled);
}
void NetGameMgr::fetchPadData()
{
    BOOL enabled = OSDisableInterrupts();
    u8 id = _970;
    NetPadData *data = mPadDataNext->mNext;
    data->mId = id;
    data->mType = 1;

    if (_972 != 0)
    {
        for (u8 i = 0; i < 4; i++)
        {
            PadMgr::getPadData(i, PadMgr::Input_Local, (KartPadData *)&data->mPad[i][0]);
        }
    }
    else
    {
        memset(&data->mPad[0][0], 0, 8);
        if (_970 != 0)
        {
            _972 = 1;
        }
    }

    mPadDataNext = data;
    data->mFrame = mFrame;
    OSRestoreInterrupts(enabled);
}
void NetGameMgr::sleep()
{
    if (mState != 0xd)
    {
        return;
    }
    mSleepReq = 1;
}
void NetGameMgr::awake()
{
    if (mState != 0xf)
    {
        return;
    }

    u8 i = 0;
    mState = 0x10;
    _970 = i;
    _971 = i;
    _97c = i;

    for (i = 0; i < mNetworkCubes; i++)
    {
        u8 j = 0;
        for (; j < 5; j++)
        {
            _d24[i * 0x78 + j * 0x18] = 0;
            _d24[i * 0x78 + j * 0x18 + 1] = j;
        }
        mPadConn[i]->mData = (NetPadData *)&_d24[i * 0x78];
    }

    for (u8 i = 0; i < 3; i++)
    {
        _10e4[i][0] = 0;
    }
    for (u8 i = 0; i < 2; i++)
    {
        *(u32 *)_112c[i] = 0;
    }
}
void NetGameMgr::handleError()
{
    NetGameMgr *mgr = this;
    if (mgr->mState < 1)
    {
        return;
    }
    if (mgr->mState > 17)
    {
        return;
    }

    switch (IPGetConfigError(0))
    {
    case -100:
        if (mgr->mState == 4)
        {
            return;
        }
    case -102:
    case -101:
    {
        BOOL enabled;
        mgr = mspNetGameMgr;
        if (mgr->mState != 0x14)
        {
            enabled = OSDisableInterrupts();
            mgr->_974 = 0;
            PadMgr::setKartPadInput(PadMgr::Input_Local);
            mgr->mState = 0x14;
            OSRestoreInterrupts(enabled);
        }
        ErrorViewApp::call(ErrorViewApp::ERROR_LA_ROUTER);
        break;
    }
    case -111:
    {
        BOOL enabled;
        mgr = mspNetGameMgr;
        if (mgr->mState != 0x14)
        {
            enabled = OSDisableInterrupts();
            mgr->_974 = 0;
            PadMgr::setKartPadInput(PadMgr::Input_Local);
            mgr->mState = 0x14;
            OSRestoreInterrupts(enabled);
        }
        ErrorViewApp::call(ErrorViewApp::ERROR_LA_CONNECTED);
        break;
    }
    case -112:
    case -110:
    case -109:
    case -108:
    case -107:
    case -106:
    case -105:
    case -104:
    case -103:
    {
        BOOL enabled;
        mgr = mspNetGameMgr;
        if (mgr->mState != 0x14)
        {
            enabled = OSDisableInterrupts();
            mgr->_974 = 0;
            PadMgr::setKartPadInput(PadMgr::Input_Local);
            mgr->mState = 0x14;
            OSRestoreInterrupts(enabled);
        }
        ErrorViewApp::call(ErrorViewApp::ERROR_LA_CONNECTION);
        break;
    }
    case 0:
        break;
    }
}
void NetGameMgr::initLogBuffer()
{
    u8 i = 0;
    for (; i < 99; i++)
    {
        mLog[i].mNext = &mLog[i + 1];
        mLog[i + 1].mPrev = &mLog[i];
    }
    mLog[i].mNext = &mLog[0];
    mLog[0].mPrev = &mLog[i];
    mLogHead = &mLog[0];
}
void NetGameMgr::logNet(NetActionType action, NetPkt &pkt)
{
    u8 type = pkt.mType;
    mLogHead->mType = type;
    mLogHead->mTick = OSGetTick();
    mLogHead->mAction = action;

    switch ((s8)type)
    {
    case 2:
        mLogHead->mArg0 = pkt.mArg0;
        mLogHead->mArg1 = pkt.mArg1;
        mLogHead->mArg2 = pkt.mArg2;
        break;
    case 3:
        mLogHead->mArg0 = (pkt.mArg0 >> 4) & 0xf;
        mLogHead->mArg1 = pkt.mArg0 & 0xf;
        mLogHead->mArg2 = pkt.mArg2;
        break;
    case 5:
        mLogHead->mArg0 = pkt.mArg0;
        mLogHead->mArg1 = pkt.mArg1;
        mLogHead->mArg2 = pkt.mArg2;
        break;
    }

    mLogHead->mUnk8 = pkt.mUnk90;
    mLogHead = mLogHead->mNext;
}
