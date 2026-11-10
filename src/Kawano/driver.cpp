#include "Kawano/driver.h"

#include "JSystem/J3D/J3DAnmTransform.h"
#include "JSystem/J3D/J3DAnmLoader.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DMtxCalc.h"
#include "JSystem/JUtility/JUTAssert.h"

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
    u8 _0[0xf0];
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


class DriverTPAnim // size 0x220
{
public:
    DriverTPAnim(DriverModel *, int);
    void setup(DriverModel *);
    void update();
    u8 _0[0x220];
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

    mDriveAnim.reset();
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

    m_4f4 = mState;
    m_4f8 = mState;
    mBeforeState = mState;
    m_500 = 3;
    m_504 = 1;
    mNext_mState_Hokan = mState;
    m_50c = mBeforeState;
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

void DriverModel::restart(int i)
{
    init(i);
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
        for (int i = 0; i < 2; i++) {
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

    switch (mState) {
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
        JUT_MINMAX_ASSERT(0, mState, 152);
        for (int i = 0; i < 2; i++) {
            mAnmID = msAnmBlendList[mState][i];
#line 997
            JUT_MINMAX_ASSERT(0, mAnmID, 82);
            if (getAnmTransformPtr(mAnmID)) {
                f32 frameMaxHalf = getAnmTransformPtr(mAnmID)->getFrameMax() * 0.5f;
                mAnmFrame[mAnmID] = 1.0f * frameMaxHalf + frameMaxHalf;
            }
        }
        break;
    }
    case (StateEnum)64: {
#line 994
        JUT_MINMAX_ASSERT(0, mState, 152);
        for (int i = 0; i < 2; i++) {
            mAnmID = msAnmBlendList[mState][i];
#line 997
            JUT_MINMAX_ASSERT(0, mAnmID, 82);
            if (getAnmTransformPtr(mAnmID)) {
                f32 frameMaxHalf = getAnmTransformPtr(mAnmID)->getFrameMax() * 0.5f;
                mAnmFrame[mAnmID] = 1.0f * frameMaxHalf + frameMaxHalf;
            }
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
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartNum) & 0x10000) != 0) {
                if (mState != 0x33) {
                    StartAnimation((StateEnum)0x88);
                    return;
                }
            }
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartNum) & 0x8000) != 0) {
                if (mState != 0x36) {
                    StartAnimation((StateEnum)0x89);
                    return;
                }
            }
        } else {
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartNum) & 0x10000) != 0) {
                if (mState != 0x39) {
                    StartAnimation((StateEnum)0x8a);
                    return;
                }
            }
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartNum) & 0x8000) != 0) {
                if (mState != 0x3c) {
                    StartAnimation((StateEnum)0x8b);
                    return;
                }
            }
        }
    }
L_ED78:

    if (IsDriftEnd()) {
        if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartNum) & 0x10000) == 0) {
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartNum) & 0x8000) == 0) {
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
    if (m_c8 != 0 && (int)state < 0x6e && (int)state >= 0x6c)
        return;

    m_c8 = 2;
    SetDriverFlags(state);

    if (m_ac == 0)
        goto no_ac;

    if ((int)state == 0x48)
        goto simple;
    if ((int)state >= 0x48) {
        if ((int)state >= 0x4b)
            goto simple;
        goto mid;
    }
    if ((int)state >= 0x46)
        goto mid;
    goto simple;

simple:
    mBeforeState = mState;
    m_50c = mBeforeState;
    mState = state;
    return;

mid:
    if (mState == 0x48 || mState == 0x45 || (mState >= 0x4b && mState < 0x4d)) {
        mBeforeState = mState;
    } else {
        mBeforeState = state;
    }
    mState = state;
    return;

no_ac:
    mState = state;
    m_c8 = 1;
}

int DriverModel::updateCheckRenew_Animation()
{
    int result = mAnimBufMgr->CheckRenew();
    if (result == -1)
        return 0;
    if (result != 1) {
        if (m_cc == 0) {
            m_cc = 0;
            return 1;
        }
    }
#line 2598
    JUT_MINMAX_ASSERT(0, mBeforeState, 152);
    mState = mState;
#line 2599
    JUT_MINMAX_ASSERT(0, mState, 152);
#line 2600
    JUT_MINMAX_ASSERT(0, m_500, 152);
    if (m_ac != 0) {
        J3DAnmTransform *pa1 = getAnmTransformPtr(msAnmBlendList[mState][1]);
        J3DAnmTransform *pa0 = getAnmTransformPtr(msAnmBlendList[mState][0]);
        J3DAnmTransform *pb1 = getAnmTransformPtr(msAnmBlendList[mBeforeState][1]);
        J3DAnmTransform *pb0 = getAnmTransformPtr(msAnmBlendList[mBeforeState][0]);
        InitMpaMtxCalc((StateEnum)mState, pb0, pb1, pa0, pa1);
        J3DAnmTransform *pb150_1 = getAnmTransformPtr(msAnmBlendList[0x96][1]);
        InitMpaMtxCalc((StateEnum)0x96, getAnmTransformPtr(msAnmBlendList[0x96][0]), pb150_1, nullptr, nullptr);
        if (getAnmTransformPtr(msAnmBlendList[0x97][0]) != nullptr) {
            J3DAnmTransform *pb151_1 = getAnmTransformPtr(msAnmBlendList[0x97][1]);
            InitMpaMtxCalc((StateEnum)0x97, getAnmTransformPtr(msAnmBlendList[0x97][0]), pb151_1, nullptr, nullptr);
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
        J3DAnmTransform *throwAnm = getAnmTransformPtr(msAnmBlendList[m_500][0]);
#line 970
        JUT_MINMAX_ASSERT(0, mState, 152);
        ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->setAnmTransform(0, throwAnm);
        ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->setAnmTransform(1, getAnmTransformPtr(msAnmBlendList[mState][0]));
        ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->_28 = 20.0f;
        J3DAnmTransform *pa1 = getAnmTransformPtr(msAnmBlendList[m_500][1]);
        InitMpaMtxCalc((StateEnum)3, getAnmTransformPtr(msAnmBlendList[m_500][0]), pa1, nullptr, nullptr);
        J3DAnmTransform *qb1 = getAnmTransformPtr(msAnmBlendList[mState][1]);
        InitMpaMtxCalc((StateEnum)mState, getAnmTransformPtr(msAnmBlendList[mState][0]), qb1, nullptr, nullptr);
    }
    goto L_367C;

L_useThrow:
    {
        J3DAnmTransform *throwAnm = getAnmTransformPtr(msAnmBlendList[m_500][0]);
#line 970
        JUT_MINMAX_ASSERT(0, mState, 152);
        ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->setAnmTransform(0, throwAnm);
        ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->setAnmTransform(1, getAnmTransformPtr(msAnmBlendList[mState][0]));
        ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->_28 = 20.0f;
        J3DAnmTransform *pa1 = getAnmTransformPtr(msAnmBlendList[m_500][1]);
        InitMpaMtxCalc((StateEnum)m_500, getAnmTransformPtr(msAnmBlendList[m_500][0]), pa1, nullptr, nullptr);
        J3DAnmTransform *qb1 = getAnmTransformPtr(msAnmBlendList[mState][1]);
        InitMpaMtxCalc((StateEnum)mState, getAnmTransformPtr(msAnmBlendList[mState][0]), qb1, nullptr, nullptr);
    }
    goto L_367C;

L_default:
    {
        J3DAnmTransform *pa1 = getAnmTransformPtr(msAnmBlendList[mState][1]);
        InitMpaMtxCalc((StateEnum)mState, getAnmTransformPtr(msAnmBlendList[mState][0]), pa1, nullptr, nullptr);
        J3DAnmTransform *pb150_1 = getAnmTransformPtr(msAnmBlendList[0x96][1]);
        InitMpaMtxCalc((StateEnum)0x96, getAnmTransformPtr(msAnmBlendList[0x96][0]), pb150_1, nullptr, nullptr);
        if (getAnmTransformPtr(msAnmBlendList[0x97][0]) != nullptr) {
            J3DAnmTransform *pb151_1 = getAnmTransformPtr(msAnmBlendList[0x97][1]);
            InitMpaMtxCalc((StateEnum)0x97, getAnmTransformPtr(msAnmBlendList[0x97][0]), pb151_1, nullptr, nullptr);
        }
    }
L_367C:
    m_cc = 0;
    return 1;
}

