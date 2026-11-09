#include "Kawano/feeling.h"
#include "JSystem/JAudio/JASFakeMatch2.h"

FeelingOne::FeelingOne(Feeling *feel, int idx)
{
    mFeeling = feel;
    mIndex = idx;

    setModelData(0, FeelingMgr::ptr()->getJ3DModelDataPtr(idx));

    char path[64];
    snprintf(path, sizeof(path), "/Driver/Feeling/%s", Feeling::mscpaFeelingAnmFileName[idx]);
    void *res = ResMgr::getPtr(ResMgr::mcArcMRAM, path);
    if (res != nullptr) {
        mAnm = (J3DAnmTransform *)J3DAnmLoaderDataBase::load(res, J3DLOADER_UNK_FLAG0);
        mAnm->mAttribute = J3DAA_UNKNOWN_2;
    }

    mMat0.mFrameCtrl.reset();
    mMat0.mFrameCtrl.setRate(0.0f);

    mMat1.mFrameCtrl.reset();
    mMat1.mFrameCtrl.setRate(0.0f);
    mMat1.mFrameCtrl.setAttribute(2);
}



const char *Feeling::mscpaFeelingAnmFileName[2] = { "feel_ef_a.bca", "feel_ef_b.bca" };

const J3DTevStageInfo FeelingMgr::mscTevStage[3] = {
    { 0x0E, 0x08, 0x0F, 0x04, 0x00, 0x00, 0x00, 0x02, 0x04, 0x07,
      0x00, 0x00, 0x00, 0x01, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00 },
    { 0x08, 0x04, 0x06, 0x04, 0x01, 0x02, 0x00, 0x0F, 0x07, 0x07,
      0x00, 0x01, 0x00, 0x00, 0x04, 0x07, 0x00, 0x00, 0x01, 0x00 },
    { 0x08, 0x04, 0x06, 0x02, 0x01, 0x02, 0x00, 0x0F, 0x04, 0x07,
      0x00, 0x01, 0x00, 0x00, 0x07, 0x01, 0x00, 0x00, 0x01, 0x00 },
};

const char *FeelingMgr::mscpaFeelingBmdFileName[2] = { "feel_ef_a.bmd", "feel_ef_b.bmd" };
const char *FeelingMgr::mscpaFeelingBrkFileName[2] = { "feel_ef_a.brk", "feel_ef_b.brk" };
const char *FeelingMgr::mscpaFeelingBtpFileName[1] = { "feel_ef_b.btp" };

FeelingMgr *FeelingMgr::mspFeelingMgr;



void *FeelingOne::getAnmFile(int)
{
}

void FeelingOne::reset()
{
}

void FeelingOne::updateF()
{
    mAnm->setFrame(mFeeling->mFrameCtrl.mFrame);

    int index = mIndex;
    J3DMtxCalc **calcs = FeelingMgr::ptr()->mMtxCalc;
    J3DMtxCalc *calc = *(calcs + index);
    calc->setAnmTransform(mAnm);

    mMat0.setFrame((f32)mFeeling->m_28);
    mMat0.update();
    mMat0.anmFrameProc();

    if (mMat1.mAnmBase != nullptr) {
        mMat1.setFrame(mFeeling->getBtpFrame());
        mMat1.update();
        mMat1.anmFrameProc();
    }

    ExModel::update(0);
}


void FeelingOne::createModelF(int, JKRSolidHeap *, u32, u32)
{
}

void FeelingOne::setCurrentViewNo(u32 viewNo)
{
    ExModel::setCurrentViewNo(viewNo);
}

Feeling::Feeling(DriverModel *driver)
{
    mDriver = driver;

    for (int i = 0; i < 2; i++) {
        FeelingOne *one = new FeelingOne(this, i);
        mOne[i] = one;
    }

    mState = 0;
    m_28 = 2;
    m_2c = 1;

    mFrameCtrl.init(mOne[1]->mAnm->getFrameMax());
    mFrameCtrl.mAttr = 0;
}

void Feeling::reset()
{
}

void Feeling::updateOffset(int)
{
}

void Feeling::updateState()
{
}

void Feeling::update(int danger)
{
    if (danger) {
        mState = 2;
        m_2c = 0;
        m_28 = FeelingMgr::ptr()->m_80;
        mFrameCtrl.mFrame = mFrameCtrl.mStartFrame;
        mFrameCtrl.mRate = 1.0f;
        mFrameCtrl.mState = 0;
        mFrameCtrl.mEndFrame = mOne[0]->mAnm->getFrameMax();
    } else {
        switch (mState) {
        case 0:
            if (!mDriver->IsDanger())
                mState = 1;
            break;
        case 1:
            if (mDriver->IsDanger()) {
                mState = 2;
                m_28 = 1;
                mFrameCtrl.mFrame = mFrameCtrl.mStartFrame;
                mFrameCtrl.mRate = 1.0f;
                mFrameCtrl.mState = 0;
                mFrameCtrl.mEndFrame = mOne[1]->mAnm->getFrameMax();
            }
            break;
        default:
            break;
        }
    }

    switch (mState) {
    case 2:
        m_2c = 0;
        break;
    case 3:
        m_2c = 1;
        break;
    }

    f32 f31 = 0.0f;
    f32 f30;

    for (int i = 0; i < 2; i++) {
        if (m_2c == i) {
            if (mState < 2) {
                if (!(mState < 0))
                    mOne[i]->hide();
            } else {
                if (mState < 4)
                    mOne[i]->show();
            }
        } else {
            mOne[i]->hide();
        }

        FeelingMgr *mgr = FeelingMgr::ptr();
        if (mgr->m_84 != 1) {
            if (!((u32)mgr->m_84 < 2)) {
                if (mgr->m_84 < 3)
                    mOne[i]->hide();
            }
        } else {
            mOne[i]->show();
        }

        Mtx mtx;
        if (mDriver->getMtxP(18) != nullptr)
            PSMTXCopy((MtxPtr)mDriver->getMtxP(18), mtx);
        else
            PSMTXCopy((MtxPtr)mDriver->getMtxP(1), mtx);

        switch (i) {
        case 0:
            f30 = DriverData::msDriverData[mDriver->mDataID].getData(82);
            break;
        case 1:
            f30 = DriverData::msDriverData[mDriver->mDataID].getData(83);
            break;
        }

        Vec offset;
        offset.x = f31;
        offset.y = f30;
        offset.z = f31;
        Vec pos;
        PSMTXMultVec((MtxPtr)mtx, &offset, &pos);

        mtx[0][3] = pos.x;
        mtx[1][3] = pos.y;
        mtx[2][3] = pos.z;
        PSMTXCopy(mtx, mOne[i]->getModel()->mPosMtx);

        mOne[i]->updateF();

        if (mFrameCtrl.mRate == 0.0f) {
            switch (mState) {
            case 3:
                mState = 0;
                break;
            case 2:
                mState = 0;
                break;
            }
        }
    }

    mFrameCtrl.update();
}

void Feeling::createModel(JKRSolidHeap *, u32, u32)
{
}

void Feeling::setCurrentViewNo(u32)
{
}

f32 Feeling::getBtpFrame()
{
    switch (m_28) {
    case 0:
        return 1.0f;
    case 1:
    case 2:
        return 0.0f;
    default:
        return 0.0f;
    }
}








FeelingMgr::FeelingMgr()
{
    int i;
    for (i = 0; i < 16; i++)
        mFeelings[i] = nullptr;

    mCount = 0;
    m_78 = 5;
    m_7c = 0;
    m_80 = 0;
    m_82 = 0;
    m_84 = 2;
    mTevColor.r = 0;
    mTevColor.g = 0;
    mTevColor.b = 0;
    mTevColor.a = 200;

    for (i = 0; i < 2; i++) {
        mModelData[i] = createModelData(i);

        J3DMtxCalc *calc;
        J3DAnmTevRegKey *brk = getBrkFile(i);
        if (brk != nullptr) {
            J3DMaterial *mat = mModelData[i]->getMaterialNodePointer(0);
            mat->getTevBlock()->getTevStage(0)->setTevStageInfo(mscTevStage[2]);
            mat->getTevBlock()->setTevColor(0, &mTevColor);
            J3DAnmObjMaterial::setupTevRegAnmData(&mTevReg[i], getJ3DModelDataPtr(i), brk);
        }

        if (i == 1) {
            J3DAnmTexPattern *btp = getBtpFile();
            J3DAnmObjMaterial::setupTexPatternAnmData(&mTexPattern, getJ3DModelDataPtr(i), btp);
        }

        mMtxCalc[i] = new J3DMtxCalcAnimation<J3DMtxCalcAnimationAdaptorDefault<J3DMtxCalcCalcTransformMaya>, J3DMtxCalcJ3DSysInitMaya>(nullptr);
        calc = mMtxCalc[i];
        getJ3DModelDataPtr(i)->getJointNodePointer(0)->mMtxCalc = calc;
    }
}

FeelingMgr::~FeelingMgr()
{
    mspFeelingMgr = nullptr;
}

FeelingMgr *FeelingMgr::ptr()
{
    if (mspFeelingMgr == nullptr)
        mspFeelingMgr = new FeelingMgr();
    return mspFeelingMgr;
}

J3DAnmTevRegKey *FeelingMgr::getBrkFile(int idx)
{
    char path[64];
    snprintf(path, sizeof(path), "/Driver/Feeling/%s", mscpaFeelingBrkFileName[idx]);
    return (J3DAnmTevRegKey *)ResMgr::getPtr(ResMgr::mcArcMRAM, path);
}

J3DAnmTexPattern *FeelingMgr::getBtpFile()
{
    char path[64];
    snprintf(path, sizeof(path), "/Driver/Feeling/%s", mscpaFeelingBtpFileName[0]);
    return (J3DAnmTexPattern *)ResMgr::getPtr(ResMgr::mcArcMRAM, path);
}

void FeelingMgr::init(DriverModel *driver)
{
    for (int i = 0; i < mCount; i++) {
        if (mFeelings[i]->mDriver == driver)
            return;
    }

    mFeelings[mCount] = new Feeling(driver);
    mCount++;
}

J3DModelData *FeelingMgr::createModelData(int idx)
{
    char path[64];
    snprintf(path, sizeof(path), "/Driver/Feeling/%s", mscpaFeelingBmdFileName[idx]);
    J3DModelData *md = J3DModelLoaderDataBase::load(ResMgr::getPtr(ResMgr::mcArcMRAM, path), 0x10020010);
    ExModel::patchModelData(md);
    md->newSharedDisplayList(0x40000);
    return md;
}

void FeelingMgr::update()
{
    int i;
    int flag;

    for (i = 0; i < 2; i++)
        mModelData[i]->getMaterialNodePointer(0)->getTevBlock()->setTevColor(0, &mTevColor);

    for (i = 0; i < mCount; i++) {
        if (i == m_82 && m_7c != 0) {
            m_7c = 0;
            flag = 1;
        } else {
            flag = 0;
        }
        mFeelings[i]->update(flag);
    }
}

void FeelingMgr::createModel(DriverModel *driver, JKRSolidHeap *heap, u32 a, u32 b)
{
    int j;
    Feeling *feel;
    FeelingOne *one;
    int found = 0;
    for (int i = 0; i < mCount; i++) {
        if (mFeelings[i]->mDriver == driver) {
            feel = mFeelings[i];
            j = 0;
            for (; j < 2; j++) {
                one = feel->mOne[j];
                one->ExModel::createDifferedModel(heap, a, 0x05020000, false);

                one->mMat0.setExModel(one);
                one->mMat0.mAnmBase = FeelingMgr::ptr()->mTevReg[j];
                one->mMat0.initFrameCtrl(one->mMat0.mAnmBase);

                if (j == 1) {
                    one->mMat1.setExModel(one);
                    one->mMat1.mAnmBase = FeelingMgr::ptr()->mTexPattern;
                    one->mMat1.initFrameCtrl(one->mMat1.mAnmBase);
                }
            }

            if (found == 0) {
                mFeelings[0]->mOne[0]->makeSharedDL();
                mFeelings[0]->mOne[0]->lock();
                mFeelings[0]->mOne[1]->makeSharedDL();
                mFeelings[0]->mOne[1]->lock();
            }
            break;
        }
        found++;
    }
}

void FeelingMgr::setCurrentViewNo(u32 viewNo)
{
    int j = 0;
    Feeling *feel;
    int i;
    for (i = 0; i < mCount; i++) {
        feel = mFeelings[i];
        for (j = 0; j < 2; j++)
            feel->mOne[j]->setCurrentViewNo(viewNo);
    }
}

J3DModelData *FeelingMgr::getJ3DModelDataPtr(int idx)
{
#line 632
    JUT_MINMAX_ASSERT(0, idx, 2)
#line 306
    return mModelData[idx];
}
