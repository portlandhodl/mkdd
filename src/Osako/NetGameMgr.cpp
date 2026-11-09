#include "Osako/NetGameMgr.h"
#include "Osako/BBAMgr.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "dolphin/os.h"
#include <string.h>

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

void NetGameMgr::initPadConv()
{
    for (int i = 0; i < 8; i++)
    {
        mPadConvData[i] = i * 4;
        mPadConvData[i + 8] = i * 4 + 1;
    }
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

void NetGameMgr::retraceCallback(u32)
{
    if (mspNetGameMgr != nullptr)
    {
        mspNetGameMgr->syncwork();
    }
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
    u8 i = 0;
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
