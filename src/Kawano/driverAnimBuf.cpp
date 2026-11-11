#include "Kawano/driverAnimBuf.h"

#include "JSystem/J3D/J3DAnmLoader.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kawano/driverMgr.h"
#include "Osako/ResMgr.h"

DriverBuf::DriverBuf() {
    mBuf = nullptr;
}

DriverBuf::~DriverBuf() {}

bool DriverBuf::readResAsync(char *folder, char *fileName) {
    return ResMgr::readResourceAsync(mBuf, mSize, folder, fileName, (ResMgr::AramAsyncState *)&mLoadState);
}

void DriverBuf::setID(int id) {
    mID = id;
    mUsed = 1;
}

int DriverBuf::CheckBufID(int id) {
    if (mUsed != 0 && mID == id)
        return 1;
    return 0;
}

DriverAnimBuf::DriverAnimBuf() {
    mSize = 0x5fe0;
    mAnm = nullptr;
    reset();
}

void DriverAnimBuf::reset() {
    if (mBuf == nullptr)
        mBuf = new (0x20) u8[0x5fe0];
    mID = -1;
    mLoadState = 0;
    mUsed = 0;
    if (mAnm == nullptr)
        mAnm = new J3DAnmTransformFull();
}

DriverAnimBuf::~DriverAnimBuf() {}

void DriverAnimBuf::changeResource() {
    J3DAnmLoaderDataBase::setResource(mAnm, mBuf);
}

DriverSoundBuf::DriverSoundBuf() {
    mSize = 0x1e0;
    reset();
}

void DriverSoundBuf::reset() {
    if (mBuf == nullptr)
        mBuf = new (0x20) u8[0x1e0];
    mID = -1;
    mLoadState = 0;
    mUsed = 0;
}

DriverSoundBuf::~DriverSoundBuf() {}

DriverAnimBufMgr::DriverAnimBufMgr() {
    reset();
}

void DriverAnimBufMgr::reset() {
    mRenew = 1;
    mRenewSound = 1;
    for (int i = 0; i < 4; i++)
        mAnimBufs[i].reset();
    mSoundBuf.reset();
}

int DriverAnimBufMgr::CheckAndLoadAnimAvailable(int anmID, int dataID) {
    for (int i = 0; i < 4; i++) {
        if (mAnimBufs[i].CheckBufID(anmID))
            return i;
    }
    return loadAnimation(anmID, dataID);
}

void DriverAnimBufMgr::CheckAndLoadAnimSoundAvailable(int sndID, int dataID) {
    LoadAnimationSound(sndID, dataID);
}

void DriverAnimBufMgr::LoadAnimation(int idx, int anmID, int dataID) {
    if (idx < 4 && anmID != 0) {
        char *folder;
        if (DriverManager::ptr()->getAnmFolderFlag(dataID, anmID))
            folder = (char *)DriverLoader::getDriverFolderName2(dataID);
        else
            folder = (char *)DriverLoader::getDriverName(dataID);

        DriverAnimBuf *buf = &mAnimBufs[idx];
        if (!buf->readResAsync(folder, (char *)DriverLoader::getAnmFileName(anmID))) {
#line 327
            JUT_ASSERT_F(false, "%s", "ARAM tensou command error?(kawano)");
        } else {
            buf->setID(anmID);
        }
    }
    mRenew = 1;
}

int DriverAnimBufMgr::loadAnimation(int anmID, int dataID) {
    for (int i = 0; i < 4; i++) {
        if (mAnimBufs[i].mUsed == 0) {
            LoadAnimation(i, anmID, dataID);
            return i;
        }
    }
    return -1;
}

void DriverAnimBufMgr::LoadAnimationSound(int sndID, int dataID) {
    if (sndID != 0 && mSoundBuf.mUsed == 0) {
        char *folder = (char *)DriverManager::ptr()->getFolderName_Sound(dataID, sndID);
        mSoundBuf.readResAsync(folder, (char *)DriverLoader::getAnmSoundFileName(sndID));
        mSoundBuf.setID(sndID);
        mRenewSound = 1;
    }
}

int DriverAnimBufMgr::CheckRenew() {
    int ret = 0;
    if (mRenew != 0) {
        for (int i = 0; i < 4; i++) {
            DriverAnimBuf *buf = &mAnimBufs[i];
            if (buf->mUsed != 0) {
                if (buf->mLoadState != 2)
                    return -1;
                buf->changeResource();
            }
        }
        mRenew = 0;
        ret = 1;
    }
    return ret;
}

int DriverAnimBufMgr::CheckRenewSound() {
    if (mRenewSound != 0) {
        mRenewSound = 0;
        if (mSoundBuf.mUsed != 0) {
            if (mSoundBuf.mLoadState != 2)
                return -1;
            return 1;
        }
    }
    return 0;
}

bool DriverAnimBufMgr::IsARAMDMAFinished() {
    for (int i = 0; i < 4; i++) {
        if (mAnimBufs[i].mUsed != 0 && mAnimBufs[i].mLoadState != 2)
            return false;
    }
    if (mSoundBuf.mUsed != 0 && mSoundBuf.mLoadState != 2)
        return false;
    return true;
}

void DriverAnimBufMgr::Check() {}

void DriverAnimBufMgr::resetFrame() {
    for (int i = 0; i < 4; i++)
        mAnimBufs[i].mUsed = 0;
}

void DriverAnimBufMgr::resetFrameSound() {
    mSoundBuf.mUsed = 0;
}

DriverAnimBuf *DriverAnimBufMgr::searchBuffer(int id) {
    DriverAnimBuf *buf = nullptr;
    for (int i = 0; i < 4; i++) {
        DriverAnimBuf *p = &mAnimBufs[i];
        if (p->CheckBufID(id)) {
            buf = p;
            break;
        }
    }
    return buf;
}

void *DriverAnimBufMgr::GetPtr(int id) {
    void *ptr = nullptr;
    if (mRenew == 0) {
        DriverAnimBuf *buf = searchBuffer(id);
        if (buf != nullptr) {
            if (buf->mLoadState != 2)
                ptr = nullptr;
            else
                ptr = buf->mAnm;
        }
    }
    return ptr;
}

void *DriverAnimBufMgr::GetSoundPtr(int id) {
    if (mSoundBuf.CheckBufID(id)) {
        if (mSoundBuf.mLoadState != 2)
            return nullptr;
        return mSoundBuf.mBuf;
    }
    return nullptr;
}
