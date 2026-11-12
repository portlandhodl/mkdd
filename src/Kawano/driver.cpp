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
    void setFlags()
    {
        m_70 |= 2;
        m_70 |= 4;
    }
    void clearFlags()
    {
        m_70 &= ~4;
        m_70 &= ~2;
    }
    u8 _0[0x70];
    u8 m_70;
    u8 _71[3];
    s32 m_74;
    u8 _78[0xf0 - 0x78];
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

const s8 DriverModel::msNext_mState_HokanDamages[2] = {110, 41};

static const f32 sZeroQuatset[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

const s32 DriverModel::msAnmBlendList[152][2] = {
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

inline J3DJoint *getDriverJoint(DriverLoader *loader, ExModel *model, u16 i)
{
    J3DJoint *jnt = nullptr;
    u16 jntNo = loader->getJntBindNo(i);
    if (jntNo != 0xffff)
        jnt = model->getModelData()->getJointNodePointer(jntNo);
    return jnt;
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

        for (u16 i = 0; i < 27; i++) {
            J3DJoint *jnt = getDriverJoint(mDriverLoader, this, i);
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
    switch (stt) {
    case 3:
        StartDWait();
        break;
    }

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
            u32 mtxCalcType = getModelData()->getMtxCalcType();
#line 836
            JUT_ASSERT(mtxCalcType == J3DMdlDataFlag_MtxCalcTypeMaya);
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

#line 968
void DriverModel::InitMpaMtxCalcThrow(J3DAnmTransform *a)
{
    JUT_MINMAX_ASSERT(0, mState, 152);
    ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->setAnmTransform(0, a);
    ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->setAnmTransform(1, getAnmTransformPtr(msAnmBlendList[mState][0]));
    ((KartMtxCalcAdditiveAnm *)mMtxCalcsThrow[0])->_28 = 20.0f;
}

void DriverModel::restart(int i)
{
    init(i);
}

#line 992
void DriverModel::setShakeAnm(StateEnum state, f32 rate)
{
    JUT_MINMAX_ASSERT(0, state, 152);
    for (int blend_num = 0; blend_num < 2; blend_num++) {
        mAnmID = msAnmBlendList[state][blend_num];
        JUT_MINMAX_ASSERT(0, mAnmID, 82);
        if (getAnmTransformPtr(mAnmID)) {
            f32 frameMaxHalf = getAnmTransformPtr(mAnmID)->getFrameMax() * 0.5f;
            mAnmFrame[mAnmID] = rate * frameMaxHalf + frameMaxHalf;
        }
    }
}

void DriverModel::setShakeAnm2(StateEnum state, f32 rate, int start, int end)
{
    JUT_MINMAX_ASSERT(0, state, 152);
    for (int blend_num = start; blend_num < end; blend_num++) {
        mAnmID = msAnmBlendList[state][blend_num];
        JUT_MINMAX_ASSERT(0, mAnmID, 82);
        if (getAnmTransformPtr(mAnmID)) {
            f32 frameMaxHalf = getAnmTransformPtr(mAnmID)->getFrameMax() * 0.5f;
            mAnmFrame[mAnmID] = rate * frameMaxHalf + frameMaxHalf;
        }
    }
}

#line 1045
void DriverModel::setShakeAnm3(StateEnum state, f32 a)
{
    JUT_MINMAX_ASSERT(0, state, 152);
    for (int blend_num = 0; blend_num < 2; blend_num++) {
        mAnmID = msAnmBlendList[state][blend_num];
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
        setShakeAnm(mcDrive, rate);
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
        setShakeAnm3((StateEnum)3, a);
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
        setShakeAnm3((StateEnum)4, a);
        break;
    }
    case (StateEnum)13:
    case (StateEnum)14:
    case (StateEnum)15:
    case (StateEnum)16: {
        if (m_500 == 3) {
            setShakeAnm((StateEnum)3, a);
        }
        break;
    }
    case (StateEnum)17:
    case (StateEnum)18:
    case (StateEnum)19:
    case (StateEnum)20: {
        if (!IsDrift((StateEnum)m_500)) {
            setShakeAnm((StateEnum)4, a);
        }
        break;
    }
    case (StateEnum)5:
    case (StateEnum)96:
    case (StateEnum)98:
    case (StateEnum)100:
    case (StateEnum)102: {
        if (IsTandemAnmStartOK()) {
            setShakeAnm((StateEnum)3, a);
        }
        break;
    }
    case (StateEnum)63:
        setShakeAnm((StateEnum)mState, 1.0f);
        break;
    case (StateEnum)64:
        setShakeAnm((StateEnum)mState, 1.0f);
        break;
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
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x1000000000000ull) != 0) {
                if (mState != 0x33) {
                    StartAnimation((StateEnum)0x88);
                    return;
                }
            }
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x800000000000ull) != 0) {
                if (mState != 0x36) {
                    StartAnimation((StateEnum)0x89);
                    return;
                }
            }
        } else {
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x1000000000000ull) != 0) {
                if (mState != 0x39) {
                    StartAnimation((StateEnum)0x8a);
                    return;
                }
            }
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x800000000000ull) != 0) {
                if (mState != 0x3c) {
                    StartAnimation((StateEnum)0x8b);
                    return;
                }
            }
        }
    }
L_ED78:

    if (IsDriftEnd()) {
        if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x1000000000000ull) == 0) {
            if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x800000000000ull) == 0) {
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
                return;
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
    setTandemAnm(a, 0.0f);
}

void DriverModel::Start1ShotAnm_base(DriversAnimator *animator, StateEnum state)
{
#line 1348
    JUT_MINMAX_ASSERT(0, state, 152);
    for (int i = 0; i < 2; i++) {
        int anmID = msAnmBlendList[state][i];
        if (anmID != 0) {
            f32 minVal, maxVal, rate;
            rate = getAnmTransformPtr(anmID)->getFrameMax() / 60.0f;
            switch (state) {
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

#line 1439
    JUT_ASSERT_F(0 <= mBeforeState && mBeforeState < 152, "range over: %d <= before_mState_Hokan=%d < %d", 0, mBeforeState, 152);
    if (m_ac != 0) {
        for (int i = 0; i < 2; i++) {
            int anmID = msAnmBlendList[mBeforeState][i];
            if (anmID != 0 && getAnmTransformPtr(anmID) == nullptr)
                return;
        }
    }

    if (m_d4 == 0)
        return;

    switch (m_d4) {
    case 1:
        if (IsDemo((StateEnum)state)) {
            mAnmID = msAnmBlendList[state][0];
            StartLoopAnm((StateEnum)state, &mDriveAnim, 0.0f, 1.0f,
                         60.0f / getAnmTransformPtr(mAnmID)->getFrameMax(), 0.3f * mDriverNo);
        } else {
            switch (state) {
            case mcS013:
            case mcS014:
            case mcS015:
            case mcS016:
            case mcS017:
            case mcS018:
            case mcS019:
            case mcS020:
            case mcS063:
            case mcS064:
                mAdditiveAnim = mDriveAnim;
                break;
            }
            Start1ShotAnm_base(&mDriveAnim, (StateEnum)mState);
        }
        break;
    case 2:
        switch (state) {
        case mcS069:
        case mcS072:
        case mcS075:
        case mcS076:
        case mcS077:
        case mcS078:
        case mcS093:
        case mcS108:
        case mcS109:
        case mcS110:
        case mcS118:
        case mcS119:
        case mcS123:
        case mcS124:
        case mcS128:
        case mcS129:
        case mcS130:
        case mcS131:
        case mcS132:
        case mcS133:
        case mcS135:
        case mcS149:
            mTandemAnim = mDriveAnim;
            mAnmID = msAnmBlendList[state][0];
            mDriveAnim.mLoopAnim.reset();
            StartLoopAnm2(&mDriveAnim, 0.0f, 1.0f, 60.0f / getAnmTransformPtr(mAnmID)->getFrameMax(), 0.0f);
            mDriveAnim.mLoopAnim.stop();
            mDriveAnim.mLoopAnim.update();
            mDriveAnim.mLoopAnim.start();
            break;
        case mcS070:
        case mcS071:
        case mcS073:
        case mcS074:
            switch (mBeforeState) {
            case mcS069:
            case mcS072:
            case mcS075:
            case mcS076:
                mTandemAnim.mLoopAnim = mDriveAnim.mLoopAnim;
                break;
            default:
                StartLoopAnm2(&mTandemAnim, 0.0f, 1.0f, 1.0f, 0.0f);
                break;
            }
            break;
        case mcS079:
        case mcS080:
        case mcS081:
        case mcS082:
        case mcS083:
        case mcS084:
        case mcS111:
        case mcS112:
        case mcS113:
        case mcS114:
        case mcS115:
        case mcS116:
        case mcS120:
            mTandemAnim = mDriveAnim;
            break;
        case mcS085:
        case mcS086:
        case mcS087:
        case mcS088:
        case mcS089:
        case mcS090:
        case mcS091:
        case mcS092:
            Start1ShotAnm_base(&mDriveAnim, (StateEnum)state);
            break;
        default:
            mTandemAnim = mDriveAnim;
            Start1ShotAnm_base(&mDriveAnim, (StateEnum)state);
            break;
        }
        break;
    }
    m_d4 = 0;
}

void DriverModel::StartAnimationWC(StateEnum s)
{
}

void DriverModel::StartAnimation(StateEnum state)
{
#line 1639
    JUT_ASSERT(state >= mcDrive);
    if (mState == (int)state)
        return;
    if (IsDemo())
        return;
    if (m_e8)
        return;
    if (m_c8 != 0) {
        switch (state) {
        case mcS108:
        case mcS109:
            return;
        default:
            break;
        }
    }

    m_c8 = 2;
    SetDriverFlags(state);

    if (m_ac != 0) {
        switch (state) {
        case mcS070:
        case mcS071:
        case mcS073:
        case mcS074:
            switch (mState) {
            case mcS069:
            case mcS072:
            case mcS075:
            case mcS076:
                mBeforeState = *(volatile s32 *)&mState;
                mState = state;
                break;
            default:
                mBeforeState = state;
                mState = state;
                break;
            }
            break;
        default:
            mBeforeState = mState;
            next_before_mState_Hokan = mBeforeState;
            mState = state;
            break;
        }
    } else {
        mState = state;
        m_c8 = 1;
    }
}

#line 1728
void DriverModel::setCommon1ShotAnm_Single(f32 rate, int blend_num)
{
    JUT_MINMAX_ASSERT(0, mState, 152);
    JUT_MINMAX_ASSERT(0, blend_num, 2);
    mAnmID = msAnmBlendList[mState][blend_num];
    JUT_MINMAX_ASSERT(0, mAnmID, 82);
    if (getAnmTransformPtr(mAnmID)) {
        mAnmFrame[mAnmID] = (s32)(rate * getAnmTransformPtr(mAnmID)->getFrameMax() + 0.5f);
    }
}

void DriverModel::setCommon1ShotAnm(f32 rate)
{
    for (int blend_num = 0; blend_num < 2; blend_num++) {
        setCommon1ShotAnm_Single(rate, blend_num);
    }
}

#line 1762
void DriverModel::setCommon1ShotAnm_Single2(int stt, f32 rate, int blend_num)
{
    JUT_MINMAX_ASSERT(0, stt, 152);
    JUT_MINMAX_ASSERT(0, blend_num, 2);
    mAnmID = msAnmBlendList[stt][blend_num];
    JUT_MINMAX_ASSERT(0, mAnmID, 82);
    if (getAnmTransformPtr(mAnmID)) {
        mAnmFrame[mAnmID] = (s32)(rate * getAnmTransformPtr(mAnmID)->getFrameMax() + 0.5f);
    }
}

void DriverModel::setCommon1ShotAnm2(int stt, f32 rate)
{
    for (int blend_num = 0; blend_num < 2; blend_num++) {
        setCommon1ShotAnm_Single2(stt, rate, blend_num);
    }
}

#line 1786
void DriverModel::setCommonAnm_Base(int stt, f32 rate, int blend_num)
{
    JUT_MINMAX_ASSERT(0, stt, 152);
    JUT_MINMAX_ASSERT(0, blend_num, 2);
    mAnmID = msAnmBlendList[stt][blend_num];
    JUT_MINMAX_ASSERT(0, mAnmID, 82);
    if (mAnmID != 0 && getAnmTransformPtr(mAnmID)) {
        s32 frame = (s32)(rate * getAnmTransformPtr(mAnmID)->getFrameMax() + 0.5f);
        if (frame == getAnmTransformPtr(mAnmID)->getFrameMax())
            frame = 0;
        mAnmFrame[mAnmID] = frame;
    }
}

void DriverModel::setCommonAnm2(int stt, f32 rate)
{
    for (int blend_num = 0; blend_num < 2; blend_num++) {
        setCommonAnm_Base(stt, rate, blend_num);
    }
}

void DriverModel::setCommonAnm(f32 rate)
{
    for (int blend_num = 0; blend_num < 2; blend_num++) {
        setCommonAnm_Base(mState, rate, blend_num);
    }
}

void DriverModel::setCommonAnm_Blend(f32 rate, int blend_num)
{
    setCommonAnm_Base(mState, rate, blend_num);
}

#line 1837
void DriverModel::Change_next_mState(StateEnum nes)
{
    JUT_ASSERT(nes >= mcDrive);
    if (m_e8 == 0 && m_504 == 0) {
        if (mState != next_mState_Hokan) {
            switch (nes) {
            case mcChangeAtoB:
            case mcChangeBtoA:
            case mcS096:
            case mcS097:
            case mcS098:
            case mcS099:
            case mcS100:
            case mcS101:
            case mcS102:
            case mcS103:
            case mcS121:
            case mcS122:
                break;
            default:
                return;
            }
        }
        next_mState_Hokan = nes;
    }
}

void DriverModel::updateFrame()
{
    setCommonAnm2(150, m_668.mCurrentValue);
    m_668.update();
    setCommonAnm2(151, m_688.mCurrentValue);
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

static inline void restartOneShot(OneShotAnimator2 *anm)
{
    anm->mMinValue = 0.25f;
    anm->mMaxValue = 1.0f;
    anm->mRate = 0.083333336f;
    anm->mTime = (anm->mMaxValue - anm->mMinValue) / anm->mRate;
    anm->mFrame = 0;
    anm->mMaxFrame = (s32)(60.0f * anm->mRate) << 4;
    anm->mStepFrame = 0x10;
    anm->mIsFinished = 1;
    anm->reset();
    anm->start();
    anm->update();
}

#line 1917
void DriverModel::updateFrame_base(DriversAnimator *animator, StateEnum state)
{
    JUT_MINMAX_ASSERT(0, state, 152);
    int isLoop = 0;
    if ((u8)CheckDriverFlags_Loop(state)) {
        setCommonAnm2(state, animator->mLoopAnim.mCurrentValue);
        if (animator == &mDriveAnim) {
            if (IsGoalFront() == 2 || IsHappyFront() == 2)
                Change_next_mState(mcS120);
            if (IsGoalBack() == 2 || IsHappyBack() == 2)
                Change_next_mState(mcS111);
        }
        isLoop = 1;
    }
    animator->mLoopAnim.update();
    if (isLoop)
        return;

    for (int blend_num = 0; blend_num < 2; blend_num++) {
        if (getAnmTransformPtr(msAnmBlendList[state][blend_num]) == nullptr)
            continue;
        if (IsTandem(state))
            continue;
        if (IsDrive(state))
            continue;
        switch (state) {
        case mcS070:
        case mcS073:
            continue;
        }

        setCommon1ShotAnm_Single2(state, animator->mOneShotAnm[blend_num].mCurrentValue, blend_num);
        if (animator->mOneShotAnm[blend_num].mIsFinished != 1) {
            if (animator == &mDriveAnim) {
                int blendTime = updateSetBlendWeight_GetBlendTime(state);
                mAnmID = msAnmBlendList[state][0];
#line 1976
                JUT_MINMAX_ASSERT(0, mAnmID, 82);
                switch (state) {
                case mcS106: {
                    int blend = DriverManager::ptr()->getBlendTime(27);
                    if (mAnmFrame[mAnmID] + blend > getAnmTransformPtr(mAnmID)->getFrameMax() / 2.0f)
                        Change_next_mState((StateEnum)msNext_mState_HokanDamages[state - 106]);
                    break;
                }
                case mcS107:
                    if ((s32)mAnmFrame[mAnmID] == 15 && mCharID != 19) {
                        mLumbarCtrl->setFlags();
                        mLumbarCtrl->m_74 = 0;
                    }
                    break;
                case mcS126: {
                    u8 blend = DriverManager::ptr()->getBlendTime(36);
                    if (mAnmFrame[mAnmID] > blend)
                        Change_next_mState((StateEnum)(state - 63));
                    break;
                }
                case mcS127: {
                    s16 frameMax = getAnmTransformPtr(mAnmID)->getFrameMax();
                    u8 blend = DriverManager::ptr()->getBlendTime(36);
                    if (mAnmFrame[mAnmID] + blend < frameMax)
                        Change_next_mState((StateEnum)(state - 63));
                    break;
                }
                case mcS117: {
                    int blend = DriverManager::ptr()->getBlendTime(29);
                    if (mAnmFrame[mAnmID] + blend > getAnmTransformPtr(mAnmID)->getFrameMax())
                        Change_next_mState(mcS111);
                    break;
                }
                case mcS122: {
                    int blend = DriverManager::ptr()->getBlendTime(17);
                    if (mAnmFrame[mAnmID] + blend > getAnmTransformPtr(mAnmID)->getFrameMax())
                        Change_next_mState(mcS110);
                    break;
                }
                case mcS096:
                case mcS098:
                case mcS100:
                case mcS102:
                    if (m_dc != 0) {
                        u8 blend = DriverManager::ptr()->_5d;
                        if (getAnmTransformPtr(mAnmID)->getFrame() > blend) {
                            Change_next_mState((StateEnum)m_dc);
                            m_dc = 0;
                        }
                    } else if (m_b8 != 0) {
                        u8 blend = DriverManager::ptr()->mBlendTimes[23];
                        f32 frame = getAnmTransformPtr(mAnmID)->getFrame();
                        if (frame + blend > getAnmTransformPtr(mAnmID)->getFrameMax()) {
                            if (IsItemWithTurn())
                                Change_next_mState(mcS112);
                            else
                                Change_next_mState(mcS111);
                        }
                    }
                    break;
                case mcS104: {
                    int blend = DriverManager::ptr()->getBlendTime(20);
                    if (mAnmFrame[mAnmID] + blend > getAnmTransformPtr(mAnmID)->getFrameMax()) {
                        if (KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x1000000000000ull)
                            Change_next_mState(mcS115);
                        else if (KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x800000000000ull)
                            Change_next_mState(mcS116);
                        else
                            Change_next_mState(mcS112);
                    }
                    break;
                }
                }
            }
        } else {
            if (animator == &mTandemAnim) {
                if (m_ac != 0)
                    continue;
            } else if (animator == &mAdditiveAnim) {
                switch (state) {
                case mcS126:
                case mcS127:
                    continue;
                }
            }

            switch (state) {
            case mcChangeAtoB:
            case mcS008:
            case mcS021:
            case mcS022:
            case mcS023:
            case mcS024:
            case mcS027:
            case mcS028:
            case mcS034:
            case mcS036:
            case mcS042:
            case mcS048:
            case mcS085:
            case mcS086:
            case mcS089:
            case mcS095:
            case mcS096:
            case mcS098:
            case mcS100:
            case mcS102:
            case mcS105:
            case mcS125:
            case mcS140:
            case mcS141:
            case mcS142:
            case mcS143:
                Change_next_mState(mcTandemB);
                break;
            case mcS146:
            case mcS148:
                if (KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x1000000000000ull)
                    Change_next_mState(mcS113);
                else if (KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x800000000000ull)
                    Change_next_mState(mcS114);
                else
                    Change_next_mState(mcS111);
                break;
            case mcS147:
                if (KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x1000000000000ull)
                    Change_next_mState(mcS115);
                else if (KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x800000000000ull)
                    Change_next_mState(mcS116);
                else
                    Change_next_mState(mcS112);
                break;
            case mcS039:
            case mcS045:
            case mcS117:
                Change_next_mState(mcS111);
                break;
            case mcS035:
            case mcS094:
                Change_next_mState(mcS069);
                break;
            case mcS014:
            case mcS016:
                if (animator->mOneShotAnm[blend_num].mMinValue == 0.0f) {
                    restartOneShot(&animator->mOneShotAnm[blend_num]);
                    break;
                }
            case mcS013:
            case mcS015:
                Change_next_mState(mcTandemB);
                break;
            case mcS051:
            case mcS054:
                if (animator != &mTandemAnim) {
                    if (animator == &mAdditiveAnim) {
                        m_500 = 3;
                        InitMpaMtxCalcThrow(getAnmTransformPtr(msAnmBlendList[m_500][0]));
                        mMtxCalcs[3]->setAnm(getAnmTransformPtr(msAnmBlendList[3][0]), getAnmTransformPtr(msAnmBlendList[3][1]), nullptr, nullptr);
                    } else {
                        Change_next_mState(mcTandemB);
                    }
                }
                break;
            case mcS007:
            case mcS025:
            case mcS026:
            case mcS029:
            case mcS087:
            case mcS088:
            case mcS090:
            case mcS144:
            case mcS145:
                Change_next_mState(mcTandemAlone);
                break;
            case mcS057:
            case mcS060:
                if (animator != &mTandemAnim) {
                    if (animator == &mAdditiveAnim) {
                        m_500 = 4;
                        InitMpaMtxCalcThrow(getAnmTransformPtr(msAnmBlendList[m_500][0]));
                        mMtxCalcs[4]->setAnm(getAnmTransformPtr(msAnmBlendList[4][0]), getAnmTransformPtr(msAnmBlendList[4][1]), nullptr, nullptr);
                    } else {
                        Change_next_mState(mcTandemAlone);
                    }
                }
                break;
            case mcS018:
            case mcS020:
                if (animator->mOneShotAnm[blend_num].mMinValue == 0.0f) {
                    restartOneShot(&animator->mOneShotAnm[blend_num]);
                    break;
                }
            case mcS017:
            case mcS019:
                Change_next_mState(mcTandemAlone);
                break;
            case mcS010:
            case mcS012:
                if (animator->mOneShotAnm[blend_num].mMinValue == 0.0f) {
                    restartOneShot(&animator->mOneShotAnm[blend_num]);
                    break;
                }
            case mcS009:
            case mcS011:
                Change_next_mState(mcDrive);
                break;
            case mcChangeBtoA:
            case mcS097:
            case mcS099:
            case mcS101:
            case mcS103:
            case mcS121:
                Change_next_mState(mcDrive);
                break;
            case mcS037:
            case mcS040:
            case mcS043:
            case mcS046:
            case mcS049:
            case mcS052:
            case mcS055:
            case mcS058:
                Change_next_mState((StateEnum)(state + 1));
                break;
            case mcS107:
                Change_next_mState((StateEnum)msNext_mState_HokanDamages[state - 106]);
                break;
            case mcS126:
            case mcS127:
                Change_next_mState((StateEnum)(state - 63));
                break;
            case mcS063:
                Change_next_mState(mcS051);
                break;
            case mcS064:
                Change_next_mState(mcS054);
                break;
            case mcS134:
                Change_next_mState(mcS135);
                break;
            case mcS136:
            case mcS137:
            case mcS138:
            case mcS139:
                Change_next_mState((StateEnum)((state - 136) * 3 + 50));
                break;
            }
        }
        animator->mOneShotAnm[blend_num].update();
    }

    switch (state) {
    case mcS053:
    case mcS059:
    case mcS114:
    case mcS116:
        setCommon1ShotAnm2(state, 0.0f);
        break;
    case mcS038:
    case mcS044:
        setCommon1ShotAnm2(state, 0.5f);
        break;
    case mcS041:
    case mcS047:
    case mcS050:
    case mcS056:
    case mcS113:
    case mcS115:
        setCommon1ShotAnm2(state, 1.0f);
        break;
    case mcS042:
    case mcS048:
        setCommon1ShotAnm_Single2(state, 1.0f, 0);
        break;
    case mcS071:
    case mcS074:
        setCommon1ShotAnm_Single2(state, 1.0f, 0);
        break;
    }
}

#line 2379
void DriverModel::updateSetFrame_base(StateEnum state)
{
    JUT_MINMAX_ASSERT(0, state, 152);
    for (int blend_num = 0; blend_num < 2; blend_num++) {
        mAnmID = msAnmBlendList[state][blend_num];
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
    }
}

void DriverModel::updateSetFrame()
{
    updateSetFrame_base((StateEnum)mState);
    if (m_ac != 0)
        updateSetFrame_base((StateEnum)mBeforeState);

    switch (mState) {
    case mcS013:
    case mcS014:
    case mcS015:
    case mcS016:
    case mcS063:
    case mcS064:
        updateSetFrame_base((StateEnum)m_500);
        break;
    case mcS017:
    case mcS018:
    case mcS019:
    case mcS020:
        updateSetFrame_base((StateEnum)m_500);
        break;
    }

    if (mItem != nullptr && m_b8 != 0 && mItem->getSuccessionItemNum() > 1)
        updateSetFrame_base((StateEnum)150);
    if (m_b9 != 0)
        updateSetFrame_base((StateEnum)151);
}

void DriverModel::updateLoadCommand(int anmID)
{
    if (!IsAnmResInMRAM(anmID))
        mAnimBufMgr->CheckAndLoadAnimAvailable(anmID, mDataID);
}

void DriverModel::updateLoadCommand_Sound(int sndID)
{
    if (!IsAnmSoundResInMRAM(sndID))
        mAnimBufMgr->CheckAndLoadAnimSoundAvailable(sndID, mDataID);
}

void DriverModel::updateLoad_Animation()
{
#line 2465
    JUT_ASSERT_F(0 <= next_mState_Hokan && next_mState_Hokan < 152, "range over: %d <= next_mState=%d < %d", 0, next_mState_Hokan, 152);
    mAnimBufMgr->resetFrame();

    if ((u8)CheckDriverFlags_Hokan((StateEnum)next_mState_Hokan)) {
#line 2471
        JUT_ASSERT_F(0 <= next_before_mState_Hokan && next_before_mState_Hokan < 152, "range over: %d <= next_before_mState_Hokan=%d < %d", 0, next_before_mState_Hokan, 152);
        updateLoadCommand(msAnmBlendList[next_before_mState_Hokan][0]);
        updateLoadCommand(msAnmBlendList[next_before_mState_Hokan][1]);
        updateLoadCommand(msAnmBlendList[next_mState_Hokan][0]);
        updateLoadCommand(msAnmBlendList[next_mState_Hokan][1]);
        return;
    }

    switch (next_mState_Hokan) {
    case mcS013:
    case mcS014:
    case mcS015:
    case mcS016:
        updateLoadCommand(msAnmBlendList[next_mState_Hokan][0]);
        switch (mState) {
        case mcS049:
        case mcS050:
        case mcS051:
        case mcS052:
        case mcS053:
        case mcS054:
        case mcS126:
        case mcS127:
            m_500 = *(volatile s32 *)&mState;
            break;
        default:
            m_500 = 3;
            break;
        }
        updateLoadCommand(msAnmBlendList[m_500][0]);
        updateLoadCommand(msAnmBlendList[m_500][1]);
        break;
    case mcS017:
    case mcS018:
    case mcS019:
    case mcS020:
        updateLoadCommand(msAnmBlendList[next_mState_Hokan][0]);
        switch (mState) {
        case mcS055:
        case mcS056:
        case mcS057:
        case mcS058:
        case mcS059:
        case mcS060:
            m_500 = mState;
            break;
        default:
            m_500 = 4;
            break;
        }
        updateLoadCommand(msAnmBlendList[m_500][0]);
        break;
    case mcS009:
    case mcS010:
    case mcS011:
    case mcS012:
        updateLoadCommand(msAnmBlendList[next_mState_Hokan][0]);
        m_500 = 0;
        updateLoadCommand(msAnmBlendList[m_500][0]);
        break;
    case mcS063:
    case mcS064:
        updateLoadCommand(msAnmBlendList[next_mState_Hokan][0]);
        m_500 = next_mState_Hokan + 63;
        updateLoadCommand(msAnmBlendList[m_500][0]);
        updateLoadCommand(msAnmBlendList[m_500][1]);
        break;
    default:
        updateLoadCommand(msAnmBlendList[next_mState_Hokan][0]);
        updateLoadCommand(msAnmBlendList[next_mState_Hokan][1]);
        break;
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
    anmID = DriverLoader::mscAnm2SoundList[anmID];
    if (!IsAnmSoundResInMRAM(anmID))
        mAnimBufMgr->CheckAndLoadAnimSoundAvailable(anmID, mDataID);
    if (mAnimBufMgr->mRes0 == 0)
        m_cc = 1;
    if (mAnimBufMgr->mRes4 == 0)
        m_d0 = 1;
}

#pragma inline_depth(2)
int DriverModel::updateCheckRenew_Animation()
{
    int result = mAnimBufMgr->CheckRenew();
    if (result == -1)
        return 0;
    if (result == 1 || m_cc != 0) {
        s32 before_mState_Hokan = mBeforeState;
#line 2598
        JUT_MINMAX_ASSERT(0, before_mState_Hokan, 152);
        JUT_MINMAX_ASSERT(0, mState, 152);
        s32 mStateThrow = m_500;
#line 2600
        JUT_MINMAX_ASSERT(0, mStateThrow, 152);
        if (m_ac != 0) {
            InitMpaMtxCalc((StateEnum)mState, getAnmTransformPtr(msAnmBlendList[mBeforeState][0]),
                           getAnmTransformPtr(msAnmBlendList[mBeforeState][1]),
                           getAnmTransformPtr(msAnmBlendList[mState][0]),
                           getAnmTransformPtr(msAnmBlendList[mState][1]));
            InitMpaMtxCalc((StateEnum)150, getAnmTransformPtr(msAnmBlendList[150][0]),
                           getAnmTransformPtr(msAnmBlendList[150][1]), nullptr, nullptr);
            if (getAnmTransformPtr(msAnmBlendList[151][0]) != nullptr) {
                InitMpaMtxCalc((StateEnum)151, getAnmTransformPtr(msAnmBlendList[151][0]),
                               getAnmTransformPtr(msAnmBlendList[151][1]), nullptr, nullptr);
            }
        } else {
            switch (mState) {
            case mcS013:
            case mcS014:
            case mcS015:
            case mcS016:
            case mcS063:
            case mcS064:
                InitMpaMtxCalcThrow(getAnmTransformPtr(msAnmBlendList[m_500][0]));
                InitMpaMtxCalc(mcTandemB, getAnmTransformPtr(msAnmBlendList[m_500][0]),
                               getAnmTransformPtr(msAnmBlendList[m_500][1]), nullptr, nullptr);
                InitMpaMtxCalc((StateEnum)mState, getAnmTransformPtr(msAnmBlendList[mState][0]),
                               getAnmTransformPtr(msAnmBlendList[mState][1]), nullptr, nullptr);
                break;
            case mcS009:
            case mcS010:
            case mcS011:
            case mcS012:
            case mcS017:
            case mcS018:
            case mcS019:
            case mcS020:
                InitMpaMtxCalcThrow(getAnmTransformPtr(msAnmBlendList[m_500][0]));
                InitMpaMtxCalc((StateEnum)m_500, getAnmTransformPtr(msAnmBlendList[m_500][0]),
                               getAnmTransformPtr(msAnmBlendList[m_500][1]), nullptr, nullptr);
                InitMpaMtxCalc((StateEnum)mState, getAnmTransformPtr(msAnmBlendList[mState][0]),
                               getAnmTransformPtr(msAnmBlendList[mState][1]), nullptr, nullptr);
                break;
            default:
                InitMpaMtxCalc((StateEnum)mState, getAnmTransformPtr(msAnmBlendList[mState][0]),
                               getAnmTransformPtr(msAnmBlendList[mState][1]), nullptr, nullptr);
                InitMpaMtxCalc((StateEnum)150, getAnmTransformPtr(msAnmBlendList[150][0]),
                               getAnmTransformPtr(msAnmBlendList[150][1]), nullptr, nullptr);
                if (getAnmTransformPtr(msAnmBlendList[151][0]) != nullptr) {
                    InitMpaMtxCalc((StateEnum)151, getAnmTransformPtr(msAnmBlendList[151][0]),
                                   getAnmTransformPtr(msAnmBlendList[151][1]), nullptr, nullptr);
                }
                break;
            }
        }
    }
    m_cc = 0;
    return 1;
}


#pragma inline_depth(4)
int DriverModel::updateCheckRenew_Sound()
{
    int result = mAnimBufMgr->CheckRenewSound();
    if (result == -1)
        return 0;
    if (result == 1 || m_d0 != 0) {
        u8 anmID = (u8)msAnmBlendList[next_mState_Hokan][0];
        DriverLoader::checkAnmID(anmID);
        void *sound = mAnimBufMgr->GetSoundPtr(DriverLoader::mscAnm2SoundList[anmID]);
        GameAudio::CharacterSoundMgr *mgr = mCharSoundMgr;
        mgr->mAnimator->startAnimation((const JAUSoundAnimation *)sound, false, 0.0f, 0.0f);
        *((u8 *)mgr->mStarter + 4) = mgr->getKartSound()->mPlayerIndex;
    }
    m_d0 = 0;
    return 1;
}

int DriverModel::updateCheckRenew()
{
    int result = updateCheckRenew_Animation();
    if (result != 0)
        result = updateCheckRenew_Sound();
    if (result != 0)
        return 1;
#line 2684
    JUT_ASSERT_F(0, "%s", "ARAM DMA not finished?(kawano).");
    return 0;
}
f64 DriverModel::BlendValue(f32 x)
{
    f32 y;
    if (x < 0.5f) {
        y = pow(2.0f * x, DriverManager::ptr()->_20);
        return y *= 0.5f;
    } else {
        y = pow(1.0f - 2.0f * (x - 0.5f), DriverManager::ptr()->_20);
        y = 1.0f - y;
        return (y *= 0.5f) + 0.5f;
    }
}

#line 2711
void DriverModel::updateBlendCount_Func1(u8 a, u8 b)
{
    m_bb++;
    JUT_MINMAX_ASSERT(0, mState, 152);
    mAnmID = msAnmBlendList[mState][0];
    if (mAnmID != 0 && getAnmTransformPtr(mAnmID)) {
        s16 frameMax = getAnmTransformPtr(mAnmID)->getFrameMax();
        s16 frame = getAnmTransformPtr(mAnmID)->getFrame() + 0.5f;
        s16 t0 = DriverManager::ptr()->mBlendTimes[a];
        s16 t1 = DriverManager::ptr()->getBlendTime(b);
        if (frameMax - frame <= t1)
            m_bb = frame - (frameMax - t1);
        else if (frame > t0)
            m_bb = 0;
    }
}

void DriverModel::updateBlendCount_Func2(u8 blendTime)
{
    if (m_bb < blendTime)
        m_bb++;
    else
        m_bb = blendTime;
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

    int blendTime = updateSetBlendWeight_GetBlendTime((StateEnum)mState);

    switch (mState) {
    case mcS042:
    case mcS048:
    case mcS069:
    case mcS072:
    case mcS091:
    case mcS092:
    case mcS093:
    case mcS094:
    case mcS095:
    case mcS097:
    case mcS099:
    case mcS101:
    case mcS103:
    case mcS104:
    case mcS106:
    case mcS107:
    case mcS108:
    case mcS109:
    case mcS110:
    case mcS117:
    case mcS118:
    case mcS119:
    case mcS121:
    case mcS122:
    case mcS123:
    case mcS124:
    case mcS125:
    case mcS126:
    case mcS127:
    case mcS128:
    case mcS129:
    case mcS130:
    case mcS131:
    case mcS132:
    case mcS133:
    case mcS134:
    case mcS135:
    case mcS136:
    case mcS137:
    case mcS138:
    case mcS139:
    case mcS146:
    case mcS147:
    case mcS148:
    case mcS149:
        if (m_bb < blendTime)
            m_bb++;
        else
            m_bb = blendTime;
        break;
    case mcS120:
        if (m_bb < blendTime) {
            m_bb++;
            if (m_bb == blendTime)
                Change_next_mState(mcDrive);
        }
        break;
    case mcS070:
    case mcS071:
    case mcS073:
    case mcS074:
        if (m_bb < blendTime) {
            m_bb++;
            if (m_bb == blendTime) {
                switch (mState) {
                case mcS070:
                    Change_next_mState(mcTandemB);
                    break;
                case mcS073:
                    Change_next_mState(mcTandemAlone);
                    break;
                case mcS071:
                    Change_next_mState(mcS111);
                    break;
                case mcS074:
                    Change_next_mState(mcS112);
                    break;
                }
            }
        } else {
            m_bb = blendTime;
        }
        break;
    case mcS075:
    case mcS076:
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
                switch (mState) {
                case mcS075:
                    Change_next_mState(mcTandemB);
                    break;
                case mcS076:
                    Change_next_mState(mcTandemAlone);
                    break;
                }
            }
        }
        break;
    case mcS077:
    case mcS078:
    case mcS079:
    case mcS080:
        if (m_bb < blendTime) {
            m_bb++;
            if (m_bb == blendTime)
                Change_next_mState((StateEnum)(mState - 76));
        }
        break;
    case mcS081:
    case mcS082:
    case mcS083:
    case mcS084:
        if (m_bb < blendTime) {
            m_bb++;
            if (m_bb >= blendTime) {
                switch (mState) {
                case mcS081:
                case mcS082:
                    Change_next_mState(mcTandemB);
                    break;
                default:
                    Change_next_mState(mcTandemAlone);
                    break;
                }
            }
        }
        break;
    case mcS085:
    case mcS086:
    case mcS087:
    case mcS088:
    case mcS140:
    case mcS141:
    case mcS142:
    case mcS143:
    case mcS144:
    case mcS145:
        updateBlendCount_Func1(8, 9);
        break;
    case mcS089:
    case mcS090:
        updateBlendCount_Func1(10, 11);
        break;
    case mcS096:
    case mcS098:
    case mcS100:
    case mcS102:
        updateBlendCount_Func1(17, 18);
        break;
    case mcS105:
        updateBlendCount_Func1(21, 22);
        break;
    case mcS111:
    case mcS112:
        if (m_bb < blendTime) {
            m_bb++;
            if (m_bb == blendTime)
                Change_next_mState((StateEnum)(mState - 108));
        }
        break;
    case mcS113:
    case mcS114:
    case mcS115:
    case mcS116:
        if (m_bb < blendTime) {
            m_bb++;
            if (m_bb == blendTime)
                Change_next_mState((StateEnum)((mState - 113) * 3 + 50));
        }
        break;
    }
}

#line 2978
f32 DriverModel::updateSetBlendWeight_Func1(u8 a, u8 b)
{
    f32 w = 0.0f;
    JUT_MINMAX_ASSERT(0, mState, 152);
    mAnmID = msAnmBlendList[mState][0];
    if (mAnmID != 0 && getAnmTransformPtr(mAnmID)) {
        s16 frameMax = getAnmTransformPtr(mAnmID)->getFrameMax();
        s16 frame = getAnmTransformPtr(mAnmID)->getFrame() + 0.5f;
        if (frameMax - frame < b) {
            if (b < m_bb)
                m_bb = b;
            w = 1.0f - (f32)m_bb / (f32)b;
        } else if (frame < a) {
            if (a < m_bb)
                m_bb = a;
            w = (f32)m_bb / (f32)a;
        } else {
            w = 1.0f;
        }
    }
    return w;
}

#line 3019
void DriverModel::updateSetBlendWeight_FuncABCD(f32 before, f32 cur, f32 rateInv, f32 rate)
{
    s32 state = mState;
#line 3021
    JUT_ASSERT_F(0 <= state && state < 152, "range over: %d <= mState=%d < %d", 0, state, 152);
    s32 before_mState_Hokan = mBeforeState;
#line 3022
    JUT_MINMAX_ASSERT(0, before_mState_Hokan, 152);
    if (msAnmBlendList[mBeforeState][1] != 0) {
        m_754[0] = rateInv * before;
        m_754[1] = rate * before;
    } else {
        m_754[0] = before;
        m_754[1] = 0.0f;
    }
    if (msAnmBlendList[mState][1] != 0) {
        m_754[2] = rateInv * cur;
        m_754[3] = rate * cur;
    } else {
        m_754[2] = cur;
        m_754[3] = 0.0f;
    }
    if (1.0f != m_754[0] + m_754[1] + m_754[2] + m_754[3])
        m_754[0] = m_754[0];
    mMtxCalcs[mState]->setWeight(0, m_754[0]);
    mMtxCalcs[mState]->setWeight(1, m_754[1]);
    mMtxCalcs[mState]->setWeight(2, m_754[2]);
    mMtxCalcs[mState]->setWeight(3, m_754[3]);
}

u8 DriverModel::updateSetBlendWeight_GetBlendTime(StateEnum stt)
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
        if (m_e0 == 0)
            return DriverManager::ptr()->getBlendTime(3);
        else
            return DriverManager::ptr()->getBlendTime(4);
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
        switch (mBeforeState) {
        case 117:
            return DriverManager::ptr()->getBlendTime(29);
        case 108: case 109:
            return DriverManager::ptr()->getBlendTime(26);
        case 71: case 74: case 91: case 92:
            return DriverManager::ptr()->getBlendTime(13);
        case 123: case 124:
            return DriverManager::ptr()->getBlendTime(33);
        default:
            return DriverManager::ptr()->getBlendTime(26);
        }
    case 112:
        return DriverManager::ptr()->getBlendTime(26);
    case 113: case 114: case 115: case 116:
        switch (mBeforeState) {
        case 123: case 124:
            return DriverManager::ptr()->getBlendTime(33);
        default:
            return DriverManager::ptr()->getBlendTime(26);
        }
    case 118: case 119:
        return DriverManager::ptr()->getBlendTime(30);
    case 120:
        return DriverManager::ptr()->getBlendTime(31);
    case 123: case 124:
        return DriverManager::ptr()->getBlendTime(32);
    case 125:
        return DriverManager::ptr()->getBlendTime(34);
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


#line 3203
void DriverModel::updateSetBlendWeight_mStateThrow()
{
    s32 mStateThrow = m_500;
    JUT_MINMAX_ASSERT(0, mStateThrow, 152);
    if (mMtxCalcs[m_500] != nullptr) {
        int blendTime = DriverManager::ptr()->mBlendTimes[0];
        mMtxCalcs[m_500]->setWeight(0, 1.0f - (f32)m_ba / blendTime);
        mMtxCalcs[m_500]->setWeight(1, (f32)m_ba / blendTime);
        if ((int)DriverData::mspDriverData->m_1a != 0) {
            mMtxCalcs[m_500]->setWeight(0, 1.0f);
            mMtxCalcs[m_500]->setWeight(1, 0.0f);
        }
    }
}

static inline void setBlendWeight4(DriverModel *d, f32 a, f32 b, f32 c, f32 e)
{
    d->m_754[0] = a;
    d->m_754[1] = b;
    d->m_754[2] = c;
    d->m_754[3] = e;
    if (1.0f != d->m_754[0] + d->m_754[1] + d->m_754[2] + d->m_754[3])
        d->m_754[0] = d->m_754[0];
    d->mMtxCalcs[d->mState]->setWeight(0, a);
    d->mMtxCalcs[d->mState]->setWeight(1, b);
    d->mMtxCalcs[d->mState]->setWeight(2, c);
    d->mMtxCalcs[d->mState]->setWeight(3, e);
}

#line 3232
void DriverModel::updateSetBlendWeight()
{
    JUT_MINMAX_ASSERT(0, mState, 152);
    if (mMtxCalcs[mState] == nullptr)
        return;

    s32 before_mState_Hokan = mBeforeState;
#line 3240
    JUT_MINMAX_ASSERT(0, before_mState_Hokan, 152);
    DriverManager *mgr = DriverManager::ptr();
    f32 rate = (f32)m_ba / (f32)mgr->mBlendTimes[0];
    f32 rateInv = 1.0f - rate;
    int blendTime = updateSetBlendWeight_GetBlendTime((StateEnum)mState);

    switch (mState) {
    case mcDrive:
    case mcDriveBack:
    case mcTandemB:
    case mcS037:
    case mcS038:
    case mcS039:
    case mcS040:
    case mcS041:
    case mcS042:
    case mcS048:
    case mcS049:
    case mcS050:
    case mcS051:
    case mcS052:
    case mcS053:
    case mcS054:
        m_754[0] = 1.0f - (f32)m_ba / blendTime;
        m_754[1] = m_ba / blendTime;
        m_754[2] = 0.0f;
        m_754[3] = 0.0f;
        mMtxCalcs[mState]->setWeight(0, 1.0f - (f32)m_ba / blendTime);
        mMtxCalcs[mState]->setWeight(1, (f32)m_ba / blendTime);
        break;
    case mcDriveC:
    case mcTandemAlone:
    case mcS008:
    case mcS009:
    case mcS010:
    case mcS011:
    case mcS012:
    case mcS013:
    case mcS014:
    case mcS015:
    case mcS016:
    case mcS017:
    case mcS018:
    case mcS019:
    case mcS020:
    case mcS034:
    case mcS043:
    case mcS044:
    case mcS045:
    case mcS046:
    case mcS047:
    case mcS055:
    case mcS056:
    case mcS057:
    case mcS058:
    case mcS059:
    case mcS060:
    case mcS061:
    case mcS062:
    case mcS063:
    case mcS064:
        m_754[0] = 1.0f;
        m_754[1] = 0.0f;
        m_754[2] = 0.0f;
        m_754[3] = 0.0f;
        mMtxCalcs[mState]->setWeight(0, 1.0f);
        mMtxCalcs[mState]->setWeight(1, 0.0f);
        mMtxCalcs[mState]->setWeight(2, 0.0f);
        mMtxCalcs[mState]->setWeight(3, 0.0f);
        break;
    case mcS069:
    case mcS070:
    case mcS071:
    case mcS072:
    case mcS073:
    case mcS074: {
        f32 wInv;
        f32 w = BlendValue((f32)m_bb / blendTime);
        wInv = 1.0f - w;
        updateSetBlendWeight_FuncABCD(wInv, w, rateInv, rate);
        break;
    }
    case mcS075:
    case mcS076: {
        f32 t = (f32)m_bb / blendTime;
        f32 w = BlendValue(t);
        w *= DriverManager::ptr()->_28;
        f32 wInv = 1.0f - w;
        updateSetBlendWeight_FuncABCD(wInv, w, rateInv, rate);
        break;
    }
    case mcS077:
    case mcS078:
    case mcS079:
    case mcS080:
    case mcS081:
    case mcS082:
    case mcS083:
    case mcS084:
    case mcS091:
    case mcS092:
    case mcS104:
    case mcS106:
    case mcS107:
    case mcS110:
    case mcS111:
    case mcS112:
    case mcS113:
    case mcS114:
    case mcS115:
    case mcS116:
    case mcS117:
    case mcS120:
    case mcS126:
    case mcS127:
    case mcS136:
    case mcS137:
    case mcS138:
    case mcS139: {
        f32 w = (f32)m_bb / blendTime;
        f32 wInv = 1.0f - w;
        updateSetBlendWeight_FuncABCD(wInv, w, rateInv, rate);
        break;
    }
    case mcS093:
    case mcS094:
    case mcS095:
    case mcS108:
    case mcS109:
    case mcS118:
    case mcS119:
    case mcS121:
    case mcS122:
    case mcS123:
    case mcS124:
    case mcS125:
    case mcS128:
    case mcS129:
    case mcS130:
    case mcS131:
    case mcS132:
    case mcS133:
    case mcS134:
    case mcS135:
    case mcS146:
    case mcS147:
    case mcS148:
    case mcS149: {
        f32 w = (f32)m_bb / blendTime;
        setBlendWeight4(this, rateInv * (1.0f - w), rate * (1.0f - w), w, 0.0f);
        break;
    }
    case mcS140:
    case mcS141:
    case mcS142:
    case mcS143:
    case mcS144:
    case mcS145: {
        f32 w = updateSetBlendWeight_Func1(DriverManager::ptr()->getBlendTime(8), DriverManager::ptr()->getBlendTime(9));
        setBlendWeight4(this, rateInv * (1.0f - w), rate * (1.0f - w), w, 0.0f);
        mAnmID = msAnmBlendList[mState][0];
        if (mAnmID != 0 && getAnmTransformPtr(mAnmID)) {
            f32 frame = getAnmTransformPtr(mAnmID)->getFrame();
            if ((s16)(frame + 0.5f) == DriverManager::ptr()->getBlendTime(8)) {
                int st;
                switch (mState) {
                case mcS144:
                case mcS145:
                    st = 4;
                    break;
                case mcS140:
                case mcS141:
                case mcS142:
                case mcS143:
                default:
                    st = 3;
                    break;
                }
                mMtxCalcs[mState]->setAnmTransform(0, getAnmTransformPtr(msAnmBlendList[st][0]));
                mMtxCalcs[mState]->setAnmTransform(1, getAnmTransformPtr(msAnmBlendList[st][1]));
                mBeforeState = st;
                m_4f8 = st;
                next_before_mState_Hokan = st;
            }
        }
        break;
    }
    case mcS085:
    case mcS086:
    case mcS087:
    case mcS088:
    case mcS089:
    case mcS090: {
        f32 wInv;
        f32 w;
        switch (mState) {
        case mcS085:
        case mcS086:
        case mcS087:
        case mcS088:
        {
            w = updateSetBlendWeight_Func1(DriverManager::ptr()->getBlendTime(8), DriverManager::ptr()->getBlendTime(9));
            break;
        }
        case mcS089:
        case mcS090:
        {
            w = updateSetBlendWeight_Func1(DriverManager::ptr()->getBlendTime(10), DriverManager::ptr()->getBlendTime(11));
            break;
        }
        }
        wInv = 1.0f - w;
        updateSetBlendWeight_FuncABCD(wInv, w, rateInv, rate);
        break;
    }
    case mcS096:
    case mcS098:
    case mcS100:
    case mcS102: {
        f32 w = updateSetBlendWeight_Func1(DriverManager::ptr()->getBlendTime(17), DriverManager::ptr()->getBlendTime(18));
        mAnmID = msAnmBlendList[mState][0];
        if (mAnmID != 0 && getAnmTransformPtr(mAnmID)) {
            f32 frame = getAnmTransformPtr(mAnmID)->getFrame();
            if ((s16)(frame + 0.5f) == DriverManager::ptr()->getBlendTime(17)) {
                int st = 3;
                if (m_b8 != 0) {
                    if (IsItemWithTurn())
                        st = 4;
                    else
                        st = 3;
                }
                mMtxCalcs[mState]->setAnmTransform(0, getAnmTransformPtr(msAnmBlendList[st][0]));
                mMtxCalcs[mState]->setAnmTransform(1, getAnmTransformPtr(msAnmBlendList[st][1]));
                mBeforeState = st;
                m_4f8 = st;
                next_before_mState_Hokan = st;
            }
        }
        f32 wInv = 1.0f - w;
        updateSetBlendWeight_FuncABCD(wInv, w, rateInv, rate);
        break;
    }
    case mcS105: {
        f32 w = updateSetBlendWeight_Func1(DriverManager::ptr()->getBlendTime(21), DriverManager::ptr()->getBlendTime(22));
        mAnmID = msAnmBlendList[mState][0];
        if (mAnmID != 0 && getAnmTransformPtr(mAnmID)) {
            f32 frame = getAnmTransformPtr(mAnmID)->getFrame();
            if ((s16)(frame + 0.5f) == DriverManager::ptr()->getBlendTime(21)) {
                StateEnum st = mcTandemB;
                mMtxCalcs[mState]->setAnmTransform(0, getAnmTransformPtr(msAnmBlendList[st][0]));
                mMtxCalcs[mState]->setAnmTransform(1, getAnmTransformPtr(msAnmBlendList[st][1]));
                mBeforeState = mcTandemB;
                m_4f8 = mcTandemB;
                next_before_mState_Hokan = mcTandemB;
            }
        }
        f32 wInv = 1.0f - w;
        updateSetBlendWeight_FuncABCD(wInv, w, rateInv, rate);
        break;
    }
    case mcS097:
    case mcS099:
    case mcS101:
    case mcS103: {
        f32 w = (f32)m_bb / blendTime;
        f32 wInv = 1.0f - w;
        updateSetBlendWeight_FuncABCD(wInv, w, rateInv, rate);
        mAnmID = msAnmBlendList[mState][0];
        if (mAnmID != 0 && getAnmTransformPtr(mAnmID)) {
            f32 frame = getAnmTransformPtr(mAnmID)->getFrame();
            if ((s16)(frame + 0.5f) > DriverManager::ptr()->getBlendTime(18))
                m_ba = 0;
        }
        break;
    }
    default:
        mMtxCalcs[mState]->setWeight(0, rateInv);
        mMtxCalcs[mState]->setWeight(1, rate);
        mMtxCalcs[mState]->setWeight(2, 0.0f);
        mMtxCalcs[mState]->setWeight(3, 0.0f);
        break;
    }

    if ((int)DriverData::mspDriverData->m_1a != 0) {
        mMtxCalcs[mState]->setWeight(0, 1.0f);
        mMtxCalcs[mState]->setWeight(1, 0.0f);
        mMtxCalcs[mState]->setWeight(2, 0.0f);
        mMtxCalcs[mState]->setWeight(3, 0.0f);
    }
}


void DriverModel::update_mIsSpeakBCA()
{
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
}

static inline void setCtrlJointMtxCalc(DriverModel *d, int ctrlNo, J3DMtxCalc *calc)
{
    J3DJoint *joint = nullptr;
    u16 jnt = d->mDriverLoader->getJntNo(ctrlNo);
    if (jnt != 0xffff)
        joint = d->getModelData()->mJointTree.mJoints[jnt];
    joint->mMtxCalc = calc;
}

static inline void setCtrlJointMtxCalcSafe(DriverModel *d, int ctrlNo, J3DMtxCalc *calc)
{
    J3DJoint *joint = nullptr;
    u16 jnt = d->mDriverLoader->getJntNo(ctrlNo);
    if (jnt != 0xffff)
        joint = d->getModelData()->mJointTree.mJoints[jnt];
    if (joint != nullptr)
        joint->mMtxCalc = calc;
}

#pragma inline_depth(2)
void DriverModel::update(u16 param)
{
    for (int i = 0; i < 8; i++) {
        mAnmWorkFrame[i] = -1.0f;
        mAnmWorkID[i] = -1;
    }
    mAnmID_debug_index = 0;

#line 3678
    JUT_ASSERT_F(mState >= mcDrive, "%s", "mState >= mcDrive");

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
            if (m_be <= 0) {
                m_be = 1;
            } else {
                if (m_be < DriverManager::ptr()->getDangerTime())
                    m_be++;
            }
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

        if ((KartCtrl::getKartCtrl()->GetCarStatus(mKartLoader->mKartNum) & 0x200000000ull) != 0)
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
        setCtrlJointMtxCalc(this, 1, nullptr);
        setCtrlJointMtxCalc(this, 10, nullptr);
        setCtrlJointMtxCalc(this, 14, nullptr);
        setCtrlJointMtxCalcSafe(this, 18, nullptr);
        goto L_after;
    }

    updateSetBlendWeight();
    update_mIsSpeakBCA();

#line 3875
    JUT_MINMAX_ASSERT(0, mState, 152);
    s32 mStateThrow = m_500;
#line 3876
    JUT_MINMAX_ASSERT(0, mStateThrow, 152);

    switch (mState) {
    case mcS009:
    case mcS010:
    case mcS011:
    case mcS012: {
        getModelData()->getJointNodePointer(0)->mMtxCalc = mMtxCalcs[0];
        setCtrlJointMtxCalc(this, 1, (J3DMtxCalc *)mMtxCalcsThrow[0]);
        setCtrlJointMtxCalc(this, 10, mMtxCalcs[mState]);
        setCtrlJointMtxCalc(this, 14, mMtxCalcs[mState]);
        setCtrlJointMtxCalcSafe(this, 18, mMtxCalcs[mState]);
        break;
    }
    case mcS013:
    case mcS014:
    case mcS015:
    case mcS016: {
        updateSetBlendWeight_mStateThrow();
        getModelData()->getJointNodePointer(0)->mMtxCalc = mMtxCalcs[m_500];
        setCtrlJointMtxCalc(this, 1, (J3DMtxCalc *)mMtxCalcsThrow[0]);
        setCtrlJointMtxCalc(this, 10, mMtxCalcs[mState]);
        setCtrlJointMtxCalc(this, 14, mMtxCalcs[mState]);
        setCtrlJointMtxCalcSafe(this, 18, mMtxCalcs[mState]);
        break;
    }
    case mcS063:
    case mcS064: {
        getModelData()->getJointNodePointer(0)->mMtxCalc = mMtxCalcs[m_500];
        setCtrlJointMtxCalc(this, 1, (J3DMtxCalc *)mMtxCalcsThrow[0]);
        setCtrlJointMtxCalc(this, 10, mMtxCalcs[mState]);
        setCtrlJointMtxCalc(this, 14, mMtxCalcs[mState]);
        setCtrlJointMtxCalcSafe(this, 18, mMtxCalcs[mState]);
        break;
    }
    case mcS017:
    case mcS018:
    case mcS019:
    case mcS020: {
        getModelData()->getJointNodePointer(0)->mMtxCalc = mMtxCalcs[m_500];
        setCtrlJointMtxCalc(this, 1, (J3DMtxCalc *)mMtxCalcsThrow[0]);
        setCtrlJointMtxCalc(this, 10, mMtxCalcs[mState]);
        setCtrlJointMtxCalc(this, 14, mMtxCalcs[mState]);
        setCtrlJointMtxCalcSafe(this, 18, mMtxCalcs[mState]);
        break;
    }
    default: {
        getModelData()->getJointNodePointer(0)->mMtxCalc = mMtxCalcs[mState];
        setCtrlJointMtxCalc(this, 1, nullptr);
        setCtrlJointMtxCalc(this, 10, nullptr);
        int set = 0;
        if (mItem != nullptr && m_b8 != 0 && mItem->getSuccessionItemNum() > 1 &&
            *(s32 *)((u8 *)mItem + 0x2d4) == 0) {
            setCtrlJointMtxCalc(this, 14, mMtxCalcs[150]);
            set = 1;
        }
        if (!set)
            setCtrlJointMtxCalc(this, 14, nullptr);
        setCtrlJointMtxCalcSafe(this, 18, nullptr);
        break;
    }
    }

    if (m_b9 != 0) {
        setCtrlJointMtxCalcSafe(this, 18, mMtxCalcs[151]);
    }

L_after:
    if (!PauseManager::tstPause())
        SetDriverOffsetPosition();

    ExModel::update(param);
    LaurelMgr::ptr()->update(this);

    MtxPtr baseMtx = getModel()->getBaseTRMtx();
    mPos.x = baseMtx[0][3];
    mPos.y = baseMtx[1][3];
    mPos.z = baseMtx[2][3];

    mAnmID = msAnmBlendList[mState][0];
    mCharSoundMgr->frameWork(mAnmFrame[mAnmID], 1.0f);
    mAnimBufMgr->Check();

    if (mState != next_mState_Hokan) {
        s32 nes = next_mState_Hokan;
        switch (nes) {
        case mcS051:
        case mcS054:
        case mcS063:
        case mcS064:
        case mcS069:
        case mcS106:
        case mcS107:
        case mcS110:
        case mcS111:
        case mcS112:
        case mcS113:
        case mcS114:
        case mcS115:
        case mcS116:
        case mcS120:
        case mcS135:
            StartAnimation((StateEnum)*(volatile s32 *)&next_mState_Hokan);
            m_504 = 1;
            break;
        }

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
        JUT_ASSERT_F(0 <= next_mState_Hokan && next_mState_Hokan < 152, "range over: %d <= next_mState=%d < %d", 0, next_mState_Hokan, 152);
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
    JUT_ASSERT_F(mState >= mcDrive, "%s", "mState >= mcDrive");

    mState = next_mState_Hokan;
    mBeforeState = next_before_mState_Hokan;
    m_4f4 = mState;
    m_4f8 = mBeforeState;
    sCurDriver = nullptr;

    if ((mLumbarCtrl->m_70 & 4) != 0) {
        if (!IsDamageL() && !IsThrow() && !IsDash()) {
            mLumbarCtrl->clearFlags();
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

        switch (mCharID) {
        case 18:
            OsageMgr::ptr()->createModel(this, heap, a, b);
            break;
        case 19:
            break;
        default:
            goto L_create_done;
        }
    L_create_done:;
    }

    LaurelMgr::ptr()->createModel(this, heap, a, b);
    mTPAnim->setup(this);
    return true;
}

void DriverModel::setCurrentViewNo(u32 viewNo)
{
    ExModel::setCurrentViewNo(viewNo);

    switch (mCharID) {
    case 18:
        OsageMgr::ptr()->setCurrentViewNo(this, viewNo);
        break;
    case 19:
        break;
    default:
        goto L_view_done;
    }
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
