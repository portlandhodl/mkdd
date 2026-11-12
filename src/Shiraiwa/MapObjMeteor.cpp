#include "Sato/GeographyObj.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/StateObserver.h"
#include "Sato/stMath.h"
#include "Shiraiwa/SiUtil.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Kaneshige/RaceMgr.h"
#include "JSystem/JMath/JMath.h"

class TMapObjMeteor : public TMapObjHioNode, public StateObserver
{
public:
    TMapObjMeteor(const CrsData::SObject &);
    virtual ~TMapObjMeteor();

    virtual void reset();
    virtual const char *getBmdFileName() { return "/Objects/GeoStar.bmd"; }
    virtual void createColModel(J3DModelData *);
    virtual void calc();

    virtual void MoveExec();
    virtual void InitExec();

    void initFunc_Hide();
    void doFunc_Hide();
    void initFunc_Appear();
    void doFunc_Appear();
    void initFunc_Move();
    void doFunc_Move();
    void moveMainProc();
    void initFunc_Crush();
    void doFunc_Crush();
    bool checkGroundHitting();
    void showItem();

    static const StateObserver::StateFuncSet<TMapObjMeteor> sTable[4];

    static f32 sBoxSize;
    static f32 sObjRadius;
    static s16 sAppearFrame;
    static f32 sRotSpeed;
    static JGeometry::TVec3f sItemVel;

private:
    JGeometry::TVec3f mAxisX;      // 0x158
    JGeometry::TVec3f mAxisY;      // 0x164
    JGeometry::TVec3f mAxisZ;      // 0x170
    JGeometry::TVec3f mItemVel;    // 0x17C
    f32 mLife;                     // 0x188
    JPABaseEmitter *mEmitter;      // 0x18C
}; // Size: 0x190

f32 TMapObjMeteor::sBoxSize = 100.0f;
f32 TMapObjMeteor::sObjRadius = 300.0f;
s16 TMapObjMeteor::sAppearFrame = 5;
f32 TMapObjMeteor::sRotSpeed = 0.1f;

const StateObserver::StateFuncSet<TMapObjMeteor> TMapObjMeteor::sTable[4] = {
    { 0, &TMapObjMeteor::initFunc_Hide, &TMapObjMeteor::doFunc_Hide },
    { 1, &TMapObjMeteor::initFunc_Appear, &TMapObjMeteor::doFunc_Appear },
    { 2, &TMapObjMeteor::initFunc_Move, &TMapObjMeteor::doFunc_Move },
    { 3, &TMapObjMeteor::initFunc_Crush, &TMapObjMeteor::doFunc_Crush },
};

JGeometry::TVec3f TMapObjMeteor::sItemVel(0.0f, 50.0f, 0.0f);

TMapObjMeteor::TMapObjMeteor(const CrsData::SObject &object) : TMapObjHioNode(object), StateObserver()
{
    createSoundMgr();
}

TMapObjMeteor::~TMapObjMeteor() {}

void TMapObjMeteor::createColModel(J3DModelData *modelData)
{
    createBoundsSphere(sObjRadius, 1.0f);
}

void TMapObjMeteor::reset()
{
    resetObject();

    mAxisY.set(mObjData->yNorm);
    mAxisY.normalize();
    mAxisZ.set(mObjData->zNorm);
    mAxisZ.normalize();
    mAxisX.cross(mAxisY, mAxisZ);
    mAxisX.normalize();
    mAxisY.negate();

    mAxisX.scale(mObjData->scale.x * sBoxSize);
    mAxisY.scale(mObjData->scale.y * sBoxSize);
    mAxisZ.scale(mObjData->scale.z * sBoxSize);

    JGeometry::TVec3f center;
    center.x = mAxisY.x + 0.5f * mAxisX.x + 0.5f * mAxisZ.x;
    center.y = mAxisY.y + 0.5f * mAxisX.y + 0.5f * mAxisZ.y;
    center.z = mAxisY.z + 0.5f * mAxisX.z + 0.5f * mAxisZ.z;
    mItemVel.scaleAdd(-1.0f, center, mObjData->position);

    mScale.set(1.0f, 1.0f, 1.0f);
    PSMTXIdentity(mRotMtx);
    clrObjFlagHidding();
    clrObjFlagCheckItemHitting();
    mCheckKartHitFlags = 0;
    _58 = 13;

    f32 life = mObjData->mParam2;
    if (life <= 0.0f) {
        life = 100.0f;
    }
    mLife = life;

    JGeometry::TVec3f xAxis;
    JGeometry::TVec3f yAxis(0.0f, 1.0f, 0.0f);
    s16 angle = mObjData->mParam4 * 182.04445f;
    JGeometry::TVec3f zAxis(JMath::sincosTable_.cosShort(angle), 0.0f, JMath::sincosTable_.sinShort(angle));
    xAxis.cross(yAxis, zAxis);
    mRotMtx.setXYZDir(xAxis, yAxis, zAxis);

    Quaternion q = {0.0f, 0.0f, 0.0f, 1.0f};
    mEmitter = nullptr;
    ResetState();
}

void TMapObjMeteor::calc()
{
    ExecuteState();
}

void TMapObjMeteor::initFunc_Hide()
{
    setObjFlagHidding();
}

void TMapObjMeteor::doFunc_Hide()
{
    if (getStateCount() > mObjData->mParam3) {
        setState(1);
    }
}

void TMapObjMeteor::initFunc_Appear()
{
    f32 x = getGeoRnd()->get_ufloat_1();
    f32 z = getGeoRnd()->get_ufloat_1();

    JGeometry::TVec3f pos;
    pos.set(mAxisX);
    pos.scale(x);
    pos.scaleAdd(z, mAxisZ);
    mPos.add(mItemVel, pos);

    mScale.set(0.0f, 0.0f, 0.0f);
    mGeoObjFlag &= ~0x20;
    mEmitter = GetJPAMgr()->createEmt("mk_rbStarKira_a", mPos);

#line 171
    JUT_WARNING_F(mEmitter != nullptr, "%s", "TMapObjMeteor: Can't create Emitter");
}

void TMapObjMeteor::doFunc_Appear()
{
    f32 scale = SiUtil::getNormalRange(getStateCount(), 0.0f, sAppearFrame);
    scale = 0.0001f + scale;
    mScale.set(scale, scale, scale);

    if (getStateCount() > sAppearFrame) {
        setState(2);
    }

    moveMainProc();
}

void TMapObjMeteor::initFunc_Move()
{
    mVel.set(mAxisY);
    mVel.setLength(mLife);
    mScale.set(1.0f, 1.0f, 1.0f);
}

void TMapObjMeteor::doFunc_Move()
{
    moveMainProc();
    JGeometry::TVec3f target;
    JGeometry::TVec3f diff;
    target.add(mItemVel, mAxisY);
    diff.sub(target, mPos);
    if (checkGroundHitting()) {
        showItem();
        setState(3);
    } else if (diff.dot(mAxisY) < 0.0f) {
        setState(3);
    }
}

void TMapObjMeteor::moveMainProc()
{
    mPos += mVel;

    Mtx mtx;
    PSMTXIdentity(mtx);

    const f32 angle = sRotSpeed;
    f32 sin = sinf(angle);
    f32 cos = cosf(angle);

    mtx[1][0] = sin;
    mtx[0][0] = cos;
    mtx[0][1] = -sin;
    mtx[1][1] = cos;
    mtx[2][2] = 1.0f;
    mtx[2][1] = 0.0f;
    mtx[1][2] = 0.0f;
    mtx[2][0] = 0.0f;
    mtx[0][2] = 0.0f;

    PSMTXConcat(mRotMtx, mtx, mRotMtx);

    if (mObjData->mParam1 == 0) {
        getSoundMgr()->setSe(0x40050);
    }

    if (mEmitter != nullptr) {
        JPABaseEmitter *emt = mEmitter;
        emt->mGlobalScl.x = mScale.x;
        emt->mGlobalScl.y = mScale.y;
        emt->mGlobalScl.z = mScale.z;
        emt->mGlobalPScl.set(mScale.x, mScale.y);
        JPABaseEmitter *emt2 = mEmitter;
        emt2->mGlobalTrs.x = mPos.x;
        emt2->mGlobalTrs.y = mPos.y;
        emt2->mGlobalTrs.z = mPos.z;
    }
}

void TMapObjMeteor::initFunc_Crush()
{
    GetJPAMgr()->createEmt("mk_rbStarBreak_a", mPos);
    if (mEmitter != nullptr) {
        GetJPAMgr()->deleteEmt(mEmitter);
    }
    mScale.set(1.0f, 1.0f, 1.0f);
}

void TMapObjMeteor::doFunc_Crush()
{
    f32 scale = 0.0001f + (1.0f - SiUtil::getNormalRange(getStateCount(), 0.0f, sAppearFrame));

    if (scale <= 0.0001f) {
        setObjFlagHidding();
        setState(0);
    }

    mScale.set(scale, scale, scale);
}

bool TMapObjMeteor::checkGroundHitting()
{
    if (mObjData->mParam1 != 0) {
        return false;
    }

    bool hit = false;
    CrsGround ground(RaceMgr::sRaceManager->getCourse());
    ground.search(mPos);
    f32 height = ground.getHeight();

    switch (ground.getAttribute()) {
    case 2:
    case 5:
    case 10:
        break;
    default:
        if (mPos.y - sObjRadius < height) {
            hit = true;
        }
        break;
    }

    return hit;
}

void TMapObjMeteor::showItem()
{
    if (RaceMgr::sRaceManager->mRaceInfo->mRaceMode == 1) {
        return;
    }

    f32 random = getGeoRnd()->get_ufloat_1();
    if (random <= 0.3f) {
        ItemObjMgr::gItemMgr->occurItem(6, mPos, sItemVel);
        getSoundMgr()->setSe(0x40051);
        GetJPAMgr()->createEmt("mk_rbStarBreak_b", mPos);
    }
}

void TMapObjMeteor::InitExec()
{
    Observer_FindAndInit(TMapObjMeteor, 4);
}

void TMapObjMeteor::MoveExec()
{
    Observer_FindAndExec(TMapObjMeteor, 4);
}

#include "JSystem/JAudio/JASFakeMatch2.h"
