#include "Osako/BBAMgr.h"
#include "Osako/NetGameMgr.h"
#include "Osako/ResMgr.h"
#include "Osako/clock.h"
#include "Kaneshige/SysDebug.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JAudio/System/JASTrack.h"
#include "JSystem/JAudio/System/JASChannel.h"
#include "JSystem/JAudio/Interface/JAISe.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
#include "dolphin/dvd.h"
#include "dolphin/os.h"
#include "mathHelper.h"

extern "C" {
void SOInit();
s32 SOStartup(SOConfig *);
s32 SOCleanup();
s32 SOSocket(s32, s32, s32);
s32 SOBind(s32, void *);
s32 SOSendTo(s32, void *, s32, s32, void *);
s32 SORecvFrom(s32, void *, s32, s32, void *);
void SSDPInit(SSDPServiceConfig *);
void SSDPStart(SSDPInfo *, s32);
s32 SSDPStop();
s32 SSDPGetState();
void SSDPSearch(SSDPSearchInfo *);
void UPnPHttpInit(HTTPServiceConfig *);
void UPnPHttpStart();
void UPnPGetHostAddr(u32 *);
void UPnPGetNetmask(u32 *);
void UPnPCreateUuidFromName(void *, const void *, const void *, s32);
char *UPnPGetUuidString(void *, char *);
s32 IPGetLinkState(s32, s32 *);
s32 IPGetConfigError(s32);
void IPClearConfigError(s32);
void IPGetMacAddr(s32, void *);
s32 EXIGetType(s32, u32, u32 *);
}

BBAMgr *BBAMgr::mspBBAMgr;

static void *alloc(u32, s32);
static void free(u32, void *, s32);

static OSMutex sAllocLock;
static OSThread sSSDPThread;
static OSThread sHTTPThread;
static u8 saSSDPStack[0x4000];
static u8 saHTTPStack[0x4000];
static char sHttpThreadName[12];
static char sHttpThreadName2[12];
static char sHttpThreadName3[12];
static char sHttpThreadName4[12];
extern u8 UPnPUdnNameSpace[16];

static SOConfig sConfig = {
    0x100, alloc, free, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0,
};

static SSDPServiceConfig sSSDPServiceConfig = {
    &sSSDPThread, saHTTPStack, 0x4000, 8, (void (*)(const char *, const void *))NetGameMgr::callbackNotify, 0x01000000,
};

static struct {
    const char *mPath;
    void *mData;
    long mSize;
} sHttpFile = {"/rootDesc.xml", 0, 0};

static HTTPServiceConfig sHTTPServiceConfig = {
    &sHTTPThread, sHttpThreadName, 0x4000, 0x14, &sHttpFile, 1,
};

static SSDPSearchInfo sSearchInfo = {
    "upnp:rootdevice", (void (*)(const char *, const void *, const void *))NetGameMgr::callbackSearch,
};

static void *alloc(u32 size, s32 align)
{
    if (align > 0)
    {
        OSLockMutex(&sAllocLock);
        void *ptr = BBAMgr::mspBBAMgr->mHeap->alloc(align, 0);
        OSUnlockMutex(&sAllocLock);
        return ptr;
    }
    return nullptr;
}

static void free(u32 size, void *ptr, s32 align)
{
    if (ptr != nullptr && align > 0)
    {
        OSLockMutex(&sAllocLock);
        BBAMgr::mspBBAMgr->mHeap->free(ptr);
        OSUnlockMutex(&sAllocLock);
    }
}

BBAMgr *BBAMgr::create(JKRHeap *heap)
{
    if (mspBBAMgr == nullptr)
    {
        BBAMgr *mgr = new (heap, 0) BBAMgr(heap);
        mspBBAMgr = mgr;
    }
    return mspBBAMgr;
}

BBAMgr::BBAMgr(JKRHeap *heap)
{
    mStartTime = 0;
    mUnk24 = 0;
    mFlag = 0;
    mState = 0;
    mUnk10 = 0;
    mTimer0 = 0.0f;
    mTimer1 = 0.0f;

    u32 type;
    if (EXIGetType(0, 2, &type) != 0)
    {
        if (type == 0x04020200)
        {
            SOInit();
            mHeap = JKRExpHeap::create(0x19000, heap, true);
            SysDebug::getManager()->createHeapInfo(mHeap, "BBA");
            mUnk00 = 1;
            mState = 1;
            mHostAddr = 0;
            SSDPInit(&sSSDPServiceConfig);
            UPnPHttpInit(&sHTTPServiceConfig);
            loadHttpFile();
            createBasicDevice();
        }
        else
        {
            mUnk00 = 3;
        }
    }
    else
    {
        mUnk00 = 2;
    }
}

int BBAMgr::processDHCP()
{
    int ret = 1;

    switch (mState)
    {
    case 2:
        sConfig.mType = 1;
        if (getState() == 2)
        {
            if (SOStartup(&sConfig) >= 0)
            {
                mSocket = SOSocket(2, 2, 0);
                memset(mAddr, 0, 8);
                mAddr[1] = 2;
                mAddr[0] = 8;
                *(u32 *)(mAddr + 4) = 0;
                *(u16 *)(mAddr + 2) = 0x6790;
                SOBind(mSocket, mAddr);
            }
        }
        mState = 3;
        ret = 1;
        IPClearConfigError(0);
        mFlag = 0;
        mUnk24 = 0;
        if (mFlag != 1)
        {
            mStartTime = Clock::getTime() - mUnk24;
            mFlag = 1;
        }
        mTimer0 = 0.0f;
        break;
    case 3:
        switch (IPGetConfigError(0))
        {
        case -100:
            SOCleanup();
            mState = 2;
            mUnk10 |= 1;
            ret = 2;
            break;
        case 0:
            UPnPGetHostAddr(&mHostAddr);
            UPnPGetNetmask(&mNetmask);
            if (mHostAddr != 0)
            {
                startUPnP();
                mState = 5;
                ret = 3;
            }
            else
            {
                ret = 1;
            }
            break;
        default:
            ret = 4;
            break;
        }
        f32 timer;
        switch (mFlag)
        {
        case 1:
            timer = Clock::calcFrameTime(mStartTime);
            break;
        case 2:
            timer = mTimeout;
            break;
        default:
            timer = 0.0f;
            break;
        }
        mTimer0 = timer;
        break;
    }

    return ret;
}

int BBAMgr::processAutoIP()
{
    switch (mState)
    {
    case 2:
        sConfig.mType = 0x8000;
        if (getState() == 2)
        {
            if (SOStartup(&sConfig) >= 0)
            {
                mSocket = SOSocket(2, 2, 0);
                memset(mAddr, 0, 8);
                mAddr[1] = 2;
                mAddr[0] = 8;
                *(u32 *)(mAddr + 4) = 0;
                *(u16 *)(mAddr + 2) = 0x6790;
                SOBind(mSocket, mAddr);
            }
        }
        mState = 4;
        mFlag = 0;
        mUnk24 = 0;
        int ret = 1;
        if (mFlag != 1)
        {
            mStartTime = Clock::getTime() - mUnk24;
            mFlag = 1;
        }
        mTimer1 = 0.0f;
        return ret;
    case 4:
    {
        int ret;
        UPnPGetHostAddr(&mHostAddr);
        if (mHostAddr != 0)
        {
            UPnPGetNetmask(&mNetmask);
            startUPnP();
            mState = 5;
            ret = 3;
        }
        else
        {
            f32 t;
            switch (mFlag)
            {
            case 1:
                t = Clock::calcFrameTime(mStartTime);
                break;
            case 2:
                t = mTimeout;
                break;
            default:
                t = 0.0f;
                break;
            }

            if (t > 80.0f)
            {
                SOCleanup();
                mState = 2;
                mUnk10 |= 2;
                ret = 2;
            }
            else
            {
                ret = 1;
            }
        }

        f32 timer;
        switch (mFlag)
        {
        case 1:
            timer = Clock::calcFrameTime(mStartTime);
            break;
        case 2:
            timer = mTimeout;
            break;
        default:
            timer = 0.0f;
            break;
        }
        mTimer1 = timer;
        return ret;
    }
    }
}

bool BBAMgr::disconnecting(bool unk)
{
    bool ret = false;

    switch (mState)
    {
    case 0:
    case 1:
    case 2:
        IPClearConfigError(0);
        ret = true;
        break;
    case 3:
    case 4:
        IPClearConfigError(0);
        SOCleanup();
        mState = 1;
        ret = true;
        break;
    case 5:
        IPClearConfigError(0);
        if (unk)
        {
            SOCleanup();
            mState = 1;
            ret = true;
            break;
        }
        SSDPStop();
        mState = 6;
        // fallthrough
    case 6:
        if (unk)
        {
            SOCleanup();
            mState = 1;
            ret = true;
            break;
        }
        if (SSDPGetState() == 1)
        {
            SOCleanup();
            mState = 1;
            ret = true;
        }
        break;
    }

    return ret;
}

void BBAMgr::startUPnP()
{
    SSDPStart(mInfo, 3);
    UPnPHttpStart();
}

void BBAMgr::startMSearch()
{
    SSDPSearch(&sSearchInfo);
}

int BBAMgr::sendTo(void *buf, int len, const void *addr)
{
    return SOSendTo(mSocket, buf, len, 0, (void *)addr);
}

int BBAMgr::recvFrom(void *buf, int len, void *addr)
{
    return SORecvFrom(mSocket, buf, len, 0, addr);
}

int BBAMgr::getState()
{
    if (mState >= 1)
    {
        s32 link;
        IPGetLinkState(0, &link);
        if (link != 0)
        {
            if (mState == 1)
            {
                mState = 2;
                mUnk00 = 0;
            }
        }
        else
        {
            mState = 1;
        }
    }
    return mState;
}

void BBAMgr::createBasicDevice()
{
    u8 uuid[16];
    char udn[] = "<UDN>uuid:";
    char name[12];
    char uuidString[40];

    DVDDiskID *diskId = DVDGetCurrentDiskID();
    IPGetMacAddr(0, name);
    memmove(name + 6, &diskId->gameName, 4);
    memmove(name + 10, diskId->company, 2);

    UPnPCreateUuidFromName(uuid, UPnPUdnNameSpace, name, 12);

    sprintf(mRootDevice, "upnp:rootdevice");
    sprintf(mDeviceUuid, "uuid:%s", UPnPGetUuidString(uuid, uuidString));
    sprintf(mDeviceType, "urn:schemas-upnp-org:device:Basic:1");
    sprintf(mRootUuid, "uuid:%s::%s", UPnPGetUuidString(uuid, uuidString), mRootDevice);
    sprintf(mUuid, "uuid:%s", UPnPGetUuidString(uuid, uuidString));
    sprintf(mTypeUuid, "uuid:%s::%s", UPnPGetUuidString(uuid, uuidString), mDeviceType);

    char *pUuid = strstr((char *)sHttpFile.mData, udn);
#line 503
    JUT_ASSERT_F(pUuid, "%s", "pUUid");

    memcpy(pUuid + 10, UPnPGetUuidString(uuid, uuidString), 36);

    mInfo[0].mName = mRootDevice;
    mInfo[1].mName = mDeviceUuid;
    mInfo[2].mName = mDeviceType;
    mInfo[0].mUuidName = mRootUuid;
    mInfo[1].mUuidName = mUuid;
    mInfo[2].mUuidName = mTypeUuid;
}

void BBAMgr::loadHttpFile()
{
    char buf[64];
    snprintf(buf, 64, "WWW%s", sHttpFile.mPath);
    sHttpFile.mData = ResMgr::getPtr(ResMgr::mcArcSystem, buf);
    sHttpFile.mSize = ResMgr::getResSize(ResMgr::mcArcSystem, sHttpFile.mData);
}
