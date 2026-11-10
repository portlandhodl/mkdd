#include "Kawano/driver.h"
#include "Kawano/driverTPAnim.h"

#include "JSystem/J3D/J3DAnmTransform.h"
#include "JSystem/J3D/J3DAnmLoader.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DMtxCalc.h"
#include "JSystem/JUtility/JUTAssert.h"

#include "JSystem/JAudio/JASFakeMatch.h"
#include "JSystem/JAudio/JASFakeMatch2.h"

#include "Kaneshige/KartLoader.h"
#include "Kaneshige/KartMotion.h"
#include "Kawano/driverMgr.h"
#include "Sato/NeckCtrl.h"
#include "Kawano/feeling.h"
#include "Kawano/osage.h"
#include "Kawano/accessory.h"
#include "Osako/animator.h"
#include "Kaneshige/RaceMgr.h"
#include "Yamamoto/kartCtrl.h"

// ---------------------------------------------------------------------------
// Local minimal declarations for classes owned by sibling units
// ---------------------------------------------------------------------------


class LumbarCtrl // 0xF0
{
public:
    LumbarCtrl(int, u8);
    void reset();
    u8 _0[0x70];
    u8 m_70;
    u8 _71[0xf0 - 0x71];
};

class ArmCtrl // 0x40
{
public:
    ArmCtrl(int, u8);
    void reset();
    u8 _0[0x40];
};

class FootCtrl // 0x44
{
public:
    FootCtrl(int, u8);
    void reset();
    u8 _0[0x44];
};

class HairCtrlBase
{
public:
    virtual void unk0();
    virtual void reset();
};


class DriverAnimBufMgr // size 0x7C
{
public:
    DriverAnimBufMgr();
    void reset();
    void *GetPtr(int);
    void *GetSoundPtr(int);
    bool CheckAndLoadAnimAvailable(int, int);
    bool CheckAndLoadAnimSoundAvailable(int, int);
    int CheckRenew();
    int CheckRenewSound();
    void Check();
    void resetFrame();
    void resetFrameSound();
    int mRes0; // 0x0
    int mRes4; // 0x4
    u8 _8[0x7c - 0x8];
};

class LaurelMgr
{
public:
    static LaurelMgr *ptr();
    void init(DriverModel *);
    void createModel(DriverModel *, JKRSolidHeap *, u32, u32);
    void setCurrentViewNo(DriverModel *, u32);
    void update(DriverModel *);
};

// Callbacks table is owned by driver_ik unit
extern const J3DJointCallBack mscaCallBackTbl__11DriverModel[27];

// ---------------------------------------------------------------------------

u8 DriverModel::msNext_mState_HokanDamages[2] = {110, 41};

static const f32 sZeroQuatset[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

const u32 DriverModel::msAnmBlendList[152][2] = {
    { 1, 2 }, { 16, 17 }, { 18, 0 }, { 52, 55 }, { 58, 0 }, { 6, 0 },
    { 7, 0 }, { 14, 0 }, { 15, 0 }, { 19, 19 }, { 19, 19 }, { 23, 23 },
    { 23, 23 }, { 19, 0 }, { 19, 0 }, { 21, 0 }, { 21, 0 }, { 23, 0 },
    { 23, 0 }, { 25, 0 }, { 25, 0 }, { 27, 0 }, { 28, 0 }, { 29, 0 },
    { 30, 0 }, { 31, 0 }, { 32, 0 }, { 33, 0 }, { 34, 0 }, { 35, 0 },
    { 36, 37 }, { 38, 0 }, { 39, 0 }, { 40, 0 }, { 41, 0 }, { 42, 0 },
    { 43, 0 }, { 44, 44 }, { 44, 44 }, { 44, 44 }, { 45, 45 }, { 45, 45 },
    { 45, 41 }, { 46, 0 }, { 46, 0 }, { 46, 0 }, { 47, 0 }, { 47, 0 },
    { 47, 41 }, { 53, 56 }, { 53, 56 }, { 53, 56 }, { 51, 54 }, { 51, 54 },
    { 51, 54 }, { 59, 0 }, { 59, 0 }, { 59, 0 }, { 57, 0 }, { 57, 0 },
    { 57, 0 }, { 60, 0 }, { 61, 0 }, { 21, 0 }, { 21, 0 }, { 72, 0 },
    { 73, 0 }, { 71, 0 }, { 72, 0 }, { 48, 49 }, { 52, 55 }, { 36, 37 },
    { 50, 0 }, { 58, 0 }, { 38, 0 }, { 48, 49 }, { 50, 0 }, { 16, 17 },
    { 18, 0 }, { 52, 55 }, { 58, 0 }, { 52, 55 }, { 52, 55 }, { 58, 0 },
    { 58, 0 }, { 27, 28 }, { 29, 30 }, { 31, 0 }, { 32, 0 }, { 33, 34 },
    { 35, 0 }, { 36, 37 }, { 38, 0 }, { 40, 0 }, { 42, 0 }, { 43, 0 },
    { 6, 0 }, { 7, 0 }, { 8, 0 }, { 9, 0 }, { 10, 0 }, { 11, 0 },
    { 12, 0 }, { 13, 0 }, { 14, 14 }, { 15, 0 }, { 44, 44 }, { 45, 45 },
    { 60, 0 }, { 61, 0 }, { 62, 63 }, { 52, 55 }, { 58, 0 }, { 53, 56 },
    { 51, 54 }, { 59, 0 }, { 57, 0 }, { 44, 44 }, { 64, 0 }, { 65, 0 },
    { 1, 2 }, { 66, 0 }, { 67, 0 }, { 68, 0 }, { 69, 0 }, { 41, 0 },
    { 53, 56 }, { 51, 54 }, { 70, 0 }, { 70, 0 }, { 71, 0 }, { 71, 0 },
    { 72, 0 }, { 74, 0 }, { 75, 0 }, { 76, 0 }, { 53, 56 }, { 51, 54 },
    { 59, 0 }, { 57, 0 }, { 27, 0 }, { 28, 0 }, { 29, 0 }, { 30, 0 },
    { 31, 0 }, { 32, 0 }, { 77, 0 }, { 78, 0 }, { 79, 0 }, { 39, 0 },
    { 80, 0 }, { 81, 0 }
};

// ---------------------------------------------------------------------------

#line 80
DriverModel::DriverModel(KartLoader *kart, int driverNo, u16 level, bool b, int arg)
    : ExModel(level)
{
    ECharID charID = kart->mKartInfo->getDriverCharID(driverNo); // KartInfo.h:142
    mCharID = charID;
    mDataID = convCharIDToDataID((ECharID)mCharID);
    m_98 = b;
    m_9c = arg;
    mKartLoader = kart;
    mKartNum = mKartLoader->mKartNum;
    mDriverNo = driverNo;
    mDriverLoader = nullptr;
    mAnimBufMgr = nullptr;
    for (int i = 0; i < 11; i++) {
        mKeepAnmTrans[i] = nullptr;
    }
    mKeepSndFile[0] = nullptr;
    mTPAnim = nullptr;
    for (int i = 0; i < 0x98; i++) {
        mMtxCalcs[i] = nullptr;
    }
    for (int i = 0; i < 2; i++) {
        mMtxCalcsThrow[i] = nullptr;
    }
    mNeckCtrl = nullptr;
    mLumbarCtrl = nullptr;
    mArmCtrl = nullptr;
    mFootCtrlL = nullptr;
    mFootCtrlR = nullptr;
    mHairCtrl = nullptr;
    mCharSoundMgr = nullptr;
    init(driverNo);
    DriverManager::ptr()->SetDriverModelPtr(this);
}

DriversAnimator::~DriversAnimator()
{
}

int DriverModel::convCharIDToDataID(ECharID charID)
{
    int ret = 20;
    switch (charID) {
    case BABY_MARIO:  ret = 0;  break;
    case BABY_LUIGI:  ret = 1;  break;
    case PARATROOPA:  ret = 2;  break;
    case KOOPATROOPA: ret = 3;  break;
    case PEACH:       ret = 4;  break;
    case DAISY:       ret = 5;  break;
    case MARIO:       ret = 6;  break;
    case LUIGI:       ret = 7;  break;
    case WARIO:       ret = 8;  break;
    case WALUIGI:     ret = 9;  break;
    case YOSHI:       ret = 10; break;
    case BIRDO:       ret = 11; break;
    case DONKEY_KONG: ret = 12; break;
    case DIDDY_KONG:  ret = 13; break;
    case BOWSER:      ret = 14; break;
    case BOWSER_JR:   ret = 15; break;
    case TOAD:        ret = 16; break;
    case TOADETTE:    ret = 17; break;
    case KING_BOO:    ret = 18; break;
    case PETEY:       ret = 19; break;
    }
    return ret;
}

void DriverModel::init_ik(int i)
{
    int kartNum = mKartLoader->mKartNum;
    if (mNeckCtrl == nullptr)
        mNeckCtrl = new NeckCtrl(kartNum, i, m_98);
    else
        mNeckCtrl->reset();

    if (mLumbarCtrl == nullptr)
        mLumbarCtrl = new LumbarCtrl(kartNum, i);
    else
        mLumbarCtrl->reset();

    if (mArmCtrl == nullptr)
        mArmCtrl = new ArmCtrl(kartNum, i);
    else
        mArmCtrl->reset();

    if (mFootCtrlL == nullptr)
        mFootCtrlL = new FootCtrl(kartNum, i);
    else
        mFootCtrlL->reset();

    if (mFootCtrlR == nullptr)
        mFootCtrlR = new FootCtrl(kartNum, i);
    else
        mFootCtrlR->reset();

    switch (mCharID) {
    case PEACH:
        if (mHairCtrl == nullptr)
            mHairCtrl = new PeachHairCtrl(kartNum, i);
        else
            ((HairCtrlBase *)mHairCtrl)->reset();
        break;
    case DAISY:
        if (mHairCtrl == nullptr)
            mHairCtrl = new DaisyHairCtrl(kartNum, i);
        else
            ((HairCtrlBase *)mHairCtrl)->reset();
        break;
    case DONKEY_KONG:
        if (mHairCtrl == nullptr)
            mHairCtrl = new DonkeyTieCtrl(kartNum, i);
        else
            ((HairCtrlBase *)mHairCtrl)->reset();
        break;
    }
}

void DriverModel::init(int driverNo)
{
    StateEnum stt;
    if (driverNo == 0)
        stt = (StateEnum)0;
    else
        stt = (StateEnum)3;
    mState = stt;

    for (int i = 0; i < 2; i++)
        (&m_ac)[i] = 0;
    m_b0 = 0.0f;
    mItem = nullptr;
    m_b8 = 0;
    m_b9 = 0;
    m_ba = 0;
    m_bb = 0;
    mHokanDamageCnt = 0;
    m_be = 0;
    m_c0 = 0;
    m_c2 = 0;
    m_c4 = 0;
    m_c8 = 0;
    m_cc = 0;
    m_d0 = 0;
    m_d4 = 0;
    m_d8 = 0;
    m_dc = 0;
    m_e0 = 0;
    m_e4 = 0;
    m_e8 = 0;

    mDriveAnim.mLoopAnim.reset();
    mTandemAnim.reset();
    mAdditiveAnim.reset();
    m_668.reset();
    m_688.reset();

    if (mDriverLoader == nullptr) {
        mDriverLoader = new DriverLoader(mDataID);

        setModelData(0, mDriverLoader->getMdlData(0));
        setModelData(1, mDriverLoader->getMdlData(1));

        J3DJoint *jnt;
        for (u16 i = 0; i < 27; i++) {
            u16 jntNo = mDriverLoader->getJntBindNo(i);
            jnt = nullptr;
            if (jntNo != 0xffff)
                jnt = getModelData()->getJointNodePointer(jntNo);
            if (jnt)
                jnt->mFunction = mscaCallBackTbl__11DriverModel[i];
        }
    }

    if (mAnimBufMgr == nullptr)
        mAnimBufMgr = new DriverAnimBufMgr();
    else
        mAnimBufMgr->reset();

    for (int i = 0; i < 82; i++) {
        mAnmFrame[i] = 0.0f;
        int keepID = DriverLoader::convAnmIDToKeepID((DriverLoader::AnmEnum)i);
        if (keepID >= 0) {
#line 609
            JUT_MINMAX_ASSERT(0, keepID, 11);
            void *anmFile = mDriverLoader->getAnmFile(keepID);
            if (anmFile) {
                if (mKeepAnmTrans[keepID] == nullptr) {
                    mKeepAnmTrans[keepID] =
                        (J3DAnmTransform *)J3DAnmLoaderDataBase::load(anmFile, (J3DAnmLoaderDataBaseFlag)0);
                    if (mDataID == 0x13)
                        initAnmTrans_BossPakkun(mKeepAnmTrans[keepID]);
                } else {
                    J3DAnmLoaderDataBase::setResource(mKeepAnmTrans[keepID], anmFile);
                }
                mAnmFrame[i] = (f32)mKeepAnmTrans[keepID]->getFrameMax() * 0.5f;
            }
        }
    }

    mAnmID = msAnmBlendList[mState][0];
    if (mState == mcDrive)
        mAnmFrame[mAnmID] = 20.0f;

    for (int sndNo = 0; sndNo < 12; sndNo++) {
        int keepID = DriverLoader::convSndIDToKeepID((DriverLoader::AnmSoundEnum)sndNo);
        if (keepID >= 0) {
            if (mKeepSndFile[keepID] == nullptr)
                mKeepSndFile[keepID] = mDriverLoader->getSoundDataFile(keepID);
        }
}

    if (mTPAnim == nullptr)
        mTPAnim = new DriverTPAnim(this, mDataID);

    initMtxCalc();
    if (stt == 3)
        StartDWait();

    switch (m_9c) {
    case 1:
        if (DriverManager::ptr()->getGoodB2Flag(mDataID) != 0)
            StartDemo2();
        else
            StartDemo();
        break;
    case 2:
    case 3:
        if (mDriverNo == 0)
            StartDemo2Front();
        else if (DriverManager::ptr()->getGoodB2Flag(mDataID) != 0)
            StartDemo2();
        else
            StartDemo2Back();
        break;
    }

    init_ik(driverNo);

    mBeforeState = mState;
    m_4f8 = mState;
    mBeforeState = mState;
    m_500 = 3;
    m_504 = 1;
    next_mState_Hokan = mState;
    next_before_mState_Hokan = mBeforeState;
    m_510 = -1;
    m_514 = 0;

    updateLoad();
    InitDriverOffsetPosition();

    mPos.z = 0.0f;
    mPos.y = 0.0f;
    mPos.x = 0.0f;

    if (mCharSoundMgr == nullptr)
        mCharSoundMgr = new GameAudio::CharacterSoundMgr((Vec *)&mPos, nullptr, mKartLoader->mKartNum, (ECharID)mCharID);

    FeelingMgr::ptr()->init(this);
    OsageMgr::ptr();
    AccessoryMgr::ptr();
    LaurelMgr::ptr();

    switch (mCharID) {
    case TOADETTE:
        OsageMgr::ptr()->init(this);
        break;
    case KING_BOO:
        if (KartInfo::KartCharacter::convPlayerKind(*(KartGamePad **)((char *)mKartLoader->mKartInfo + 4)) != 4)
            AccessoryMgr::ptr()->init(this);
        break;
    }

    if (m_9c != 0)
        LaurelMgr::ptr()->init(this);

    for (int i = 0; i < 4; i++) {
        m_6c4[i].z = 0.0f; m_6c4[i].y = 0.0f; m_6c4[i].x = 0.0f;
        m_6f4[i].z = 0.0f; m_6f4[i].y = 0.0f; m_6f4[i].x = 0.0f;
        m_724[i].z = 0.0f; m_724[i].y = 0.0f; m_724[i].x = 0.0f;
    }

    for (int i = 0; i < 4; i++)
        m_754[i] = 0.0f;
}

enum { J3DMdlDataFlag_MtxCalcTypeMaya = 2 };

void DriverModel::initMtxCalc()
{
    for (int i = 0; i < 0x98; i++) {
        if (mMtxCalcs[i] == nullptr) {
#line 836
            JUT_ASSERT(getModelData()->getMtxCalcType() == J3DMdlDataFlag_MtxCalcTypeMaya);
            mMtxCalcs[i] = new KartMtxCalcBlendAnm();
        } else {
            ((KartMtxCalcBlendAnm *)mMtxCalcs[i])->reset();
        }
    }

    if (mMtxCalcsThrow[0] == nullptr)
        mMtxCalcsThrow[0] = new KartMtxCalcAdditiveAnm();
    else
        ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->reset();

    ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->_24 |= 1;
    ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->_24 |= 2;
    ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->_24 |= 4;
    mMtxCalcsThrow[1] = nullptr;
}

void DriverModel::initAnmTrans_BossPakkun(J3DAnmTransform *anm)
{
    u16 *tbl = (u16 *)((J3DAnmTransformFull *)anm)->mTable;
    u16 jnt = mDriverLoader->getJntNo(19);
    anm->mTranslationVals[*(u16 *)((u8 *)tbl + jnt * 0x24 + 0xa)] = -22.73f;
    anm->mTranslationVals[*(u16 *)((u8 *)tbl + (jnt * 3 + 1) * 0xc + 0xa)] = -81.05f;
}

void DriverModel::InitMpaMtxCalc(StateEnum state, J3DAnmTransform *a1, J3DAnmTransform *a2,
                                 J3DAnmTransform *a3, J3DAnmTransform *a4)
{
#line 939
    JUT_MINMAX_ASSERT(0, state, 152);

    if (mDataID == 0x13) {
        if (a1) initAnmTrans_BossPakkun(a1);
        if (a2) initAnmTrans_BossPakkun(a2);
        if (a3) initAnmTrans_BossPakkun(a3);
        if (a4) initAnmTrans_BossPakkun(a4);
    }

    ((KartMtxCalcBlendAnm *)mMtxCalcs[state])->setAnm(a1, a2, a3, a4);
}

void DriverModel::InitMpaMtxCalcThrow(J3DAnmTransform *a)
{
}

void DriverModel::restart(int i)
{
    init(i);
}

void DriverModel::setShakeAnm(StateEnum s, f32 a)
{
}

void DriverModel::setShakeAnm2(StateEnum s, f32 a, int b, int c)
{
}

void DriverModel::setShakeAnm3(StateEnum s, f32 a)
{
}

void DriverModel::setDriveAnm(f32 rate)
{
    switch (mState) {
    case mcDrive:
    case (StateEnum)9:
    case (StateEnum)10:
    case (StateEnum)11:
    case (StateEnum)12:
    case (StateEnum)120:
        int i;
        for (i = 0; i < 2; i++) {
            mAnmID = msAnmBlendList[0][i];
#line 997
            JUT_MINMAX_ASSERT(0, mAnmID, 82);
            if (getAnmTransformPtr(mAnmID)) {
                f32 frameMax = getAnmTransformPtr(mAnmID)->getFrameMax() * 0.5f;
                mAnmFrame[mAnmID] = rate * frameMax + frameMax;
            }
        }
        break;
    default:
        break;
    }
}

void DriverModel::setTandemAnm(f32 a, f32 rate)
{
    if (!IsMove())
        m_b0 = rate;

    s32 state = mState;
    switch (state) {
    case mcDriveBack:
    case mcTandemB:
    case (StateEnum)50:
    case (StateEnum)51:
    case (StateEnum)53:
    case (StateEnum)54:
    case (StateEnum)70:
    case (StateEnum)71:
    case (StateEnum)77:
    case (StateEnum)79:
    case (StateEnum)81:
    case (StateEnum)82:
    case (StateEnum)85:
    case (StateEnum)86:
    case (StateEnum)89:
    case (StateEnum)91:
    case (StateEnum)104:
    case (StateEnum)105:
    case (StateEnum)106:
    case (StateEnum)107:
    case (StateEnum)111:
    case (StateEnum)117:
    case (StateEnum)136:
    case (StateEnum)137:
    case (StateEnum)140:
    case (StateEnum)141:
    case (StateEnum)142:
    case (StateEnum)143:
    case (StateEnum)146:
    case (StateEnum)147:
    case (StateEnum)148: {
        for (int i = 0; i < 2; i++) {
            mAnmID = msAnmBlendList[3][i];
#line 1050
            JUT_MINMAX_ASSERT(0, mAnmID, 82);
            if (getAnmTransformPtr(mAnmID)) {
                f32 frameMaxHalf = getAnmTransformPtr(mAnmID)->getFrameMax() * 0.5f;
                f32 target = a * frameMaxHalf + frameMaxHalf;
                if (mAnmFrame[mAnmID] > target) {
                    if (mAnmFrame[mAnmID] - target > getAnmTransformPtr(mAnmID)->getFrameMax() / 10.0f)
                        mAnmFrame[mAnmID] -= getAnmTransformPtr(mAnmID)->getFrameMax() / 10.0f;
                    else
                        mAnmFrame[mAnmID] = target;
                } else {
                    if (target - mAnmFrame[mAnmID] > getAnmTransformPtr(mAnmID)->getFrameMax() / 10.0f)
                        mAnmFrame[mAnmID] += getAnmTransformPtr(mAnmID)->getFrameMax() / 10.0f;
                    else
                        mAnmFrame[mAnmID] = target;
                }
            }
        }
        break;
    }
    case mcDriveC:
    case mcTandemAlone:
    case (StateEnum)56:
    case (StateEnum)57:
    case (StateEnum)59:
    case (StateEnum)60:
    case (StateEnum)73:
    case (StateEnum)74:
    case (StateEnum)78:
    case (StateEnum)80:
    case (StateEnum)83:
    case (StateEnum)84:
    case (StateEnum)87:
    case (StateEnum)88:
    case (StateEnum)90:
    case (StateEnum)92:
    case (StateEnum)112:
    case (StateEnum)138:
    case (StateEnum)139:
    case (StateEnum)144:
    case (StateEnum)145: {
        for (int i = 0; i < 2; i++) {
            mAnmID = msAnmBlendList[4][i];
#line 1050
            JUT_MINMAX_ASSERT(0, mAnmID, 82);
            if (getAnmTransformPtr(mAnmID)) {
                f32 frameMaxHalf = getAnmTransformPtr(mAnmID)->getFrameMax() * 0.5f;
                f32 target = a * frameMaxHalf + frameMaxHalf;
                if (mAnmFrame[mAnmID] > target) {
                    if (mAnmFrame[mAnmID] - target > getAnmTransformPtr(mAnmID)->getFrameMax() / 10.0f)
                        mAnmFrame[mAnmID] -= getAnmTransformPtr(mAnmID)->getFrameMax() / 10.0f;
                    else
                        mAnmFrame[mAnmID] = target;
                } else {
                    if (target - mAnmFrame[mAnmID] > getAnmTransformPtr(mAnmID)->getFrameMax() / 10.0f)
                        mAnmFrame[mAnmID] += getAnmTransformPtr(mAnmID)->getFrameMax() / 10.0f;
                    else
                        mAnmFrame[mAnmID] = target;
                }
            }
        }
        break;
    }
    case (StateEnum)13:
    case (StateEnum)14:
    case (StateEnum)15:
    case (StateEnum)16: {
        if (m_500 == 3) {
            for (int i = 0; i < 2; i++) {
                mAnmID = msAnmBlendList[3][i];
#line 997
                JUT_MINMAX_ASSERT(0, mAnmID, 82);
                if (getAnmTransformPtr(mAnmID)) {
                    f32 frameMaxHalf = getAnmTransformPtr(mAnmID)->getFrameMax() * 0.5f;
                    mAnmFrame[mAnmID] = a * frameMaxHalf + frameMaxHalf;
                }
            }
        }
        break;
    }
    case (StateEnum)17:
    case (StateEnum)18:
    case (StateEnum)19:
    case (StateEnum)20: {
        if (!IsDrift((StateEnum)m_500)) {
            for (int i = 0; i < 2; i++) {
                mAnmID = msAnmBlendList[4][i];
#line 997
                JUT_MINMAX_ASSERT(0, mAnmID, 82);
                if (getAnmTransformPtr(mAnmID)) {
                    f32 frameMaxHalf = getAnmTransformPtr(mAnmID)->getFrameMax() * 0.5f;
                    mAnmFrame[mAnmID] = a * frameMaxHalf + frameMaxHalf;
                }
            }
        }
        break;
    }
    case (StateEnum)5:
    case (StateEnum)96:
    case (StateEnum)98:
    case (StateEnum)100:
    case (StateEnum)102: {
        if (IsTandemAnmStartOK()) {
            for (int i = 0; i < 2; i++) {
                mAnmID = msAnmBlendList[3][i];
#line 997
                JUT_MINMAX_ASSERT(0, mAnmID, 82);
                if (getAnmTransformPtr(mAnmID)) {
                    f32 frameMaxHalf = getAnmTransformPtr(mAnmID)->getFrameMax() * 0.5f;
                    mAnmFrame[mAnmID] = a * frameMaxHalf + frameMaxHalf;
                }
            }
        }
        break;
    }
    case (StateEnum)63: {
#line 994
        JUT_MINMAX_ASSERT(0, state, 152);
        const u8 *p = (const u8 *)msAnmBlendList[state];
        int i = 0;
        state = 0;
        while (i < 2) {
            mAnmID = *(const u32 *)(p + state);
#line 997
            JUT_MINMAX_ASSERT(0, mAnmID, 82);
            if (getAnmTransformPtr(mAnmID)) {
                f32 frameMaxHalf = getAnmTransformPtr(mAnmID)->getFrameMax() * 0.5f;
                f32 one = 1.0f;
                mAnmFrame[mAnmID] = one * frameMaxHalf + frameMaxHalf;
            }
            i++;
            state += 4;
        }
        break;
    }
    case (StateEnum)64: {
#line 994
        JUT_MINMAX_ASSERT(0, state, 152);
        const u8 *p = (const u8 *)msAnmBlendList[state];
        int i = 0;
        state = 0;
        while (i < 2) {
            mAnmID = *(const u32 *)(p + state);
#line 997
            JUT_MINMAX_ASSERT(0, mAnmID, 82);
            if (getAnmTransformPtr(mAnmID)) {
                f32 frameMaxHalf = getAnmTransformPtr(mAnmID)->getFrameMax() * 0.5f;
                f32 one = 1.0f;
                mAnmFrame[mAnmID] = one * frameMaxHalf + frameMaxHalf;
            }
            i++;
            state += 4;
        }
        break;
    }
    default:
        break;
    }

    if (IsTandem() || IsDriftFinish() || IsSit()) {
        if (IsDamage())
            goto L_ED78;
        if (IsHappy())
            goto L_ED78;
        if (IsDangerPlus())
            goto L_ED78;

        if (!IsBack()) {
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x10000) != 0) {
                if (mState != 0x33) {
                    StartAnimation((StateEnum)0x88);
                    return;
                }
            }
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x8000) != 0) {
                if (mState != 0x36) {
                    StartAnimation((StateEnum)0x89);
                    return;
                }
            }
        } else {
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x10000) != 0) {
                if (mState != 0x39) {
                    StartAnimation((StateEnum)0x8a);
                    return;
                }
            }
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x8000) != 0) {
                if (mState != 0x3c) {
                    StartAnimation((StateEnum)0x8b);
                    return;
                }
            }
        }
    }
L_ED78:

    if (IsDriftEnd()) {
        if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x10000) == 0) {
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x8000) == 0) {
                if (!IsBack()) {
                    if (IsDriftLeft()) {
                        StartAnimation((StateEnum)0x33);
                    } else {
                        StartAnimation((StateEnum)0x36);
                    }
                } else {
                    if (IsDriftLeft()) {
                        StartAnimation((StateEnum)0x39);
                    } else {
                        StartAnimation((StateEnum)0x3c);
                    }
                }
            }
        }
    }

    switch (mState) {
    case (StateEnum)3:
    case (StateEnum)4:
        if (a == 0.0f) {
            if (mHokanDamageCnt == 0) {
                StartAnimation((StateEnum)(mState + 0x4a));
            }
        } else {
            mHokanDamageCnt = DriverManager::ptr()->getHokanDamageCnt();
        }
        break;
    case mcDriveBack:
    case mcDriveC:
    case (StateEnum)0x4d:
    case (StateEnum)0x4e:
        if (a != 0.0f) {
            if (!IsBack()) {
                StartAnimation((StateEnum)0x4f);
            } else {
                StartAnimation((StateEnum)0x50);
            }
        }
        break;
    default:
        break;
    }
}
void DriverModel::setTandemAnm(f32 a)
{
}

void DriverModel::Start1ShotAnm_base(DriversAnimator *animator, StateEnum state)
{
#line 1348
    JUT_MINMAX_ASSERT(0, state, 152);
    for (int i = 0; i < 2; i++) {
        int anmID = msAnmBlendList[state][i];
        if (anmID != 0) {
            f32 rate = getAnmTransformPtr(anmID)->getFrameMax() / 60.0f;
            f32 minVal, maxVal;
            switch (state) {
            case (StateEnum)10:
            case (StateEnum)12:
            case (StateEnum)14:
            case (StateEnum)16:
            case (StateEnum)18:
            case (StateEnum)20:
                maxVal = 0.25f;
                minVal = 0.0f;
                rate *= maxVal;
                break;
            case (StateEnum)37:
            case (StateEnum)43:
            case (StateEnum)106:
                maxVal = 0.5f;
                minVal = 0.0f;
                rate *= maxVal;
                break;
            case (StateEnum)39:
            case (StateEnum)45:
            case (StateEnum)117:
                minVal = 0.5f;
                maxVal = 1.0f;
                rate *= minVal;
                break;
            case (StateEnum)51:
            case (StateEnum)52:
            case (StateEnum)57:
            case (StateEnum)58:
            case (StateEnum)127:
            case (StateEnum)137:
            case (StateEnum)139:
                minVal = 1.0f;
                maxVal = 0.0f;
                break;
            case (StateEnum)38:
            case (StateEnum)41:
            case (StateEnum)44:
            case (StateEnum)47:
            case (StateEnum)50:
            case (StateEnum)53:
            case (StateEnum)56:
            case (StateEnum)59:
                continue;
            default:
                minVal = 0.0f;
                maxVal = 1.0f;
                break;
            }
            animator->mOneShotAnm[i].mMinValue = minVal;
            animator->mOneShotAnm[i].mMaxValue = maxVal;
            animator->mOneShotAnm[i].mRate = rate;
            animator->mOneShotAnm[i].mTime =
                (animator->mOneShotAnm[i].mMaxValue - animator->mOneShotAnm[i].mMinValue) /
                animator->mOneShotAnm[i].mRate;
            animator->mOneShotAnm[i].mFrame = 0;
            animator->mOneShotAnm[i].mMaxFrame = (s32)(60.0f * animator->mOneShotAnm[i].mRate) << 4;
            animator->mOneShotAnm[i].mStepFrame = 0x10;
            animator->mOneShotAnm[i].mIsFinished = 1;
            animator->mOneShotAnm[i].reset();
            animator->mOneShotAnm[i].start();
            animator->mOneShotAnm[i].update();
        }
    }
}
void DriverModel::StartAnimation2()
{
    s32 state = mState;
#line 1429
    JUT_MINMAX_ASSERT(0, state, 152);
    for (int i = 0; i < 2; i++) {
        int anmID = msAnmBlendList[state][i];
        if (anmID != 0 && getAnmTransformPtr(anmID) == nullptr)
            return;
    }

    s32 before_mState_Hokan = mBeforeState;
#line 1439
    JUT_MINMAX_ASSERT(0, before_mState_Hokan, 152);
    if (m_ac != 0) {
        for (int i = 0; i < 2; i++) {
            int anmID = msAnmBlendList[mBeforeState][i];
            if (anmID != 0 && getAnmTransformPtr(anmID) == nullptr)
                return;
        }
    }

    if (m_d4 == 0)
        return;
    if (m_d4 == 2)
        goto L_main;
    if (m_d4 >= 2)
        goto L_end;
    if (m_d4 >= 1) {
        if (IsDemo((StateEnum)state)) {
            mAnmID = msAnmBlendList[state][0];
            f32 frameMax = getAnmTransformPtr(mAnmID)->getFrameMax();
            StartLoopAnm((StateEnum)state, &mDriveAnim, 0.0f, 1.0f, 60.0f / frameMax, 0.3f * frameMax);
            goto L_end;
        }

        if (state < 63) {
            if (state >= 21)
                goto L_common1;
            if (state >= 13)
                goto L_copy;
            goto L_common1;
        }
        if (state < 65)
            goto L_copy;
        goto L_common1;

    L_copy:
        mTandemAnim = mDriveAnim;
    L_common1:
        Start1ShotAnm_base(&mDriveAnim, (StateEnum)state);
        goto L_end;
    }
    goto L_end;

L_main:
    switch (state) {
    case 69: case 72: case 75: case 76: case 77: case 78:
    case 93: case 108: case 109: case 110: case 118: case 119:
    case 123: case 124: case 128: case 129: case 130: case 131:
    case 132: case 133: case 135: case 149:
        mTandemAnim = mDriveAnim;
        goto L_loop;

    case 70: case 71: case 73: case 74:
        if (mBeforeState == 0x48 || (mBeforeState >= 0x45 && mBeforeState < 0x4d)) {
            mTandemAnim.mLoopAnim.mCurrentValue = mDriveAnim.mLoopAnim.mCurrentValue;
            mTandemAnim.mLoopAnim.mFrameCounter = mDriveAnim.mLoopAnim.mFrameCounter;
            mTandemAnim.mLoopAnim.mAnimationState = mDriveAnim.mLoopAnim.mAnimationState;
            mTandemAnim.mLoopAnim.mMinValue = mDriveAnim.mLoopAnim.mMinValue;
            mTandemAnim.mLoopAnim.mMaxValue = mDriveAnim.mLoopAnim.mMaxValue;
            mTandemAnim.mLoopAnim.mSpeed = mDriveAnim.mLoopAnim.mSpeed;
            mTandemAnim.mLoopAnim.mCurrentPhase = mDriveAnim.mLoopAnim.mCurrentPhase;
        } else {
            StartLoopAnm2(&mTandemAnim, 0.0f, 1.0f, 1.0f, 0.0f);
        }
        break;

    case 79: case 80: case 81: case 82: case 83: case 84:
    case 111: case 112: case 113: case 114: case 115: case 116: case 120:
        mTandemAnim = mDriveAnim;
        break;

    case 85: case 86: case 87: case 88: case 89: case 90: case 91: case 92:
        Start1ShotAnm_base(&mDriveAnim, (StateEnum)state);
        break;

    default:
        mTandemAnim = mDriveAnim;
        Start1ShotAnm_base(&mDriveAnim, (StateEnum)state);
        break;
    }
    goto L_end;

L_loop:
    mAnmID = msAnmBlendList[state][0];
    mDriveAnim.reset();
    {
        f32 frameMax = getAnmTransformPtr(mAnmID)->getFrameMax();
        mDriveAnim.mLoopAnim.stop();
        StartLoopAnm2(&mDriveAnim, 0.0f, 1.0f, 60.0f / frameMax, 0.0f);
    }
    mDriveAnim.mLoopAnim.update();
    mDriveAnim.mLoopAnim.start();

L_end:
    m_d4 = 0;
}

void DriverModel::StartAnimationWC(StateEnum s)
{
}

void DriverModel::StartAnimation(StateEnum state)
{
#line 1639
    JUT_ASSERT((int)state >= mcDrive);
    if (mState == (int)state)
        return;
    if (IsDemo())
        return;
    if (m_e8)
        return;
    if (m_c8 != 0) {
        if ((int)state >= 0x6e)
            goto L_go;
        if ((int)state >= 0x6c)
            return;
        goto L_go;
    }
L_go:;

    m_c8 = 2;
    SetDriverFlags(state);

    if (m_ac == 0)
        goto no_ac;

    if ((int)state == 0x48)
        goto simple;
    if ((int)state >= 0x48)
        goto L_ge48;
    if ((int)state >= 0x46)
        goto mid;
    goto simple;

L_ge48:
    if ((int)state >= 0x4b)
        goto simple;
    goto mid;

mid:
    if (mState == 0x48)
        goto L_keep;
    if (mState >= 0x48)
        goto L_ge49;
    if (mState == 0x45)
        goto L_keep;
    goto L_use;

L_ge49:
    if (mState >= 0x4d)
        goto L_use;
    if (mState >= 0x4b) {
        mBeforeState = mState;
        mState = state;
        return;
    }
    goto L_use;
    goto L_use;

L_keep:
    mBeforeState = mState;
    mState = state;
    return;
L_use:
    mBeforeState = state;
    mState = state;
    return;

simple:
    mBeforeState = mState;
    next_before_mState_Hokan = mBeforeState;
    mState = state;
    return;

no_ac:
    mState = state;
    m_c8 = 1;
}

void DriverModel::setCommon1ShotAnm_Single(f32 a, int b)
{
}

void DriverModel::setCommon1ShotAnm(f32 a)
{
}

void DriverModel::setCommon1ShotAnm_Single2(int b, f32 a, int d)
{
}

void DriverModel::setCommon1ShotAnm2(int b, f32 a)
{
}

void DriverModel::setCommonAnm_Base(int b, f32 a, int d)
{
}

void DriverModel::setCommonAnm2(int b, f32 a)
{
}

void DriverModel::setCommonAnm(f32 a)
{
}

void DriverModel::setCommonAnm_Blend(f32 a, int c)
{
}

void DriverModel::Change_next_mState(StateEnum s)
{
}

void DriverModel::updateFrame()
{
    f32 rate = m_668.mCurrentValue;
    for (int blend_num = 0; blend_num < 2; blend_num++) {
#line 1789
        JUT_MINMAX_ASSERT(0, blend_num, 2);
        mAnmID = msAnmBlendList[0x96][blend_num];
#line 1791
        JUT_MINMAX_ASSERT(0, mAnmID, 82);
        if (mAnmID != 0 && getAnmTransformPtr(mAnmID)) {
            s32 frame = (s32)(rate * getAnmTransformPtr(mAnmID)->getFrameMax() + 0.5f);
            if (frame == getAnmTransformPtr(mAnmID)->getFrameMax())
                frame = 0;
            mAnmFrame[mAnmID] = frame;
        }
    }
    m_668.update();

    rate = m_688.mCurrentValue;
    for (int blend_num = 0; blend_num < 2; blend_num++) {
#line 1789
        JUT_MINMAX_ASSERT(0, blend_num, 2);
        mAnmID = msAnmBlendList[0x97][blend_num];
#line 1791
        JUT_MINMAX_ASSERT(0, mAnmID, 82);
        if (mAnmID != 0 && getAnmTransformPtr(mAnmID)) {
            s32 frame = (s32)(rate * getAnmTransformPtr(mAnmID)->getFrameMax() + 0.5f);
            if (frame == getAnmTransformPtr(mAnmID)->getFrameMax())
                frame = 0;
            mAnmFrame[mAnmID] = frame;
        }
    }
    m_688.update();

    if (m_c8 == 0) {
        updateFrame_base(&mDriveAnim, (StateEnum)mState);
        if (m_ac != 0) {
            updateFrame_base(&mTandemAnim, (StateEnum)mBeforeState);
        } else if (mState < 63) {
            if (mState >= 21) {
            } else if (mState >= 13) {
                goto add;
            }
        } else {
            if (mState < 65)
                goto add;
        }
        goto done;
    add:
        updateFrame_base(&mAdditiveAnim, (StateEnum)m_500);
    done:;
    }
}

void DriverModel::updateFrame_base(DriversAnimator *animator, StateEnum state)
{
}

void DriverModel::updateSetFrame_base(StateEnum s)
{
}

void DriverModel::updateSetFrame()
{
    const u32 (*list)[2] = this->msAnmBlendList;
    {
        s32 state = mState;
#line 2381
        JUT_MINMAX_ASSERT(0, state, 152);
        const u8 *p = (const u8 *)list[state];
        int blend_num = 0;
        state = 0;
        while (blend_num < 2) {
            mAnmID = *(const u32 *)(p + state);
#line 2384
            JUT_MINMAX_ASSERT(0, mAnmID, 82);
            if (getAnmTransformPtr(mAnmID) != nullptr) {
                f32 frame = mAnmFrame[mAnmID];
                getAnmTransformPtr(mAnmID)->setFrame(frame);
#line 2390
                JUT_MINMAX_ASSERT(0, mAnmID_debug_index, 8);
                mAnmWorkFrame[mAnmID_debug_index] = mAnmFrame[mAnmID];
                mAnmWorkID[mAnmID_debug_index] = mAnmID;
                mAnmID_debug_index++;
            }
            blend_num++;
            state += 4;
        }
    }

    if (m_ac != 0) {
        s32 state = mBeforeState;
#line 2381
        JUT_MINMAX_ASSERT(0, state, 152);
        const u8 *p = (const u8 *)list[state];
        int blend_num = 0;
        state = 0;
        while (blend_num < 2) {
            mAnmID = *(const u32 *)(p + state);
#line 2384
            JUT_MINMAX_ASSERT(0, mAnmID, 82);
            if (getAnmTransformPtr(mAnmID) != nullptr) {
                f32 frame = mAnmFrame[mAnmID];
                getAnmTransformPtr(mAnmID)->setFrame(frame);
#line 2390
                JUT_MINMAX_ASSERT(0, mAnmID_debug_index, 8);
                mAnmWorkFrame[mAnmID_debug_index] = mAnmFrame[mAnmID];
                mAnmWorkID[mAnmID_debug_index] = mAnmID;
                mAnmID_debug_index++;
            }
            blend_num++;
            state += 4;
        }
    }

    if (mState < 21) {
        if (mState < 17) {
            if (mState >= 13)
                goto L_use3;
        } else {
            goto L_use4;
        }
    } else if (mState < 65) {
        if (mState >= 63)
            goto L_use3;
    }
    goto done;

L_use3:
    {
        s32 state = m_500;
#line 2381
        JUT_MINMAX_ASSERT(0, state, 152);
        const u8 *p = (const u8 *)list[state];
        int blend_num = 0;
        state = 0;
        while (blend_num < 2) {
            mAnmID = *(const u32 *)(p + state);
#line 2384
            JUT_MINMAX_ASSERT(0, mAnmID, 82);
            if (getAnmTransformPtr(mAnmID) != nullptr) {
                f32 frame = mAnmFrame[mAnmID];
                getAnmTransformPtr(mAnmID)->setFrame(frame);
#line 2390
                JUT_MINMAX_ASSERT(0, mAnmID_debug_index, 8);
                mAnmWorkFrame[mAnmID_debug_index] = mAnmFrame[mAnmID];
                mAnmWorkID[mAnmID_debug_index] = mAnmID;
                mAnmID_debug_index++;
            }
            blend_num++;
            state += 4;
        }
    }
    goto L_item;

L_use4:
    {
        s32 state = m_500;
#line 2381
        JUT_MINMAX_ASSERT(0, state, 152);
        const u8 *p = (const u8 *)list[state];
        int blend_num = 0;
        state = 0;
        while (blend_num < 2) {
            mAnmID = *(const u32 *)(p + state);
#line 2384
            JUT_MINMAX_ASSERT(0, mAnmID, 82);
            if (getAnmTransformPtr(mAnmID) != nullptr) {
                f32 frame = mAnmFrame[mAnmID];
                getAnmTransformPtr(mAnmID)->setFrame(frame);
#line 2390
                JUT_MINMAX_ASSERT(0, mAnmID_debug_index, 8);
                mAnmWorkFrame[mAnmID_debug_index] = mAnmFrame[mAnmID];
                mAnmWorkID[mAnmID_debug_index] = mAnmID;
                mAnmID_debug_index++;
            }
            blend_num++;
            state += 4;
        }
    }

L_item:
    if (mItem != nullptr && m_b8 != 0 && mItem->getSuccessionItemNum() > 1) {
        const u8 *p = (const u8 *)list[0x96];
        int blend_num = 0;
        int state = 0;
        while (blend_num < 2) {
            mAnmID = *(const u32 *)(p + state);
#line 2384
            JUT_MINMAX_ASSERT(0, mAnmID, 82);
            if (getAnmTransformPtr(mAnmID) != nullptr) {
                f32 frame = mAnmFrame[mAnmID];
                getAnmTransformPtr(mAnmID)->setFrame(frame);
#line 2390
                JUT_MINMAX_ASSERT(0, mAnmID_debug_index, 8);
                mAnmWorkFrame[mAnmID_debug_index] = mAnmFrame[mAnmID];
                mAnmWorkID[mAnmID_debug_index] = mAnmID;
                mAnmID_debug_index++;
            }
            blend_num++;
            state += 4;
        }
    }

    if (m_b9 != 0) {
        const u8 *p = (const u8 *)list[0x97];
        int blend_num = 0;
        int state = 0;
        while (blend_num < 2) {
            mAnmID = *(const u32 *)(p + state);
#line 2384
            JUT_MINMAX_ASSERT(0, mAnmID, 82);
            if (getAnmTransformPtr(mAnmID) != nullptr) {
                f32 frame = mAnmFrame[mAnmID];
                getAnmTransformPtr(mAnmID)->setFrame(frame);
#line 2390
                JUT_MINMAX_ASSERT(0, mAnmID_debug_index, 8);
                mAnmWorkFrame[mAnmID_debug_index] = mAnmFrame[mAnmID];
                mAnmWorkID[mAnmID_debug_index] = mAnmID;
                mAnmID_debug_index++;
            }
            blend_num++;
            state += 4;
        }
    }
done:;
}

void DriverModel::updateLoadCommand(int a)
{
}

void DriverModel::updateLoadCommand_Sound(int a)
{
}

void DriverModel::updateLoad_Animation()
{
#line 2465
    JUT_MINMAX_ASSERT(0, next_mState_Hokan, 152);
    mAnimBufMgr->resetFrame();
    int anmID;

    if ((u8)CheckDriverFlags_Hokan((StateEnum)next_mState_Hokan)) {
#line 2471
        JUT_MINMAX_ASSERT(0, next_before_mState_Hokan, 152);
        anmID = msAnmBlendList[next_before_mState_Hokan][0];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        anmID = msAnmBlendList[next_before_mState_Hokan][1];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        anmID = msAnmBlendList[next_mState_Hokan][0];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        anmID = msAnmBlendList[next_mState_Hokan][1];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        return;
    }

    switch (next_mState_Hokan) {
    case 13:
    case 14:
    case 15:
    case 16: {
        anmID = msAnmBlendList[next_mState_Hokan][0];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        if (mState >= 126) {
            if (mState >= 128)
                m_500 = 3;
            else
                m_500 = mState;
        } else {
            if (mState >= 55)
                m_500 = 3;
            else if (mState >= 49)
                m_500 = mState;
            else
                m_500 = 3;
        }
        anmID = msAnmBlendList[m_500][0];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        anmID = msAnmBlendList[m_500][1];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        break;
    }
    case 17:
    case 18:
    case 19:
    case 20: {
        anmID = msAnmBlendList[next_mState_Hokan][0];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        if (mState >= 55 && mState < 61)
            m_500 = mState;
        else
            m_500 = 4;
        anmID = msAnmBlendList[m_500][0];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        anmID = msAnmBlendList[m_500][1];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        break;
    }
    case 9:
    case 10:
    case 11:
    case 12: {
        anmID = msAnmBlendList[next_mState_Hokan][0];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        m_500 = 0;
        anmID = msAnmBlendList[m_500][0];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        break;
    }
    case 63:
    case 64: {
        anmID = msAnmBlendList[next_mState_Hokan][0];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        m_500 = next_mState_Hokan + 63;
        anmID = msAnmBlendList[m_500][0];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        anmID = msAnmBlendList[m_500][1];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        break;
    }
    default: {
        anmID = msAnmBlendList[next_mState_Hokan][0];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        anmID = msAnmBlendList[next_mState_Hokan][1];
        if (!IsAnmResInMRAM(anmID))
            mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
        break;
    }
    }
}

void DriverModel::updateLoad_Sound()
{
}

void DriverModel::updateLoad()
{
    if (mState == next_mState_Hokan && m_504 == 0)
        return;

    updateLoad_Animation();
    mAnimBufMgr->resetFrameSound();

    s32 next_mState = next_mState_Hokan;
#line 2565
    JUT_MINMAX_ASSERT(0, next_mState, 152);
    u8 anmID = (u8)msAnmBlendList[next_mState_Hokan][0];
    DriverLoader::checkAnmID(anmID);
    int sndNo = DriverLoader::mscAnm2SoundList[anmID];
    if (!IsAnmSoundResInMRAM(sndNo))
        mAnimBufMgr->CheckAndLoadAnimSoundAvailable(sndNo, mDataID);
    if (mAnimBufMgr->mRes0 == 0)
        m_cc = 1;
    if (mAnimBufMgr->mRes4 == 0)
        m_d0 = 1;
}

#pragma inline_depth(0)
int DriverModel::updateCheckRenew_Animation()
{
    int result = mAnimBufMgr->CheckRenew();
    if (result == -1)
        return 0;
    if (result != 1) {
        if (m_cc == 0)
            goto L_367C;
    }
#line 2598
    JUT_MINMAX_ASSERT(0, mBeforeState, 152);
    mState = mState;
#line 2599
    JUT_MINMAX_ASSERT(0, mState, 152);
#line 2600
    JUT_MINMAX_ASSERT(0, m_500, 152);
    const u32 (*list)[2] = msAnmBlendList;
    if (m_ac != 0) {
        J3DAnmTransform *pa1 = getAnmTransformPtr(list[mState][1]);
        J3DAnmTransform *pa0 = getAnmTransformPtr(list[mState][0]);
        J3DAnmTransform *pb1 = getAnmTransformPtr(list[mBeforeState][1]);
        J3DAnmTransform *pb0 = getAnmTransformPtr(list[mBeforeState][0]);
        InitMpaMtxCalc((StateEnum)mState, pb0, pb1, pa0, pa1);
        J3DAnmTransform *pb150_1 = getAnmTransformPtr(list[0x96][1]);
        InitMpaMtxCalc((StateEnum)0x96, getAnmTransformPtr(list[0x96][0]), pb150_1, nullptr, nullptr);
        if (getAnmTransformPtr(list[0x97][0]) != nullptr) {
            J3DAnmTransform *pb151_1 = getAnmTransformPtr(list[0x97][1]);
            InitMpaMtxCalc((StateEnum)0x97, getAnmTransformPtr(list[0x97][0]), pb151_1, nullptr, nullptr);
        }
        goto L_367C;
    }

    if (mState < 21) {
        if (mState < 13) {
            if (mState >= 9)
                goto L_useThrow;
            goto L_default;
        }
        if (mState >= 17)
            goto L_useThrow;
        goto L_useThree;
    }
    if (mState >= 65)
        goto L_default;
    if (mState >= 63)
        goto L_useThree;
    goto L_default;

L_useThree:
    {
        J3DAnmTransform *throwAnm = getAnmTransformPtr(list[m_500][0]);
#line 970
        JUT_MINMAX_ASSERT(0, mState, 152);
        ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->setAnmTransform(0, throwAnm);
        ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->setAnmTransform(1, getAnmTransformPtr(list[mState][0]));
        ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->_28 = 20.0f;
        J3DAnmTransform *pa1 = getAnmTransformPtr(list[m_500][1]);
        InitMpaMtxCalc((StateEnum)3, getAnmTransformPtr(list[m_500][0]), pa1, nullptr, nullptr);
        J3DAnmTransform *qb1 = getAnmTransformPtr(list[mState][1]);
        InitMpaMtxCalc((StateEnum)mState, getAnmTransformPtr(list[mState][0]), qb1, nullptr, nullptr);
    }
    goto L_367C;

L_useThrow:
    {
        J3DAnmTransform *throwAnm = getAnmTransformPtr(list[m_500][0]);
#line 970
        JUT_MINMAX_ASSERT(0, mState, 152);
        ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->setAnmTransform(0, throwAnm);
        ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->setAnmTransform(1, getAnmTransformPtr(list[mState][0]));
        ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->_28 = 20.0f;
        J3DAnmTransform *pa1 = getAnmTransformPtr(list[m_500][1]);
        InitMpaMtxCalc((StateEnum)m_500, getAnmTransformPtr(list[m_500][0]), pa1, nullptr, nullptr);
        J3DAnmTransform *qb1 = getAnmTransformPtr(list[mState][1]);
        InitMpaMtxCalc((StateEnum)mState, getAnmTransformPtr(list[mState][0]), qb1, nullptr, nullptr);
    }
    goto L_367C;

L_default:
    {
        J3DAnmTransform *pa1 = getAnmTransformPtr(list[mState][1]);
        InitMpaMtxCalc((StateEnum)mState, getAnmTransformPtr(list[mState][0]), pa1, nullptr, nullptr);
        J3DAnmTransform *pb150_1 = getAnmTransformPtr(list[0x96][1]);
        InitMpaMtxCalc((StateEnum)0x96, getAnmTransformPtr(list[0x96][0]), pb150_1, nullptr, nullptr);
        if (getAnmTransformPtr(list[0x97][0]) != nullptr) {
            J3DAnmTransform *pb151_1 = getAnmTransformPtr(list[0x97][1]);
            InitMpaMtxCalc((StateEnum)0x97, getAnmTransformPtr(list[0x97][0]), pb151_1, nullptr, nullptr);
        }
    }
L_367C:
    m_cc = 0;
    return 1;
}


#pragma inline_depth(4)
void DriverModel::updateCheckRenew_Sound()
{
}

int DriverModel::updateCheckRenew()
{
    int result = updateCheckRenew_Animation();
    if (result != 0) {
        int renewSound = mAnimBufMgr->CheckRenewSound();
        if (renewSound == -1) {
            result = 0;
        } else {
            if (renewSound == 1 || m_d0 != 0) {
                u8 anmID = (u8)msAnmBlendList[next_mState_Hokan][0];
                DriverLoader::checkAnmID(anmID);
                void *sound = mAnimBufMgr->GetSoundPtr(DriverLoader::mscAnm2SoundList[anmID]);
                GameAudio::CharacterSoundMgr *mgr = mCharSoundMgr;
                mgr->mAnimator->startAnimation((const JAUSoundAnimation *)sound, false, 1.0f, 1.0f);
                *((u8 *)mgr->mStarter + 4) = mgr->getKartSound()->mKartCount;
            }
            m_d0 = 0;
            result = 1;
        }
    }
#line 2684
    JUT_ASSERT(result != 0);
    return result;
}

void DriverModel::BlendValue(f32 a)
{
}

void DriverModel::updateBlendCount_Func1(u8 a, u8 b)
{
}

void DriverModel::updateBlendCount_Func2(u8 a)
{
}

void DriverModel::updateBlendCount()
{
    if (m_b8 != 0 && !(*(s32 *)((u8 *)mItem + 0x118) == 2 && *(u32 *)((u8 *)mItem + 0x7c) == 7)) {
        if (DriverManager::ptr()->mBlendTimes[0] > m_ba)
            m_ba++;
    } else {
        if (m_ba != 0)
            m_ba--;
    }

    int blendTime = (u8)updateSetBlendWeight_GetBlendTime((StateEnum)mState);

    switch (mState) {
    case 42:
    case 48:
    case 69:
    case 72:
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
    case 97:
    case 99:
    case 101:
    case 103:
    case 104:
    case 106:
    case 107:
    case 108:
    case 109:
    case 110:
    case 117:
    case 118:
    case 119:
    case 121:
    case 122:
    case 123:
    case 124:
    case 125:
    case 126:
    case 127:
    case 128:
    case 129:
    case 130:
    case 131:
    case 132:
    case 133:
    case 134:
    case 135:
    case 136:
    case 137:
    case 138:
    case 139:
    case 146:
    case 147:
    case 148:
    case 149:
        if (m_bb < blendTime)
            m_bb++;
        else
            m_bb = blendTime;
        break;

    case 120:
        if (m_bb < blendTime) {
            m_bb++;
            if (m_bb == blendTime && m_e8 == 0 && m_504 == 0 && mState == next_mState_Hokan)
                next_mState_Hokan = 0;
        } else {
            m_bb = blendTime;
        }
        break;

    case 70:
    case 71:
    case 73:
    case 74:
        if (m_bb < blendTime) {
            m_bb++;
            if (m_bb == blendTime) {
                if (mState == 72)
                    goto L_bend;
                if (mState >= 72)
                    goto L_bge72;
                if (mState == 70)
                    goto L_b70;
                if (mState >= 70)
                    goto L_b111;
                goto L_bend;
            L_bge72:
                if (mState == 74)
                    goto L_b112;
                if (mState >= 74)
                    goto L_bend;
                goto L_b4;
            L_b70:
                if (m_e8 == 0 && m_504 == 0 && mState == next_mState_Hokan)
                    next_mState_Hokan = 3;
                goto L_bend;
            L_b4:
                if (m_e8 == 0 && m_504 == 0 && mState == next_mState_Hokan)
                    next_mState_Hokan = 4;
                goto L_bend;
            L_b111:
                if (m_e8 == 0 && m_504 == 0 && mState == next_mState_Hokan)
                    next_mState_Hokan = 111;
                goto L_bend;
            L_b112:
                if (m_e8 == 0 && m_504 == 0 && mState == next_mState_Hokan)
                    next_mState_Hokan = 112;
            L_bend:;
            }
        } else {
            m_bb = blendTime;
        }
        break;

    case 75:
    case 76:
        if (m_bb > blendTime)
            m_bb = blendTime;
        if (m_e0 == 0) {
            if (m_bb < blendTime)
                m_bb++;
        } else {
            if (m_bb != 0)
                m_bb--;
            if (m_bb == 0) {
                m_e0 = 0;
                if (mState == 76)
                    goto L_c4;
                if (mState >= 76)
                    goto L_cend;
                if (mState >= 75)
                    goto L_c3;
                goto L_cend;
            L_c3:
                if (m_e8 == 0 && m_504 == 0 && mState == next_mState_Hokan)
                    next_mState_Hokan = 3;
                goto L_cend;
            L_c4:
                if (m_e8 == 0 && m_504 == 0 && mState == next_mState_Hokan)
                    next_mState_Hokan = 4;
            L_cend:;
            }
        }
        break;

    case 77:
    case 78:
    case 79:
    case 80: {
        if (m_bb < blendTime) {
            m_bb++;
            if (m_bb == blendTime) {
                int nes = mState - 76;
#line 1839
                JUT_ASSERT((int)nes >= mcDrive);
                if (m_e8 == 0 && m_504 == 0) {
                    if (next_mState_Hokan != mState) {
                        if (nes < 104) {
                            if (nes < 7) {
                                if (nes >= 5)
                                    goto L_set_d;
                            } else if (nes >= 96) {
                                goto L_set_d;
                            }
                        } else {
                            if (nes >= 123)
                                goto L_done_d;
                            if (nes >= 121)
                                goto L_set_d;
                            goto L_done_d;
                        }
                        goto L_done_d;
                    L_set_d:
                        next_mState_Hokan = nes;
                    L_done_d:;
                    }
                }
            }
        }
        break;
    }

    case 81:
    case 82:
    case 83:
    case 84:
        if (m_bb < blendTime) {
            m_bb++;
            if (m_bb >= blendTime) {
                if (m_e8 == 0 && m_504 == 0) {
                    if (next_mState_Hokan != mState) {
                        if (mState < 83) {
                            if (mState >= 81)
                                next_mState_Hokan = 3;
                        } else {
                            next_mState_Hokan = 4;
                        }
                    }
                }
            }
        }
        break;

    case 85:
    case 86:
    case 87:
    case 88:
    case 140:
    case 141:
    case 142:
    case 143:
    case 144:
    case 145:
        m_bb++;
#line 2714
        JUT_MINMAX_ASSERT(0, mState, 152);
        mAnmID = msAnmBlendList[mState][0];
        if (mAnmID != 0 && getAnmTransformPtr(mAnmID) != nullptr) {
            s16 frameMax = getAnmTransformPtr(mAnmID)->getFrameMax();
            s32 frame = (s32)(getAnmTransformPtr(mAnmID)->getFrame() + 0.5f);
            s16 t0 = DriverManager::ptr()->mBlendTimes[8];
            s16 t1 = DriverManager::ptr()->mBlendTimes[9];
            if (frame - frameMax <= t1)
                m_bb = t1 - frameMax - frame;
            else if (frameMax > t0)
                m_bb = 0;
        }
        break;

    case 89:
    case 90:
        m_bb++;
#line 2714
        JUT_MINMAX_ASSERT(0, mState, 152);
        mAnmID = msAnmBlendList[mState][0];
        if (mAnmID != 0 && getAnmTransformPtr(mAnmID) != nullptr) {
            s16 frameMax = getAnmTransformPtr(mAnmID)->getFrameMax();
            s32 frame = (s32)(getAnmTransformPtr(mAnmID)->getFrame() + 0.5f);
            s16 t0 = DriverManager::ptr()->mBlendTimes[10];
            s16 t1 = DriverManager::ptr()->mBlendTimes[11];
            if (frame - frameMax <= t1)
                m_bb = t1 - frameMax - frame;
            else if (frameMax > t0)
                m_bb = 0;
        }
        break;

    case 96:
    case 98:
    case 100:
    case 102:
        m_bb++;
#line 2714
        JUT_MINMAX_ASSERT(0, mState, 152);
        mAnmID = msAnmBlendList[mState][0];
        if (mAnmID != 0 && getAnmTransformPtr(mAnmID) != nullptr) {
            s16 frameMax = getAnmTransformPtr(mAnmID)->getFrameMax();
            s32 frame = (s32)(getAnmTransformPtr(mAnmID)->getFrame() + 0.5f);
            s16 t0 = DriverManager::ptr()->mBlendTimes[17];
            s16 t1 = DriverManager::ptr()->mBlendTimes[18];
            if (frame - frameMax <= t1)
                m_bb = t1 - frameMax - frame;
            else if (frameMax > t0)
                m_bb = 0;
        }
        break;

    case 105:
        m_bb++;
#line 2714
        JUT_MINMAX_ASSERT(0, mState, 152);
        mAnmID = msAnmBlendList[mState][0];
        if (mAnmID != 0 && getAnmTransformPtr(mAnmID) != nullptr) {
            s16 frameMax = getAnmTransformPtr(mAnmID)->getFrameMax();
            s32 frame = (s32)(getAnmTransformPtr(mAnmID)->getFrame() + 0.5f);
            s16 t0 = DriverManager::ptr()->mBlendTimes[21];
            s16 t1 = DriverManager::ptr()->mBlendTimes[22];
            if (frame - frameMax <= t1)
                m_bb = t1 - frameMax - frame;
            else if (frameMax > t0)
                m_bb = 0;
        }
        break;

    case 111:
    case 112: {
        if (m_bb < blendTime) {
            m_bb++;
            if (m_bb == blendTime) {
                int nes = mState - 108;
#line 1839
                JUT_ASSERT((int)nes >= mcDrive);
                if (m_e8 == 0 && m_504 == 0) {
                    if (next_mState_Hokan != mState) {
                        if (nes < 104) {
                            if (nes < 7) {
                                if (nes >= 5)
                                    goto L_set_j;
                            } else if (nes >= 96) {
                                goto L_set_j;
                            }
                        } else {
                            if (nes >= 123)
                                goto L_done_j;
                            if (nes >= 121)
                                goto L_set_j;
                            goto L_done_j;
                        }
                        goto L_done_j;
                    L_set_j:
                        next_mState_Hokan = nes;
                    L_done_j:;
                    }
                }
            }
        }
        break;
    }

    case 113:
    case 114:
    case 115:
    case 116: {
        if (m_bb < blendTime) {
            m_bb++;
            if (m_bb == blendTime) {
                int nes = (mState - 113) * 3 + 50;
#line 1839
                JUT_ASSERT((int)nes >= mcDrive);
                if (m_e8 == 0 && m_504 == 0) {
                    if (next_mState_Hokan != mState) {
                        if (nes < 104) {
                            if (nes < 7) {
                                if (nes >= 5)
                                    goto L_set_k;
                            } else if (nes >= 96) {
                                goto L_set_k;
                            }
                        } else {
                            if (nes >= 123)
                                goto L_done_k;
                            if (nes >= 121)
                                goto L_set_k;
                            goto L_done_k;
                        }
                        goto L_done_k;
                    L_set_k:
                        next_mState_Hokan = nes;
                    L_done_k:;
                    }
                }
            }
        }
        break;
    }

    default:
        break;
    }
}

void DriverModel::updateSetBlendWeight_Func1(u8 a, u8 b)
{
}

void DriverModel::updateSetBlendWeight_FuncABCD(f32 a, f32 b, f32 c, f32 d)
{
}

int DriverModel::updateSetBlendWeight_GetBlendTime(StateEnum stt)
{
    switch (stt) {
    case 0: case 1: case 3:
    case 37: case 38: case 39: case 40: case 41:
    case 49: case 50: case 51: case 52: case 53: case 54:
        return DriverManager::ptr()->getBlendTime(0);
    case 42: case 48:
        return 10;
    case 69: case 72:
        return DriverManager::ptr()->getBlendTime(1);
    case 70: case 71: case 73: case 74:
        return DriverManager::ptr()->getBlendTime(2);
    case 75: case 76:
        return m_e0 == 0 ? DriverManager::ptr()->getBlendTime(3) : DriverManager::ptr()->getBlendTime(4);
    case 77: case 78:
        return DriverManager::ptr()->getBlendTime(5);
    case 79: case 80:
        return DriverManager::ptr()->getBlendTime(6);
    case 81: case 82: case 83: case 84:
        return DriverManager::ptr()->getBlendTime(7);
    case 108: case 109:
        return DriverManager::ptr()->getBlendTime(25);
    case 110:
        return DriverManager::ptr()->getBlendTime(27);
    case 91: case 92:
        return DriverManager::ptr()->getBlendTime(12);
    case 93: case 149:
        return DriverManager::ptr()->getBlendTime(14);
    case 94:
        return DriverManager::ptr()->getBlendTime(15);
    case 95:
        return DriverManager::ptr()->getBlendTime(16);
    case 97: case 99: case 101: case 103: case 121:
        return DriverManager::ptr()->getBlendTime(18);
    case 96: case 98: case 100: case 102: case 122:
        return DriverManager::ptr()->getBlendTime(17);
    case 104:
        return DriverManager::ptr()->getBlendTime(19);
    case 105:
        return DriverManager::ptr()->getBlendTime(21);
    case 106: case 107:
        return DriverManager::ptr()->getBlendTime(24);
    case 117:
        return DriverManager::ptr()->getBlendTime(28);
    case 111:
        if (mBeforeState < 108) {
            if (mBeforeState == 74)
                return DriverManager::ptr()->getBlendTime(13);
            if (mBeforeState < 74) {
                if (mBeforeState == 71)
                    return DriverManager::ptr()->getBlendTime(13);
                return DriverManager::ptr()->getBlendTime(26);
            }
            if (mBeforeState >= 93)
                return DriverManager::ptr()->getBlendTime(26);
            if (mBeforeState >= 91)
                return DriverManager::ptr()->getBlendTime(13);
            return DriverManager::ptr()->getBlendTime(26);
        }
        if (mBeforeState == 117)
            return DriverManager::ptr()->getBlendTime(29);
        if (mBeforeState < 117)
            return DriverManager::ptr()->getBlendTime(26);
        if (mBeforeState >= 125)
            return DriverManager::ptr()->getBlendTime(26);
        if (mBeforeState >= 123)
            return DriverManager::ptr()->getBlendTime(33);
        return DriverManager::ptr()->getBlendTime(26);
    case 112:
        if (mBeforeState < 125 && mBeforeState >= 123)
            return DriverManager::ptr()->getBlendTime(33);
        return DriverManager::ptr()->getBlendTime(26);
    case 113: case 114: case 115: case 116:
        return DriverManager::ptr()->getBlendTime(30);
    case 118: case 119:
        return DriverManager::ptr()->getBlendTime(31);
    case 120:
        return DriverManager::ptr()->getBlendTime(32);
    case 123: case 124:
        return DriverManager::ptr()->getBlendTime(34);
    case 125:
        return DriverManager::ptr()->getBlendTime(35);
    case 126: case 127:
        return DriverManager::ptr()->getBlendTime(35);
    case 128: case 129: case 130: case 131: case 132: case 133: case 134: case 135:
        return DriverManager::ptr()->getBlendTime(35);
    case 136: case 137: case 138: case 139:
        return DriverManager::ptr()->getBlendTime(38);
    default:
        return 5;
    }
}


void DriverModel::updateSetBlendWeight_mStateThrow()
{
}

void DriverModel::updateSetBlendWeight()
{
}

void DriverModel::update_mIsSpeakBCA()
{
}

#pragma inline_depth(0)
void DriverModel::update(u16 param)
{
    for (int i = 0; i < 8; i++) {
        mAnmWorkFrame[i] = 0.0f;
        mAnmWorkID[i] = -1;
    }
    mAnmID_debug_index = 0;

#line 3678
    JUT_ASSERT_F(mState >= 0, "%s", "mState >= 0");

    if (m_e8 == 0 && m_514 > 0) {
        m_514--;
        if (m_514 == 0) {
            StartAnimation((StateEnum)m_510);
            m_510 = -1;
        }
    }

    if (param != 0) {
        ExModel::update(param);
        return;
    }

    sCurDriver = this;

    if (m_e8 != 0) {
        mState = m_4f4;
        mBeforeState = m_4f8;
    }

    next_mState_Hokan = mState;
    next_before_mState_Hokan = mBeforeState;

    if (m_4f4 != mState) {
        m_504 = 1;
        mState = m_4f4;
        mBeforeState = m_4f8;
    } else {
        m_504 = 0;
    }

    CheckDriverFlags();

    if (m_e8 == 0) {
        mTPAnim->update();

        if (IsDanger()) {
            if (m_be <= 0)
                m_be = 1;
            else if (m_be < DriverManager::ptr()->_5b)
                m_be++;
        } else {
            m_be = 0;
        }

        if (m_c0 != 0) {
            m_c0--;
            if (m_c0 == 0) {
                if (mKartLoader->mDriverModels[0]->IsPiyo())
                    mKartLoader->mDriverModels[0]->StartAnimation((StateEnum)122);
                else
                    mKartLoader->mDriverModels[1]->StartAnimation((StateEnum)122);
            }
        }

        if (IsGoal()) {
            if (m_c2 < 0)
                m_c2 = 0;
            if (m_c2 > 0) {
                if (m_c2 < DriverManager::ptr()->_5c * 60)
                    m_c2++;
            }
        } else {
            if (m_c2 != 1)
                m_c2 = 0;
        }

        if (IsHappy()) {
            if (m_c4 <= 0) {
                m_c4 = 1;
            } else {
                if (IsHappyFront()) {
                    s16 limit;
                    if (DriverManager::ptr()->_838 != 0)
                        limit = DriverManager::ptr()->_83a;
                    else
                        limit = (s16)DriverData::msDriverData[mDataID].data[84];
                    if (m_c4 < limit)
                        m_c4++;
                }
                if (IsHappyBack()) {
                    s16 limit;
                    if (DriverManager::ptr()->_838 != 0)
                        limit = DriverManager::ptr()->_83c;
                    else
                        limit = (s16)DriverData::msDriverData[mDataID].data[85];
                    if (m_c4 < limit)
                        m_c4++;
                }
            }
        } else {
            m_c4 = 0;
        }

        if (mHokanDamageCnt != 0)
            mHokanDamageCnt--;

        if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 2) != 0)
            m_e4 = 1;
        else
            m_e4 = 0;
    }

    int renew = updateCheckRenew();

    if (m_e8 == 0)
        updateBlendCount();

    if (m_d4 != 0)
        StartAnimation2();
    m_d4 = 0;

    if (!PauseManager::tstPause() && renew != 0) {
        if (m_e8 == 0)
            updateFrame();
        updateSetFrame();
    }

    if (renew == 0) {
        getModelData()->getJointNodePointer(0)->mMtxCalc = nullptr;

        u16 jnt = mDriverLoader->getJntBindNo(1);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = nullptr;
        jnt = mDriverLoader->getJntBindNo(10);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = nullptr;
        jnt = mDriverLoader->getJntBindNo(14);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = nullptr;

        jnt = mDriverLoader->getJntBindNo(18);
        if (jnt != 0xffff) {
            J3DJoint *jnt18 = getModelData()->getJointNodePointer(jnt);
            if (jnt18 != nullptr)
                jnt18->mMtxCalc = nullptr;
        }

        goto L_after;
    }

    updateSetBlendWeight();

    if (m_e8 != 0) {
        m_b9 = 0;
    } else if (mCharSoundMgr->isSpeak()) {
        if (DriverManager::ptr()->getSpeakFlag(mDataID) != 0) {
            if (IsAttack() || IsDamage() || IsDash() || IsRocketStart() || IsGoal() || IsDanger()) {
                m_b9 = 0;
            } else {
                if (m_b9 == 0)
                    StartSpeak();
                m_b9 = 1;
            }
        } else {
            m_b9 = 0;
        }
    } else {
        m_b9 = 0;
    }

#line 3875
    JUT_MINMAX_ASSERT(0, mState, 152);
#line 3876
    JUT_MINMAX_ASSERT(0, m_500, 152);

    if (mState < 21) {
        if (mState < 13) {
            if (mState >= 9)
                goto L_useThrow;
            goto L_default;
        }
        if (mState >= 17)
            goto L_useThrow;
        goto L_useThree;
    }
    if (mState >= 65)
        goto L_default;
    if (mState >= 63)
        goto L_useThree;
    goto L_default;

L_useThree:
    if (mState < 17) {
#line 3206
        JUT_MINMAX_ASSERT(0, m_500, 152);
        if (mMtxCalcs[m_500] != nullptr) {
            mMtxCalcs[m_500]->setWeight(0, 1.0f - (f32)m_ba / (f32)DriverManager::ptr()->mBlendTimes[0]);
            mMtxCalcs[m_500]->setWeight(1, (f32)m_ba / (f32)DriverManager::ptr()->mBlendTimes[0]);
            if (DriverData::mspDriverData->m_1a != 0) {
                mMtxCalcs[m_500]->setWeight(0, 1.0f);
                mMtxCalcs[m_500]->setWeight(1, 0.0f);
            }
        }

        getModelData()->getJointNodePointer(0)->mMtxCalc = mMtxCalcs[m_500];

        u16 jnt = mDriverLoader->getJntBindNo(10);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = mMtxCalcs[mState];
        jnt = mDriverLoader->getJntBindNo(14);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = mMtxCalcs[mState];
        jnt = mDriverLoader->getJntBindNo(18);
        if (jnt != 0xffff) {
            J3DJoint *jnt18 = getModelData()->getJointNodePointer(jnt);
            if (jnt18 != nullptr)
                jnt18->mMtxCalc = mMtxCalcs[mState];
        }
    } else if (mState < 21) {
        getModelData()->getJointNodePointer(0)->mMtxCalc = mMtxCalcs[m_500];

        u16 jnt = mDriverLoader->getJntBindNo(10);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = mMtxCalcs[mState];
        jnt = mDriverLoader->getJntBindNo(14);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = mMtxCalcs[mState];
        jnt = mDriverLoader->getJntBindNo(18);
        if (jnt != 0xffff) {
            J3DJoint *jnt18 = getModelData()->getJointNodePointer(jnt);
            if (jnt18 != nullptr)
                jnt18->mMtxCalc = mMtxCalcs[mState];
        }
    } else {
        getModelData()->getJointNodePointer(0)->mMtxCalc = mMtxCalcs[m_500];

        u16 jnt = mDriverLoader->getJntBindNo(10);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = (J3DMtxCalc *)mMtxCalcsThrow[0];
        jnt = mDriverLoader->getJntBindNo(14);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = mMtxCalcs[mState];
        jnt = mDriverLoader->getJntBindNo(18);
        if (jnt != 0xffff) {
            J3DJoint *jnt18 = getModelData()->getJointNodePointer(jnt);
            if (jnt18 != nullptr)
                jnt18->mMtxCalc = mMtxCalcs[mState];
        }
    }
    goto L_after;

L_useThrow:
    getModelData()->getJointNodePointer(0)->mMtxCalc = mMtxCalcs[0];

    {
        u16 jnt = mDriverLoader->getJntBindNo(1);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = (J3DMtxCalc *)mMtxCalcsThrow[0];
        jnt = mDriverLoader->getJntBindNo(10);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = mMtxCalcs[mState];
        jnt = mDriverLoader->getJntBindNo(14);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = mMtxCalcs[mState];
        jnt = mDriverLoader->getJntBindNo(18);
        if (jnt != 0xffff) {
            J3DJoint *jnt18 = getModelData()->getJointNodePointer(jnt);
            if (jnt18 != nullptr)
                jnt18->mMtxCalc = mMtxCalcs[mState];
        }
    }
    goto L_after;

L_default:
    getModelData()->getJointNodePointer(0)->mMtxCalc = mMtxCalcs[mState];

    {
        u16 jnt = mDriverLoader->getJntBindNo(1);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = nullptr;
        jnt = mDriverLoader->getJntBindNo(10);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = nullptr;

        int set = 0;
        if (mItem != nullptr && m_b8 != 0 && mItem->getSuccessionItemNum() > 1 &&
            *(u32 *)((u8 *)mItem + 0x2d4) == 0) {
            jnt = mDriverLoader->getJntBindNo(14);
            if (jnt != 0xffff)
                getModelData()->getJointNodePointer(jnt)->mMtxCalc = mMtxCalcs[0x96];
            set = 1;
        }
        if (!set) {
            jnt = mDriverLoader->getJntBindNo(14);
            if (jnt != 0xffff)
                getModelData()->getJointNodePointer(jnt)->mMtxCalc = nullptr;
        }

        jnt = mDriverLoader->getJntBindNo(18);
        if (jnt != 0xffff)
            getModelData()->getJointNodePointer(jnt)->mMtxCalc = nullptr;

        if (m_b9 != 0) {
            jnt = mDriverLoader->getJntBindNo(18);
            if (jnt != 0xffff) {
                J3DJoint *jnt18 = getModelData()->getJointNodePointer(jnt);
                if (jnt18 != nullptr)
                    jnt18->mMtxCalc = mMtxCalcs[0x97];
            }
        }
    }

L_after:
    if (!PauseManager::tstPause())
        SetDriverOffsetPosition();

    ExModel::update(param);
    LaurelMgr::ptr()->update(this);

    mPos.x = getModel()->getBaseTRMtx()[0][3];
    mPos.y = getModel()->getBaseTRMtx()[1][3];
    mPos.z = getModel()->getBaseTRMtx()[2][3];

    mAnmID = msAnmBlendList[mState][0];
    mCharSoundMgr->frameWork(mAnmFrame[mAnmID], 1.0f);
    mAnimBufMgr->Check();

    if (mState != next_mState_Hokan) {
        if (next_mState_Hokan < 106) {
            if (next_mState_Hokan < 63) {
                if (next_mState_Hokan == 54)
                    goto L_start_anim;
                if (next_mState_Hokan >= 54)
                    goto L_finish;
                if (next_mState_Hokan == 51)
                    goto L_start_anim;
                goto L_finish;
            }
            if (next_mState_Hokan == 69)
                goto L_start_anim;
            if (next_mState_Hokan >= 69)
                goto L_finish;
            if (next_mState_Hokan >= 65)
                goto L_finish;
            goto L_start_anim;
        }
        if (next_mState_Hokan == 120)
            goto L_start_anim;
        if (next_mState_Hokan >= 120) {
            if (next_mState_Hokan == 135)
                goto L_start_anim;
            goto L_finish;
        }
        if (next_mState_Hokan >= 110) {
            if (next_mState_Hokan >= 117)
                goto L_finish;
            goto L_start_anim;
        }
        if (next_mState_Hokan >= 108)
            goto L_finish;
        goto L_start_anim;

    L_start_anim:
        StartAnimation((StateEnum)next_mState_Hokan);
        m_504 = 1;
    L_finish:;
        m_bb = 0;
        m_d4 = m_c8;
        m_c8 = 0;
    }

    if (IsDemo() && m_c8 == 1) {
        m_d4 = m_c8;
        m_c8 = 0;
        m_504 = 1;
    }

    if (mState != next_mState_Hokan || m_504 != 0) {
        updateLoad_Animation();
        mAnimBufMgr->resetFrameSound();

#line 2565
        JUT_MINMAX_ASSERT(0, next_mState_Hokan, 152);
        u8 anmID = (u8)msAnmBlendList[next_mState_Hokan][0];
        DriverLoader::checkAnmID(anmID);
        int sndNo = DriverLoader::mscAnm2SoundList[anmID];
        if (!IsAnmSoundResInMRAM(sndNo))
            mAnimBufMgr->CheckAndLoadAnimSoundAvailable(sndNo, mDataID);

        if (mAnimBufMgr->mRes0 == 0)
            m_cc = 1;
        if (mAnimBufMgr->mRes4 == 0)
            m_d0 = 1;
    }

#line 4031
    JUT_ASSERT_F(mState >= 0, "%s", "mState >= 0");

    mState = next_mState_Hokan;
    mBeforeState = next_before_mState_Hokan;
    m_4f4 = mState;
    m_4f8 = mBeforeState;
    sCurDriver = nullptr;

    if ((mLumbarCtrl->m_70 & 4) != 0) {
        if (!IsDamageL() && !IsThrow() && !IsDash()) {
            mLumbarCtrl->m_70 &= ~4;
            mLumbarCtrl->m_70 &= ~2;
        }
    }
}

#pragma inline_depth(4)
bool DriverModel::createModel(JKRSolidHeap *heap, u32 a, u32 b)
{
    if (ExModel::createDifferedModel(heap, a, 0x1080200, true)) {
        m_d8 = 1;
        makeSharedDL();
        m_d8 = 0;
        lock();
        FeelingMgr::ptr()->createModel(this, heap, a, b);

        if (mCharID == 19)
            goto L_create_done;
        if (mCharID >= 19)
            goto L_create_done;
        if (mCharID >= 18)
            goto L_create_osage;
        goto L_create_done;
    L_create_osage:
        OsageMgr::ptr()->createModel(this, heap, a, b);
    L_create_done:;
    }

    LaurelMgr::ptr()->createModel(this, heap, a, b);
    mTPAnim->setup(this);
    return true;
}

void DriverModel::setCurrentViewNo(u32 viewNo)
{
    ExModel::setCurrentViewNo(viewNo);

    if (mCharID == 19)
        goto L_view_done;
    if (mCharID >= 19)
        goto L_view_done;
    if (mCharID >= 18)
        goto L_view_osage;
    goto L_view_done;
L_view_osage:
    OsageMgr::ptr()->setCurrentViewNo(this, viewNo);
L_view_done:;
    LaurelMgr::ptr()->setCurrentViewNo(this, viewNo);

    MtxPtr mtx = RaceMgr::getManager()->getKartDrawer(mKartNum)->getLight(viewNo)->getEffectMtx();
    setEffectMtx(mtx, 1);
}

void DriverModel::setItem(ItemObj *item)
{
    if (!m_b8)
        StartJuggle();
    m_b8 = 1;
    mItem = item;
}

void DriverModel::resetItem()
{
    m_b8 = 0;
    mItem = nullptr;
}

void DriverModel::setLODLevel(int a)
{
}

Mtx *DriverModel::getMtxP_HandR()
{
    return &getModel()->mMtxBuffer->getAnmMtx(mDriverLoader->getJntNo(16));
}

Mtx *DriverModel::getMtxP_HandR2()
{
    return &getModel()->mMtxBuffer->getAnmMtx(mDriverLoader->getJntNo(17));
}

Mtx *DriverModel::getMtxP(int ctrlNo)
{
#line 4209
    JUT_MINMAX_ASSERT(0, ctrlNo, 27);
    int jntNo = mDriverLoader->getJntBindNo(ctrlNo);
    if (jntNo == 0xffff)
        return nullptr;
    return &getModel()->mMtxBuffer->getAnmMtx(jntNo);
}

J3DAnmTransform *DriverModel::getAnmTransformPtr(int anmID)
{
    int keepID = DriverLoader::convAnmIDToKeepID((DriverLoader::AnmEnum)anmID);
    if (keepID >= 0)
        return mKeepAnmTrans[keepID];
    J3DAnmTransform *anm = (J3DAnmTransform *)mAnimBufMgr->GetPtr(anmID);
    if (anm != nullptr)
        return anm;
    return nullptr;
}

int DriverModel::IsAnmResInMRAM(int anmID)
{
    int ret = 0;
    if (DriverLoader::convAnmIDToKeepID((DriverLoader::AnmEnum)anmID) >= 0)
        ret = 1;
    return ret;
}

int DriverModel::IsAnmSoundResInMRAM(int sndID)
{
    int ret = 0;
    if (DriverLoader::convSndIDToKeepID((DriverLoader::AnmSoundEnum)sndID) >= 0)
        ret = 1;
    return ret;
}

bool DriverModel::IsARAMDMAFinished()
{
    return false;
}

__declspec(weak) DriverModel::~DriverModel()
{
}
