#include "Osako/NetGameMgr.h"
#include "Osako/BBAMgr.h"
#include "Osako/ErrorViewApp.h"
#include "Osako/RaceApp.h"
#include "Osako/AppMgr.h"
#include "Osako/SystemRecord.h"
#include "Osako/system.h"
#include "Kaneshige/RaceInfo.h"
#include "Yamamoto/kartCtrl.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JUtility/JUTDbg.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
#include "dolphin/os.h"
#include "kartLocale.h"
#include <string.h>

#ifdef DEBUG
#include "mathHelper.h"
#endif

extern "C" {
s32 IPGetConfigError(s32);
int HTTPGetStatus(const char *);
int HTTPGetHeader(const char *, const char *, char *, int);
int UPnPScanUuid(const char *, UPnPUuid *);
int SSDPGetCacheControlMaxAge(const char *);
const char *SSDPGetServerName();
}

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
    mFrame = zero;
    mNetworkCubes = 1;
    mPadConnArray[0].mIndex = 0;
}
u32 NetGameMgr::getNextConsoleAddr()
{
    return mPadConnArray[0].mNext->mAddr;
}
void NetGameMgr::sendNext(SendPriority prio)
{
    mSendBuf[prio].mAddr.mAddr.mAddr = getNextConsoleAddr();
    System::mspSendTask->request(sendTask, (void *)prio, nullptr);
}
void NetGameMgr::broadcast(SendPriority prio)
{
    mSendBuf[prio].mAddr.mAddr.mAddr = 0xffffffff;
    System::mspSendTask->request(sendTask, (void *)prio, nullptr);
}
void NetGameMgr::constructFifo()
{
    u8 j;
    u8 i;
    for (i = 0; i < 8; i++)
    {
        mPadConnArray[i].mAddr = 0;
        mPadConnArray[i].mNext = nullptr;
        for (j = 0; j < 5; j++)
        {
            mPadData[i][j].mType = 0;
            mPadData[i][j].mId = j;
        }
        for (j = 0; j < 4; j++)
        {
            mPadData[i][j].mNext = &mPadData[i][j + 1];
            mPadData[i][j + 1].mPrev = &mPadData[i][j];
        }
        mPadData[i][j].mNext = &mPadData[i][0];
        mPadData[i][0].mPrev = &mPadData[i][j];
    }

    for (j = 0; j < 2; j++)
    {
        mPadDataLocal[j].mType = 0;
        mPadDataLocal[j].mNext = &mPadDataLocal[j + 1];
        mPadDataLocal[j + 1].mPrev = &mPadDataLocal[j];
    }
    mPadDataLocal[j].mType = 0;
    mPadDataLocal[0].mPrev = &mPadDataLocal[j];
    mPadDataLocal[j].mNext = &mPadDataLocal[0];
    mPadDataNext = &mPadDataLocal[0];

    for (j = 0; j < 2; j++)
    {
        mSendPkt[j].mState = 0;
        mSendPkt[j].mNext = &mSendPkt[j + 1];
        mSendPkt[j + 1].mPrev = &mSendPkt[j];
    }
    mSendPkt[j].mState = 0;
    mSendPkt[0].mPrev = &mSendPkt[j];
    mSendPkt[j].mNext = &mSendPkt[0];
    mSendPktNext = &mSendPkt[0];
}
void NetGameMgr::resetFifo()
{
    u8 i = 0;
    _970 = i;
    _971 = i;
    _97c = i;

    for (; i < mNetworkCubes; i++)
    {
        for (u8 j = 0; j < 5; j++)
        {
            mPadData[i][j].mType = 0;
            mPadData[i][j].mId = j;
        }
        mPadConn[i]->mData = mPadData[i];
    }

    for (i = 0; i < 3; i++)
    {
        mPadDataLocal[i].mType = 0;
    }
    for (i = 0; i < 2; i++)
    {
        mSendPkt[i].mState = 0;
    }
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
    mSeed = OSGetTick();
    constructFifo();

    mGCInfoNum = 0;
    mLinkNum = 0;
    for (int i = 0; i < 16; i++)
    {
        mGCInfo[i].mAddr = 0;
        mGCInfo[i].mState = 0;
        mGCInfo[i].mWait = 0;
        mGCInfo[i].mCount = 0;
        mGCInfo[i].mTTL = 0;
        mGCInfo[i].mSecretKart = 0;
        mGCInfo[i].mGameFlag = 0;
        mGCInfo[i].mDelay = 0;
    }

    initPadConv();

    for (u8 n = 0; n < 2; n++)
    {
        mSendBuf[n].mAddr.mFamily = 2;
        mSendBuf[n].mAddr.mLen = 8;
        mSendBuf[n].mAddr.mPort = 0x6790;
    }

    mRecvPkt.mAddr.mLen = 8;
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
void NetGameMgr::makeLoop()
{
    u8 i;
    NetPadConn *conn;
    for (i = 1; i < mNetworkCubes; i++)
    {
        NetPadConn **link = &mPadConnHead->mNext;
        for (conn = mPadConnHead->mNext; conn != nullptr && conn->mDelay < mPadConnArray[i].mDelay; conn = conn->mNext)
        {
            link = &conn->mNext;
        }
        mPadConnArray[i].mNext = conn;
        *link = &mPadConnArray[i];
    }

    conn = mPadConnHead;
    for (i = 0; i < mNetworkCubes; i++)
    {
        mPadConn[i] = conn;
        conn->mIndex = i;
        conn->mData = mPadData[i];
        conn = conn->mNext;
    }
    mPadConn[i - 1]->mNext = mPadConnHead;
}
void NetGameMgr::makeLinkData(NetLinkPkt *pkt)
{
    pkt->mType = 1;
    pkt->mVideoFlag = KartLocale::getVideoFrameMode() == VideoFrameMode_60Hz;
    pkt->mSecretKart = gSystemRecord.mSecretKart;
    pkt->mGameFlag = gSystemRecord.mGameFlag;
    u8 n = 0;
    for (int i = 0; i < 16; i++)
    {
        switch (mGCInfo[i].mState)
        {
        case 3:
        case 4:
            pkt->mAddr[n++] = mGCInfo[i].mAddr;
            break;
        }
        if (n >= 15)
        {
            break;
        }
    }
    pkt->mNum = n;
}
void NetGameMgr::makeSyncData(NetSyncPkt *pkt)
{
    pkt->mType = 2;
    pkt->mNum = mNetworkCubes;
    pkt->mSeed = mSeed;
    pkt->mSecretKart = mSecretKart;
    pkt->mGameFlag = mGameFlag;
    NetPadConn *conn = mPadConnHead;
    for (u8 i = 0; i < mNetworkCubes; i++)
    {
        pkt->mAddr[i] = conn->mAddr;
        conn = conn->mNext;
    }
}
int NetGameMgr::calcPadPktSize(NetPadPkt &pkt)
{
    return sizeof(NetPadPkt) - (8 - pkt.mHeader.mNum) * sizeof(NetPadUnit);
}
void NetGameMgr::makePadData(NetPadPkt *pkt)
{
    pkt->mHeader.mType = 3;
    pkt->mHeader.mNum = 0;
    pkt->mHeader.mTTL = (mNetworkCubes - 1) * 2;
    pkt->mHeader.mFrame = _970;
    pkt->mHeader.mFlags = 0;
    pkt->mTime = mFrame;
}
void NetGameMgr::makeSleepData(NetSleepPkt *pkt)
{
    pkt->mHeader.mType = 4;
    pkt->mHeader.mCount = mNetworkCubes;
}
void NetGameMgr::makeAwakeData(NetAwakePkt *pkt)
{
    pkt->mHeader.mType = 5;
    pkt->mHeader.mCount = mNetworkCubes;
}
void NetGameMgr::makeHaltData(NetHaltPkt *pkt)
{
    pkt->mHeader.mType = 6;
}
void NetGameMgr::doStart()
{
    if (BBAMgr::mspBBAMgr->getState() == 2)
    {
        mState = 2;
    }
}
void NetGameMgr::doDHCP()
{
    switch (BBAMgr::mspBBAMgr->processDHCP())
    {
    case 1:
        break;
    case 3:
        mPadConnArray[0].mAddr = BBAMgr::mspBBAMgr->mHostAddr;
        mPadConnHead = &mPadConnArray[0];
        mState = 6;
        break;
    case 2:
        mState = 4;
        break;
    case 4:
        mState = 3;
        break;
    }
}
void NetGameMgr::doAutoIP()
{
    switch (BBAMgr::mspBBAMgr->processAutoIP())
    {
    case 0:
    case 1:
        break;
    case 3:
        mPadConnArray[0].mAddr = BBAMgr::mspBBAMgr->mHostAddr;
        mPadConnHead = &mPadConnArray[0];
        mState = 6;
        break;
    case 2:
        mState = 5;
        break;
    }
}
void NetGameMgr::startSearch()
{
    BBAMgr::mspBBAMgr->startMSearch();
    mState = 7;
}
void NetGameMgr::doLink()
{
    GCInfo *target;
    u8 readyNum = 0;
    mLinkNum = 0;
    target = nullptr;
    for (int i = 0; i < 16; i++)
    {
        GCInfo *info = &mGCInfo[i];
        switch (info->mState)
        {
        case 1:
            info->mState = 2;
            info->mWait = 0;
            target = info;
            break;
        case 3:
        case 4:
            if (--info->mTTL < 0)
            {
                info->mState = 2;
                info->mWait = 0;
                break;
            }
            if (info->mState == 4)
            {
                readyNum++;
            }
            mLinkNum++;
        case 2:
            if (target == nullptr || target->mWait > info->mWait)
            {
                target = info;
            }
            if (info->mWait != 0)
            {
                info->mWait--;
            }
            break;
        }
    }

    if (target != nullptr)
    {
        target->mWait = 16;
        makeLinkData(&mSendBuf[0].mLink);
        mSendBuf[0].mLength = 36;
        mSendBuf[0].mAddr.mAddr.mAddr = target->mAddr;
        System::mspSendTask->request(sendTask, (void *)SEND_PRIORITY_0, nullptr);
    }

    if (readyNum == mLinkNum)
    {
        mState = 9;
    }
    else
    {
        mState = 8;
    }
}
void NetGameMgr::startLink()
{
    if (mState == 6)
    {
        BBAMgr::mspBBAMgr->startMSearch();
        mState = 7;
    }
}
bool NetGameMgr::finishLink()
{
    if (mState == 9)
    {
        mState = 10;
        int n = 0;
        mPadConnArray[n++].mDelay = 0;
        mGameFlag = gSystemRecord.mGameFlag;
        mSecretKart = gSystemRecord.mSecretKart;
        for (int i = 0; i < 16; i++)
        {
            GCInfo *info = &mGCInfo[i];
            switch (info->mState)
            {
            case 1:
            case 3:
                return false;
            case 4:
                mPadConnArray[n].mAddr = info->mAddr;
                mPadConnArray[n].mDelay = info->mDelay / info->mCount;
                mGameFlag |= info->mGameFlag;
                mSecretKart |= info->mSecretKart;
                n++;
                break;
            }
            if (n >= 8)
            {
                break;
            }
        }

        mNetworkCubes = mLinkNum;
        mNetworkCubes++;

        makeLoop();
        mState = 11;
        return true;
    }
    return false;
}
void NetGameMgr::doSync()
{
    if (&mPadConnArray[0] == mPadConnHead)
    {
        makeSyncData(&mSendBuf[0].mSync);
        mSendBuf[0].mLength = 40;
        sendNext(SEND_PRIORITY_0);
    }
}
void NetGameMgr::doPad(SendPriority prio)
{
    BOOL enabled = OSDisableInterrupts();
    NetPadPkt pkt;
    if (&mPadConnArray[0] == mPadConnHead)
    {
        makePadData(&pkt);
        storePadData(pkt);
        mPadState = 2;
    }

    if (mPadState == 2)
    {
        if (appendPadData(_970, &pkt))
        {
            u8 mask = (1 << mNetworkCubes) - 1;
            if (&mPadConnArray[0] == mPadConnHead && mask == (mask & pkt.mHeader.mFlags))
            {
                mState = 14;
            }
            else if (pkt.mHeader.mTTL != 0)
            {
                pkt.mHeader.mTTL--;
                mSendBuf[prio].mPad = pkt;
                mSendBuf[prio].mLength = calcPadPktSize(pkt);
                sendNext(prio);
            }

            if (pkt.mHeader.mNum == mNetworkCubes)
            {
                distributePadData(pkt);
            }
            mPadState = 0;
        }
    }
    OSRestoreInterrupts(enabled);
}
void NetGameMgr::doAwake()
{
    if (&mPadConnArray[0] == mPadConnHead)
    {
        makeAwakeData(&mSendBuf[0].mAwake);
        mSendBuf[0].mLength = 4;
        if (--mSendBuf[0].mAwake.mHeader.mCount != 0)
        {
            sendNext(SEND_PRIORITY_0);
        }
        else
        {
            mState = 13;
            _970 = 0;
            _972 = 0;
            mFrame = 0.0f;
        }
    }

    if (++_97c > 600)
    {
        end(false);
        ErrorViewApp::call(ErrorViewApp::ERROR_LA_TRANSFER2);
    }
}
void NetGameMgr::doWaitSleep()
{
    if (&mPadConnArray[0] == mPadConnHead)
    {
        makeSleepData(&mSendBuf[0].mSleep);
        mSendBuf[0].mLength = 4;
        if (--mSendBuf[0].mSleep.mHeader.mCount != 0)
        {
            sendNext(SEND_PRIORITY_0);
        }
        else
        {
            mState = 15;
            mSleepReq = 0;
        }
    }
}
void NetGameMgr::doWaitEnd()
{
    if (BBAMgr::mspBBAMgr->disconnecting(_974))
    {
        mState = 0;
        mNetworkCubes = 1;
        mPadConnArray[0].mIndex = 0;
    }
}
void NetGameMgr::doWaitPad()
{
    if (++_980 > 300)
    {
        end(false);
        ErrorViewApp::call(ErrorViewApp::ERROR_LA_TRANSFER2);
    }
}
void NetGameMgr::doException()
{
    switch (mHaltKind)
    {
    case 0:
#line 794
        JUT_ASSERT_F(false, "cno(%d),pos", mPadConnArray[0].mIndex);
        break;
    default:
#line 797
        JUT_PANIC(0);
        break;
    }
}
void NetGameMgr::doHalt()
{
#line 808
    JUT_ASSERT_F(false, "halt(%d)", mPadConnArray[0].mIndex);
}
void NetGameMgr::haltNetConsole(ExceptionKind kind)
{
    mHaltKind = kind;
    makeHaltData(&mSendBuf[0].mHalt);
    mSendBuf[0].mLength = 4;
    broadcast(SEND_PRIORITY_0);
    mState = 18;
}
void NetGameMgr::recvLink()
{
    if (mState != 8 && mState != 9)
    {
        return;
    }

    NetLinkPkt &pkt = mRecvPkt.mLink;
    if (pkt.mVideoFlag != (KartLocale::getVideoFrameMode() == VideoFrameMode_60Hz))
    {
        return;
    }

    OSTime now = OSGetTime();
    u32 addr = mRecvPkt.mAddr.mAddr.mAddr;
    u8 count = 0;
    GCInfo *found = nullptr;
    for (int i = 0; i < 16; i++)
    {
        GCInfo *info = &mGCInfo[i];
        if (info->mState != 0)
        {
            if (info->mAddr == addr)
            {
                if (info->mCount < 255)
                {
                    info->mDelay += (u32)((now - mSyncTime) & 0xffffffff);
                    info->mCount++;
                }
                info->mSecretKart = pkt.mSecretKart;
                info->mGameFlag = pkt.mGameFlag;
                info->mTTL = 20;
                found = info;
                count++;
            }
            else
            {
                for (int j = 0; j < pkt.mNum; j++)
                {
                    if (info->mAddr == pkt.mAddr[j])
                    {
                        count++;
                        break;
                    }
                }
            }
        }
    }

    for (int j = 0; j < pkt.mNum; j++)
    {
        if (mPadConnArray[0].mAddr == pkt.mAddr[j])
        {
            count++;
            break;
        }
    }

    if (found != nullptr)
    {
        if (count == pkt.mNum + 1 && count == mLinkNum + 1)
        {
            found->mState = 4;
        }
        else
        {
            found->mState = 3;
        }
    }
}
void NetGameMgr::recvSync()
{
    switch (mState)
    {
    case 9:
    case 11:
    case 12:
        break;
    default:
        return;
    }

    NetSyncPkt &pkt = mRecvPkt.mSync;
    if (pkt.mAddr[0] == mPadConnArray[0].mAddr)
    {
        mState = 12;
        _970 = 0;
        _972 = 0;
        mFrame = 0.0f;
    }
    else if (mState != 11 || mPadConnArray[0].mAddr > pkt.mAddr[0])
    {
        u8 i;
        u8 map[8];
        u8 n = 1;
        for (i = 0; i < pkt.mNum; i++)
        {
            if (pkt.mAddr[i] == mPadConnArray[0].mAddr)
            {
                map[i] = 0;
            }
            else
            {
                map[i] = n;
                mPadConnArray[n].mAddr = pkt.mAddr[i];
                mPadConnArray[n].mDelay = 0;
                n++;
            }
        }
        mGameFlag = pkt.mGameFlag;
        mSecretKart = pkt.mSecretKart;
        mPadConnHead = &mPadConnArray[map[0]];
        NetPadConn **link = &mPadConnHead->mNext;
        for (i = 1; i < pkt.mNum; i++)
        {
            *link = &mPadConnArray[map[i]];
            link = &(*link)->mNext;
        }
        *link = mPadConnHead;
        mNetworkCubes = pkt.mNum;
        mSendBuf[0].mBody = mRecvPkt.mBody;
        mSendBuf[0].mLength = mRecvPkt.mLength;
        sendNext(SEND_PRIORITY_0);

        NetPadConn *conn = mPadConnHead;
        for (i = 0; i < mNetworkCubes; i++)
        {
            mPadConn[i] = conn;
            conn->mIndex = i;
            conn->mData = mPadData[i];
            conn = conn->mNext;
        }
        mSeed = pkt.mSeed;
        mState = 12;
        _970 = 0;
        _972 = 0;
        mFrame = 0.0f;
    }
    PadMgr::setKartPadInput(PadMgr::Input_Net);
}
void NetGameMgr::recvPad()
{
    switch (mState)
    {
    case 17:
        mState = 12;
    case 12:
    case 13:
    {
        BOOL enabled = OSDisableInterrupts();
        NetPadPkt &pkt = mRecvPkt.mPad;
        if (pkt.mHeader.mNum < mNetworkCubes)
        {
            storePadData(pkt);
            if (appendPadData(pkt.mHeader.mFrame, &pkt))
            {
                mPadState = 1;
            }
            else
            {
                mPadState = 2;
            }
        }

        if (mPadState == 1 || pkt.mHeader.mNum == mNetworkCubes)
        {
            u8 mask = (1 << mNetworkCubes) - 1;
            if (&mPadConnArray[0] == mPadConnHead && mask == (mask & pkt.mHeader.mFlags))
            {
                mState = 14;
            }
            else if (pkt.mHeader.mTTL != 0)
            {
                mSendBuf[0].mPad = pkt;
                mSendBuf[0].mPad.mHeader.mTTL--;
                mSendBuf[0].mLength = calcPadPktSize(mSendBuf[0].mPad);
                sendNext(SEND_PRIORITY_0);
            }
            mPadState = 0;
        }

        if (pkt.mHeader.mNum == mNetworkCubes)
        {
            distributePadData(pkt);
        }
        OSRestoreInterrupts(enabled);
        break;
    }
    }
}
void NetGameMgr::distributePadData(NetPadPkt &pkt)
{
    NetPadConn *conn = mPadConnHead;
    for (u8 i = 0; i < mNetworkCubes; i++)
    {
        NetPadData *data = conn->mData;
        for (u8 j = 0; j < 5; j++)
        {
            if (data->mId == pkt.mHeader.mFrame)
            {
                data->mType = 1;
                data->mPad = pkt.mPad[i].mCur;
                if (data->mPrev->mId == (u8)(pkt.mHeader.mFrame - 1) && data->mPrev->mType != 1)
                {
                    data->mPrev->mType = 1;
                    data->mPrev->mPad = pkt.mPad[i].mPrev;
                }
                break;
            }
            data = data->mNext;
        }
        conn = conn->mNext;
    }

    if (_972 != 0)
    {
        NetPadData *data = mPadDataNext;
        for (u8 i = 0; i < 3; i++)
        {
            if (data->mType == 1 && data->mId == pkt.mHeader.mFrame)
            {
                if (memcmp(&data->mPad, &pkt.mPad[mPadConnArray[0].mIndex], sizeof(NetPadFrame)) != 0)
                {
                    end(false);
                    ErrorViewApp::call(ErrorViewApp::ERROR_LA_TRANSFER);
                }
                break;
            }
            data = data->mPrev;
        }
    }
}
bool NetGameMgr::appendPadData(u8 frame, NetPadPkt *pkt)
{
    NetPadQueue *queue = mSendPktNext;
    bool found = false;
    bool hasPrev = false;
    bool hasNext = false;
    bool stored = false;
    NetPadData *data = mPadDataNext;
    NetPadQueue *prevQueue;
    NetPadQueue *nextQueue;

    for (u8 i = 0; i < 3; i++)
    {
        if (data->mType == 1 && data->mId == frame)
        {
            found = true;
            break;
        }
        data = data->mPrev;
    }

    if (found)
    {
        for (u8 i = 0; i < 3; i++)
        {
            if (queue->mState == 1)
            {
                u8 id = queue->mPkt.mHeader.mFrame;
                if (id == frame)
                {
                    stored = true;
                    break;
                }
                if (id == (u8)(frame - 1))
                {
                    prevQueue = queue;
                    hasPrev = true;
                }
                if (id == (u8)(frame + 1))
                {
                    nextQueue = queue;
                    hasNext = true;
                }
            }
            queue = queue->mPrev;
        }

        if (stored)
        {
            *pkt = queue->mPkt;
            if (data->mFrame != pkt->mTime && 0.0f != pkt->mTime)
            {
                end(false);
                ErrorViewApp::call(ErrorViewApp::ERROR_LA_TRANSFER);
            }
        }
        else if (hasPrev && hasNext)
        {
            pkt->mHeader.mType = prevQueue->mPkt.mHeader.mType;
            pkt->mHeader.mNum = prevQueue->mPkt.mHeader.mNum;
            pkt->mHeader.mTTL = prevQueue->mPkt.mHeader.mTTL;
            pkt->mHeader.mFlags = 0;
            pkt->mHeader.mFrame = frame;
            for (u8 i = 0; i < prevQueue->mPkt.mHeader.mNum && i < 8; i++)
            {
                pkt->mPad[i].mPrev = prevQueue->mPkt.mPad[i].mCur;
                pkt->mPad[i].mCur = nextQueue->mPkt.mPad[i].mPrev;
            }
            pkt->mTime = 0.0f;
            stored = true;
        }

        if (stored && pkt->mHeader.mNum < 8)
        {
            pkt->mPad[pkt->mHeader.mNum].mCur = data->mPad;
            pkt->mPad[pkt->mHeader.mNum].mPrev = data->mPrev->mPad;
            if (mSleepReq != 0)
            {
                pkt->mHeader.mFlags |= 1 << pkt->mHeader.mNum;
            }
            else
            {
                pkt->mHeader.mFlags &= ~(1 << pkt->mHeader.mNum);
            }
            pkt->mHeader.mNum++;
            return true;
        }
    }

    return false;
}
void NetGameMgr::storePadData(NetPadPkt &pkt)
{
    NetPadQueue *queue = mSendPktNext;
    if (queue->mState != 1 || queue->mPkt.mHeader.mFrame != pkt.mHeader.mFrame)
    {
        queue = queue->mNext;
        queue->mState = 1;
        queue->mPkt = pkt;
        mSendPktNext = queue;
    }
}
void NetGameMgr::recvSleep()
{
    switch (mState)
    {
    case 13:
    case 14:
    case 15:
        mSendBuf[0].mSleep = mRecvPkt.mSleep;
        mSendBuf[0].mLength = 4;
        if (mRecvPkt.mSleep.mHeader.mCount != 0)
        {
            mSendBuf[0].mSleep.mHeader.mCount--;
            sendNext(SEND_PRIORITY_0);
        }
        mState = 15;
        mSleepReq = 0;
        break;
    }
}
void NetGameMgr::recvAwake()
{
    switch (mState)
    {
    case 12:
    case 13:
    case 16:
    case 17:
        if (mRecvPkt.mAwake.mHeader.mCount != 0)
        {
            mSendBuf[0].mAwake = mRecvPkt.mAwake;
            mSendBuf[0].mAwake.mHeader.mCount--;
            mSendBuf[0].mLength = 4;
            sendNext(SEND_PRIORITY_0);
            mState = 17;
            _980 = 0;
        }
        else
        {
            mState = 12;
        }
        _970 = 0;
        _972 = 0;
        mFrame = 0.0f;
        break;
    }
}
void NetGameMgr::recvHalt()
{
    if (mState != 18)
    {
        mState = 19;
    }
}
void NetGameMgr::recv()
{
    logNet(NET_ACTION_RECV, mRecvPkt);
    switch ((s8)mRecvPkt.mHeader.mType)
    {
    case 1:
        recvLink();
        break;
    case 2:
        recvSync();
        break;
    case 3:
        recvPad();
        break;
    case 4:
        recvSleep();
        break;
    case 5:
        recvAwake();
        break;
    case 6:
        recvHalt();
        break;
    }
}
void NetGameMgr::recvTask(void *)
{
    int ret = BBAMgr::mspBBAMgr->recvFrom(&mspNetGameMgr->mRecvPkt, sizeof(NetPktBody), &mspNetGameMgr->mRecvPkt.mAddr);
    if (ret < 0)
    {
        mspNetGameMgr->end(false);
        switch (ret)
        {
        case -56:
            if (mspNetGameMgr->mState != 0x14)
            {
                ErrorViewApp::call(ErrorViewApp::ERROR_LA_CONNECTION);
            }
            break;
        default:
            ErrorViewApp::call(ErrorViewApp::ERROR_LA_ROUTER);
            break;
        }
    }
    else
    {
        mspNetGameMgr->mRecvPkt.mLength = ret;
        mspNetGameMgr->recv();
        System::mspRecvTask->request(recvTask, nullptr, nullptr);
    }
}
void NetGameMgr::sendTask(void *arg)
{
    int prio = (int)arg;
    int ret = BBAMgr::mspBBAMgr->sendTo(&mspNetGameMgr->mSendBuf[prio], mspNetGameMgr->mSendBuf[prio].mLength, &mspNetGameMgr->mSendBuf[prio].mAddr);
    if (ret < 0)
    {
        mspNetGameMgr->end(false);
        switch (ret)
        {
        case -38:
            ErrorViewApp::call(ErrorViewApp::ERROR_LA_CONNECTION);
            break;
        default:
            ErrorViewApp::call(ErrorViewApp::ERROR_LA_ROUTER);
            break;
        }
    }
    else
    {
        mspNetGameMgr->logNet((NetActionType)prio, mspNetGameMgr->mSendBuf[prio]);
    }
}
void NetGameMgr::startRecvTask()
{
    if (mState == 7)
    {
        System::mspRecvTask->request(recvTask, nullptr, nullptr);
        mState = 8;
    }
}
void NetGameMgr::framework()
{
    if (mState == 0)
    {
        return;
    }

    switch (mState)
    {
    case 1:
        doStart();
        break;
    case 2:
        doDHCP();
        break;
    case 4:
        doAutoIP();
        break;
    case 8:
    case 9:
        doLink();
        break;
    case 11:
        doSync();
        break;
    case 12:
        mState = 13;
    case 13:
        doPad(SEND_PRIORITY_0);
        break;
    case 17:
        doWaitPad();
        break;
    case 16:
        doAwake();
        break;
    case 14:
        doWaitSleep();
        break;
    case 20:
        doWaitEnd();
        break;
    case 18:
        doException();
        break;
    case 19:
        doHalt();
        break;
    }

    handleError();
}
void NetGameMgr::syncwork()
{
    switch (mState)
    {
    case 0:
    case 15:
    case 19:
    case 20:
        break;
    default:

    mSyncTime = OSGetTime();
    mLogHead->mTick = OSGetTick();
    mLogHead->mAction = 3;
    mLogHead->mType = 0;
    mLogHead->mArg0 = 0;
    mLogHead->mArg1 = 0;
    mLogHead->mArg2 = 0;
    mLogHead = mLogHead->mNext;

    if (mWaitPad != 0)
    {
        doPad(SEND_PRIORITY_1);
    }
    break;
    }
}
void NetGameMgr::retraceCallback(u32)
{
    if (mspNetGameMgr != nullptr)
    {
        mspNetGameMgr->syncwork();
    }
}
bool NetGameMgr::checkUSN(const char *url, UPnPUuid *uuid)
{
    char usn[80];
    if (HTTPGetHeader(url, "USN", usn, sizeof(usn)) <= 0)
    {
        return false;
    }
    if (strnicmp(usn, "uuid:", 5) != 0)
    {
        return false;
    }
    if (UPnPScanUuid(usn + 5, uuid) < 0)
    {
        return false;
    }
    if (stricmp(usn + 41, "::upnp:rootdevice") != 0)
    {
        return false;
    }
    return true;
}
bool NetGameMgr::checkSERVER(const char *url)
{
    char server[64];
    if (HTTPGetHeader(url, "SERVER", server, sizeof(server)) <= 0)
    {
        return false;
    }
    if (strncmp(SSDPGetServerName(), server, 39) != 0)
    {
        return false;
    }
    return true;
}
void NetGameMgr::callbackNotify(const char *url, const SOInAddr *addr)
{
    if (url == nullptr && addr == nullptr)
    {
        return;
    }

    BBAMgr *bba = BBAMgr::mspBBAMgr;
    if ((addr->mAddr & bba->mNetmask) != (bba->mHostAddr & bba->mNetmask))
    {
        return;
    }

    UPnPUuid uuid;
    if (!checkUSN(url, &uuid))
    {
        return;
    }

    char nts[16];
    if (HTTPGetHeader(url, "NTS", nts, sizeof(nts)) <= 0)
    {
        return;
    }

    if (stricmp(nts, "ssdp:alive") == 0)
    {
        if (checkSERVER(url) && SSDPGetCacheControlMaxAge(url) > 0)
        {
            mspNetGameMgr->appendGCInfo(addr);
        }
    }
    else if (stricmp(nts, "ssdp:byebye") == 0)
    {
        mspNetGameMgr->removeGCInfo(addr);
    }
}
void NetGameMgr::callbackSearch(const char *url, const SOInAddr *addr, const SSDPSearchInfo *)
{
    if (url == nullptr && addr == nullptr)
    {
        mspNetGameMgr->startRecvTask();
        return;
    }

    BBAMgr *bba = BBAMgr::mspBBAMgr;
    if ((addr->mAddr & bba->mNetmask) != (bba->mHostAddr & bba->mNetmask))
    {
        return;
    }

    if (HTTPGetStatus(url) != 200)
    {
        return;
    }

    UPnPUuid uuid;
    if (!checkUSN(url, &uuid))
    {
        return;
    }
    if (!checkSERVER(url))
    {
        return;
    }
    if (SSDPGetCacheControlMaxAge(url) <= 0)
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
        empty->mState = 1;
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
            info->mState = 0;
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
            u8 *src = (u8 *)&data->mPad.mPad[port];
            u8 *dst = (u8 *)padData;
            dst[0] = src[0];
            dst[1] = src[1];
            return;
        }
    }

    memset(padData, 0, sizeof(KartPadData));
}
void NetGameMgr::adjustFrame()
{
    if (mState != 0xd)
    {
        return;
    }

    if (_971 >= 1)
    {
        int frame = _970 - 1;
        mWaitCount = 0;
        do
        {
            BOOL enabled = OSDisableInterrupts();
            mWaitPad = 0;
            if (mState == 0xd)
            {
                for (int i = 0; i < mNetworkCubes; i++)
                {
                    NetPadData *data = mPadConn[i]->mData;
                    if (data->mId != (u8)frame || data->mType != 1)
                    {
                        mWaitPad = 1;
                        break;
                    }
                }

                mFrame = 0.0f;
                if (AppMgr::mspGameApp == RaceApp::ptr() && RaceApp::ptr()->getRaceMgr()->getRacePhase() == PHASE_RUNNING)
                {
                    for (int i = 0; i < gRaceInfo.getKartNumber(); i++)
                    {
                        JGeometry::TVec3f pos;
                        KartCtrl::getKartCtrl()->GetBodyPos(i, &pos);
                        mFrame += pos.x + pos.y + pos.z;
                    }
                }
            }
            OSRestoreInterrupts(enabled);

            if (mWaitPad != 0)
            {
                mWaitCount++;
                if (mWaitCount == 30)
                {
                    System::haltRumble();
                }
                if (mWaitCount > 300)
                {
                    end(false);
                    ErrorViewApp::call(ErrorViewApp::ERROR_LA_TRANSFER2);
                    mWaitPad = 0;
                }
                VIWaitForRetrace();
            }
        } while (mWaitPad != 0);
    }
    _970++;
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
            PadMgr::getPadData(i, PadMgr::Input_Local, &data->mPad.mPad[i]);
        }
    }
    else
    {
        memset(&data->mPad, 0, 8);
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

    mState = 0x10;
    resetFifo();
}
void NetGameMgr::handleError()
{
    if (mState < 1)
    {
        return;
    }
    if (mState > 17)
    {
        return;
    }

    switch (IPGetConfigError(0))
    {
    case -100:
        if (mState == 4)
        {
            break;
        }
    case -102:
    case -101:
        mspNetGameMgr->end(false);
        ErrorViewApp::call(ErrorViewApp::ERROR_LA_ROUTER);
        break;
    case -111:
        mspNetGameMgr->end(false);
        ErrorViewApp::call(ErrorViewApp::ERROR_LA_CONNECTED);
        break;
    case -112:
    case -110:
    case -109:
    case -108:
    case -107:
    case -106:
    case -105:
    case -104:
    case -103:
        mspNetGameMgr->end(false);
        ErrorViewApp::call(ErrorViewApp::ERROR_LA_CONNECTION);
        break;
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
    u8 type = pkt.mHeader.mType;
    mLogHead->mType = type;
    mLogHead->mTick = OSGetTick();
    mLogHead->mAction = action;

    switch ((s8)type)
    {
    case 2:
        mLogHead->mArg0 = pkt.mHeader.mArg0;
        mLogHead->mArg1 = pkt.mHeader.mArg1;
        mLogHead->mArg2 = pkt.mHeader.mArg2;
        break;
    case 3:
        mLogHead->mArg0 = (pkt.mHeader.mArg0 >> 4) & 0xf;
        mLogHead->mArg1 = pkt.mHeader.mArg0 & 0xf;
        mLogHead->mArg2 = pkt.mHeader.mArg2;
        break;
    case 5:
        mLogHead->mArg0 = pkt.mHeader.mArg0;
        mLogHead->mArg1 = pkt.mHeader.mArg1;
        mLogHead->mArg2 = pkt.mHeader.mArg2;
        break;
    }

    mLogHead->mUnk8 = pkt.mAddr.mAddr.mAddr;
    mLogHead = mLogHead->mNext;
}
