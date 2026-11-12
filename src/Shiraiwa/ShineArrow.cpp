#include "Shiraiwa/ShineArrow.h"
#include "Kaneshige/Objects/GeoRabbitMark.h"
#include "Kaneshige/RaceDrawer.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"
#include "Shiraiwa/ZCaptureMgr.h"
#include <math.h>

static const f32 sDummy[4] = {0.0f, 0.0f, 0.0f, 1.0f};

TShineArrowManager::TShineArrowManager()
{
    for (int i = 0; i < 4; i++) {
        mArrows[i] = nullptr;
    }
}

void TShineArrowManager::create()
{
    for (u8 i = 0; i < RaceMgr::getManager()->getStatusNumber(); i++) {
        mArrows[i] = (TShineArrow *)GetGeoObjMgr()->createSubObj(0x26b5);
        mArrows[i]->init(i, RaceMgr::getManager()->getTargetKartNo(i));
    }
}

StateObserver::StateFuncSet<TShineArrow> TShineArrow::sTable[4] = {
    {0, &TShineArrow::initFunc_Appear, &TShineArrow::doFunc_Appear},
    {1, &TShineArrow::initFunc_Point, &TShineArrow::doFunc_Point},
    {2, &TShineArrow::initFunc_Disappear, &TShineArrow::doFunc_Disappear},
    {3, &TShineArrow::initFunc_Hide, &TShineArrow::doFunc_Hide},
};

const f32 TShineArrow::scHeightOffset[21][4] = {
    {375.0f, 330.0f, 360.0f, 360.0f},
    {375.0f, 330.0f, 360.0f, 360.0f},
    {375.0f, 330.0f, 360.0f, 360.0f},
    {364.0f, 319.0f, 349.0f, 349.0f},
    {337.0f, 292.0f, 322.0f, 322.0f},
    {346.0f, 301.0f, 331.0f, 331.0f},
    {352.0f, 307.0f, 337.0f, 337.0f},
    {346.0f, 301.0f, 331.0f, 331.0f},
    {375.0f, 330.0f, 360.0f, 360.0f},
    {356.0f, 311.0f, 341.0f, 341.0f},
    {375.0f, 330.0f, 360.0f, 360.0f},
    {364.0f, 319.0f, 349.0f, 349.0f},
    {337.0f, 292.0f, 322.0f, 322.0f},
    {346.0f, 301.0f, 331.0f, 331.0f},
    {365.0f, 320.0f, 350.0f, 350.0f},
    {356.0f, 311.0f, 341.0f, 341.0f},
    {362.0f, 317.0f, 347.0f, 347.0f},
    {362.0f, 317.0f, 347.0f, 347.0f},
    {337.0f, 292.0f, 322.0f, 322.0f},
    {337.0f, 292.0f, 322.0f, 322.0f},
    {379.0f, 334.0f, 364.0f, 364.0f},
};

const f32 TShineArrow::scPakkunHeightOffset[21][4] = {
    {375.0f, 330.0f, 360.0f, 360.0f},
    {498.0f, 448.0f, 478.0f, 478.0f},
    {375.0f, 330.0f, 360.0f, 360.0f},
    {364.0f, 319.0f, 349.0f, 349.0f},
    {337.0f, 292.0f, 322.0f, 322.0f},
    {346.0f, 301.0f, 331.0f, 331.0f},
    {475.0f, 425.0f, 455.0f, 455.0f},
    {469.0f, 419.0f, 449.0f, 449.0f},
    {375.0f, 330.0f, 360.0f, 360.0f},
    {356.0f, 311.0f, 341.0f, 341.0f},
    {375.0f, 330.0f, 360.0f, 360.0f},
    {364.0f, 319.0f, 349.0f, 349.0f},
    {337.0f, 292.0f, 322.0f, 322.0f},
    {346.0f, 301.0f, 331.0f, 331.0f},
    {365.0f, 320.0f, 350.0f, 350.0f},
    {356.0f, 311.0f, 341.0f, 341.0f},
    {362.0f, 317.0f, 347.0f, 347.0f},
    {362.0f, 317.0f, 347.0f, 347.0f},
    {460.0f, 410.0f, 440.0f, 440.0f},
    {460.0f, 410.0f, 440.0f, 440.0f},
    {502.0f, 452.0f, 482.0f, 482.0f},
};

f32 TShineArrow::sScaleSpeed = 0.1f;
J3DAnmTextureSRTKey *TShineArrow::sShineArrowBtkAnm;
f32 TShineArrow::sRotRad;
s16 TShineArrow::sAppearFrame;
s16 TShineArrow::sEraseFrame;

#include "JSystem/JAudio/JASFakeMatch2.h"

TShineArrow::TShineArrow() : TMapObjHioNode(0x26b5), StateObserver(), mAnmObj(), mBtk()
{
    setObjFlagSimpleDraw();
}

TShineArrow::~TShineArrow() {}

void TShineArrow::init(u8 viewNo, u8 kartNo)
{
    mViewNo = viewNo;
    mKartNo = kartNo;
}

void TShineArrow::InitExec()
{
    Observer_FindAndInit(TShineArrow, 4);
}

void TShineArrow::MoveExec()
{
    Observer_FindAndExec(TShineArrow, 4);
}

void TShineArrow::createModel(JKRSolidHeap *, u32)
{
    mBtk.setExModel(&mModel);
    mBtk.mAnmBase = sShineArrowBtkAnm;
    mBtk.initFrameCtrl(mBtk.mAnmBase);
}

void TShineArrow::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    J3DAnmObjMaterial::setupTexSRTAnmData(&sShineArrowBtkAnm, modelData, ObjUtility::getPtrCourseArc("/Objects/MiniGameArrow.btk"));
}

static inline void setRotZMtx(Mtx m, f32 rad)
{
    f32 s = sin(rad);
    f32 c = cos(rad);
    m[0][0] = c;
    m[0][1] = -s;
    m[1][0] = s;
    m[1][1] = c;
    m[2][2] = 1.0f;
    m[2][1] = 0.0f;
    m[1][2] = 0.0f;
    m[2][0] = 0.0f;
    m[0][2] = 0.0f;
}

void TShineArrow::reset()
{
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();
    _58 = 0;
    PSMTXIdentity(mAimMtx);
    PSMTXIdentity(mSpinMtx);
    PSMTXIdentity(mRotSpeedMtx);
    setRotZMtx(mRotSpeedMtx, sRotRad);

    int frame;
    switch (RaceMgr::getManager()->getCourse()->getCourseID()) {
    case MINI_8:
        frame = 3;
        break;
    case MINI_7:
        frame = 1;
        break;
    case MINI_5:
        frame = 2;
        break;
    default:
        frame = 0;
        break;
    }
    mBtk.mFrameCtrl.setFrame(frame);

    KartInfo *kartInfo = RaceMgr::getManager()->getKartInfo(mKartNo);
    EKartID kartID = kartInfo->getKartID();
    if (kartInfo->getKartCharacter(0)->getCharID() == PETEY || kartInfo->getKartCharacter(1)->getCharID() == PETEY) {
        mHeightOffset = scPakkunHeightOffset[kartID];
    }
    else {
        mHeightOffset = scHeightOffset[kartID];
    }
}

void TShineArrow::update()
{
    mBtk.anmFrameProc();
    setModelMatrixAndScale();
    mModel.update(0);
}

void TShineArrow::calc()
{
    moveKartHead();
    ExecuteState();
}

void TShineArrow::moveKartHead()
{
    ObjUtility::getKartPos(mKartNo, &mPos);
    int con = RaceMgr::getManager()->getConsoleNumber() - 1;
#line 274
    JUT_MINMAX_ASSERT(0, con, 4);
    mPos.y += mHeightOffset[con];
}

void TShineArrow::viewCalc(u32 viewNo)
{
    if (viewNo != mViewNo) {
        mModel.clipAll(viewNo, false);
    }
}

void TShineArrow::initFunc_Appear()
{
    clrObjFlagHidding();
}

void TShineArrow::doFunc_Appear()
{
    aimToShine();
    f32 scale = mScale.y + sScaleSpeed;
    if (scale > 1.0f) {
        scale = 1.0f;
        setState(1);
    }
    mScale.set(scale, scale, scale);
}

void TShineArrow::initFunc_Point() {}

void TShineArrow::doFunc_Point()
{
    aimToShine();
    if (isDisappear()) {
        setState(2);
    }
}

bool TShineArrow::isDisappear()
{
    if (getStateCount() < sEraseFrame) {
        return false;
    }
    return isRabbitHolder(mKartNo);
}

bool TShineArrow::isAppear()
{
    if (getStateCount() < sAppearFrame) {
        changeCheckPos();
        return false;
    }
    return !isRabbitHolder(mKartNo);
}

void TShineArrow::changeCheckPos()
{
    GeoRabbitMarkSupervisor *supervisor = GeoRabbitMark::getSupervisor();
    if (supervisor) {
        JGeometry::TVec3f pos;
        supervisor->getMarkPosition(&pos);
        JGeometry::TVec2f screenPos;
        changeCheckPos(pos, &screenPos);
    }
}

void TShineArrow::changeCheckPos(const JGeometry::TVec3f &pos, JGeometry::TVec2f *screenPos)
{
    SiUtil::getScreenPosCheckMirror(mViewNo, pos, screenPos);
    RaceMgr::getManager()->mRaceDrawer->getCaptureMgr()->setPosition(10, mViewNo, screenPos->x, screenPos->y);
}

bool TShineArrow::isInside()
{
    bool inside = false;
    GeoRabbitMarkSupervisor *supervisor = GeoRabbitMark::getSupervisor();
    if (supervisor) {
        JGeometry::TVec3f pos;
        supervisor->getMarkPosition(&pos);
        JGeometry::TVec2f screenPos;
        SiUtil::getScreenPosCheckMirror(mViewNo, pos, &screenPos);
        JGeometry::TBox2f viewport;
        SiUtil::getViewport(&viewport, mViewNo, SiUtil::getDrawConsoleNum());
        if (screenPos.x >= viewport.i.x && screenPos.x <= viewport.f.x && screenPos.y >= viewport.i.y &&
            screenPos.y <= viewport.f.y) {
            inside = true;
        }
    }
    return inside;
}

void TShineArrow::aimToShine()
{
    JGeometry::TVec3f xDir, yDir, zDir;
    JGeometry::TVec3f shinePos(0.0f, 0.0f, 0.0f);
    GeoRabbitMarkSupervisor *supervisor = GeoRabbitMark::getSupervisor();
    if (supervisor) {
        supervisor->getMarkPosition(&shinePos);
    }
    zDir.sub(shinePos, mPos);
    if (zDir.squared() <= JGeometry::TUtilf::epsilon()) {
        zDir.set(0.0f, 1.0f, 0.0f);
    }
    zDir.normalize();
    yDir.set(0.0f, 1.0f, 0.0f);
    xDir.cross(yDir, zDir);
    xDir.normalize();
    yDir.cross(zDir, xDir);
    yDir.normalize();
    mAimMtx[0][0] = xDir.x;
    mAimMtx[1][0] = xDir.y;
    mAimMtx[2][0] = xDir.z;
    mAimMtx[0][1] = yDir.x;
    mAimMtx[1][1] = yDir.y;
    mAimMtx[2][1] = yDir.z;
    mAimMtx[0][2] = zDir.x;
    mAimMtx[1][2] = zDir.y;
    mAimMtx[2][2] = zDir.z;
    PSMTXConcat(mSpinMtx, mRotSpeedMtx, mSpinMtx);
    PSMTXConcat(mAimMtx, mSpinMtx, mRotMtx);
}

bool TShineArrow::isRabbitHolder(u32 kartNo)
{
    bool ret = false;
    GeoRabbitMarkSupervisor *supervisor = GeoRabbitMark::getSupervisor();
    if (supervisor) {
        u32 rabbitKartNo = supervisor->getRabbitKartNo();
        if (rabbitKartNo == kartNo) {
            ret = true;
        }
    }
    return ret;
}

void TShineArrow::initFunc_Disappear() {}

void TShineArrow::doFunc_Disappear()
{
    aimToShine();
    f32 scale = mScale.y - sScaleSpeed;
    if (scale < 0.01f) {
        scale = 0.01f;
        setState(3);
    }
    mScale.set(scale, scale, scale);
}

void TShineArrow::initFunc_Hide()
{
    setObjFlagHidding();
}

void TShineArrow::doFunc_Hide()
{
    if (isAppear()) {
        setState(0);
    }
}
