#include "Shiraiwa/TestString.h"

#include <JSystem/J3D/J3DModelLoader.h>
#include <JSystem/J3D/J3DTransform.h>
#include <JSystem/JUtility/JUTAssert.h>
#include <dolphin/gx.h>
#include <dolphin/gx/GXVert.h>

#include "Kaneshige/Objects/GeoItemCar.h"
#include "Yamamoto/KartStrat.h"
#include "Yamamoto/kartBody.h"
#include "Yamamoto/kartCtrl.h"
#include "Sato/Objects/GeoItemBox.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/ObjUtility.h"

static const f32 sDummy[4] = {0.0f, 0.0f, 0.0f, 1.0f};

f32 TTestStrBarrel::sItemVelScale = 0.6f;
f32 TTestStrBarrel::sKartVelScale = 3.0f;

f32 TTestString::sAdjustRatio = 0.2f;
f32 TTestString::sAirFric = 0.999999f;
f32 TTestString::sGravity = -0.2f;
f32 TTestString::sWidth = 20.0f;
f32 TTestString::sDownVel = 5.0f;

TTestStrSupervisor *TTestString::sSupervisor;

const f32 TTestStrSupervisor::sStringOffset[18] = {
    -5.0f, 50.0f, -5.77f, 5.0f, 50.0f, -5.77f, 0.0f, 50.0f, 4.23f,
    -5.0f, 0.0f, -5.77f, 5.0f, 0.0f, -5.77f, 0.0f, 0.0f, 4.23f,
};

const f32 TTestStrSupervisor::sStringTexOffset[12] = {
    1.0f, 0.0f, 1.0f, 0.33f, 1.0f, 0.66f,
    0.0f, 0.0f, 0.0f, 0.33f, 0.0f, 0.66f,
};

void TTestStrBarrel::reset()
{
    resetObject();
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
    mGeoObjFlag &= ~0x20;
    _58 = 0;
}


void TTestStrBarrel::calc()
{
    JGeometry::TVec3f cross1;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f up;

    mString->getPoint(&mPos);
    up.x = mString->mObjData->zNorm.x;
    up.y = mString->mObjData->zNorm.y;
    up.z = mString->mObjData->zNorm.z;
    mString->getPointDir(&dir);
    dir.negate();
    cross1.cross(dir, up);
    cross1.normalize();
    up.cross(cross1, dir);
    up.normalize();
    mPos.scaleAdd(-100.0f, dir, mPos);

    mRotMtx[0][0] = cross1.x;
    mRotMtx[1][0] = cross1.y;
    mRotMtx[2][0] = cross1.z;
    mRotMtx[0][1] = dir.x;
    mRotMtx[1][1] = dir.y;
    mRotMtx[2][1] = dir.z;
    mRotMtx[0][2] = up.x;
    mRotMtx[1][2] = up.y;
    mRotMtx[2][2] = up.z;

    checkItemHitting();
    moveShadowModel();
}

void TTestStrBarrel::moveShadowModel()
{
    if ((mShadowMdl->getFlags() & 0xf) == 0)
        return;

    CrsGround ground(RCMGetCourse());
    ground.search(this, mPos, mPos);

    Mtx m;
    PSMTXCopy(mRotMtx, m);
    m[0][3] = mPos.x;
    m[1][3] = ground.getHeight();
    m[2][3] = mPos.z;
    mShadowMdl->setBaseTRMtx(m);
    JGeometry::TVec3f *scale = (JGeometry::TVec3f *)((u8 *)mShadowMdl->getModel() + 0x18);
    scale->x = mScale.x;
    scale->y = mScale.y;
    scale->z = mScale.z;
}

void TTestStrBarrel::checkItemHitting()
{
    JGeometry::TVec3f vel;
    JGeometry::TVec3f dir;
    if (tstItemHitting()) {
        ItemObj *item = mColItemObj;
        u32 kind = item->getKind();
        switch (kind) {
        case 0:
        case 1:
        case 2:
        case 7:
        case 9:
        case 11:
            vel.set(item->getVel());
            if (mReaction.tstFlg(kind) == 1) {
                vel.negate();
            }
            vel.scale(sItemVelScale);
            mString->attack(vel);
            break;
        case 8:
            dir.sub(mPos, JGeometry::TVec3f(item->getPos()));
            dir.setLength(50.0f);
            mString->attack(dir);
            break;
        }
    }
}

void TTestStrBarrel::createColModel(J3DModelData *)
{
    createBoundsSphere(115.0f, 1.0f);
}

void TTestStrBarrel::doKartColCallBack(int kartNo)
{
    JGeometry::TVec3f vel;
    JGeometry::TVec3f kartPos;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f up;
    JGeometry::TVec3f pos;
    f32 radius;

    ObjUtility::getKartVel(kartNo, &vel);
    if (vel.squared() < 1.0f) {
        vel.zero();
    }
    vel.scale(sKartVelScale);
    GeographyObjManager::gMgr->getKartCollidePosRadius(kartPos, radius, kartNo);
    dir.sub(getBoundsGlPos(0), kartPos);
    dir.normalize();
    mRotMtx.getYDir(up);
    up.normalize();
    up.scale(100.0f);
    pos.scaleAdd(115.0f + radius, dir, kartPos);
    pos.add(up);
    mString->attack(pos, vel);
    vibrate(kartNo);
    KartCtrl::getKartCtrl()->getKartSound(kartNo)->mSoundMgr->setCrushSe(getSoundID(), 0.03f * getBounds(0)->getBoundDepth());
}

__declspec(weak) u32 TTestStrBarrel::getSoundID() const { return 0x10063; }

void TTestStrBarrel::vibrate(int kartNo)
{
    JGeometry::TVec3f vel;
    ObjUtility::getKartVel(kartNo, &vel);
    if (vel.squared() > 100.0f) {
        KartBody *body = KartCtrl::getKartCtrl()->getKartBody(kartNo);
        body->getStrat()->DoMotor((MotorManager::MotorType)getMotorType());
    }
}

void TTestStrItem::reset()
{
    resetObject();
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = -1;
    mGeoObjFlag &= ~0x20;
    mTimer = 0;
    _58 = 0;
    for (int i = 0; i < 2; i++)
        mEmitters[i] = nullptr;
}


void TTestStrItem::calc()
{
    JGeometry::TVec3f cross1;
    JGeometry::TVec3f dir;

    mString->getPoint(&mPos);
    JGeometry::TVec3f up;
    up.set(0.0f, 0.0f, 1.0f);
    mString->getPointDir(&dir);
    cross1.cross(dir, up);
    cross1.normalize();
    up.cross(cross1, dir);
    up.normalize();

    mRotMtx[0][0] = cross1.x;
    mRotMtx[1][0] = cross1.y;
    mRotMtx[2][0] = cross1.z;
    mRotMtx[0][1] = dir.x;
    mRotMtx[1][1] = dir.y;
    mRotMtx[2][1] = dir.z;
    mRotMtx[0][2] = up.x;
    mRotMtx[1][2] = up.y;
    mRotMtx[2][2] = up.z;

    moveShadowModel();

    if (mGeoObjFlag & 0x20) {
        mTimer++;
        if (mTimer > 600) {
            mCheckKartHitFlags = -1;
            mGeoObjFlag &= ~0x20;
            mTimer = 0;
        }
    }
}

void TTestStrItem::doKartColCallBack(int kartNo)
{
    if (mIsHitKartFlags & (1 << kartNo)) {
        GeoItemCar::hitItemBoxProc(kartNo, mEmitters);
        mCheckKartHitFlags = 0;
        mGeoObjFlag |= 0x20;
    }
}

void TTestStrItem::createColModel(J3DModelData *)
{
    createBoundsSphere(150.0f, 1.0f);
}

static void forceJSUListDtor(JSUList<TTestString> *list) { list->~JSUList<TTestString>(); }

TTestStrSupervisor::~TTestStrSupervisor() { delete mTexture; }

void TTestStrSupervisor::drawAll(u32 cameraNo)
{
    GXColor color = { 0xff, 0xff, 0xff, 0xff };
    GXSetTevColor(GX_TEVREG0, color);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetNumTexGens(1);
    GXSetNumTevStages(1);
    GXSetNumChans(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0, GX_FALSE, GX_PTIDENTITY);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
    GXSetCurrentMtx(0);
    GXSetCullMode(GX_CULL_NONE);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_OR, GX_GREATER, 0);
    mTexture->load(GX_TEXMAP0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_TEX0, (void *)sStringTexOffset, 8);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    for (JSULink<TTestString> *link = mList.getFirst(); link != nullptr; link = link->getNext())
        link->getObject()->draw(cameraNo);
}

void TTestStrSupervisor::load()
{
    ResTIMG *timg = (ResTIMG *)ObjUtility::getPtrCourseArc("/Objects/TestHimo1Tex.bti");
#line 387
    JUT_ASSERT(timg != 0);
    timg->mWrapS = 1;
    mTexture->storeTIMG(timg, (u8)0);

    J3DModelData *modelData = J3DModelLoaderDataBase::load(ObjUtility::getPtrCourseArc("/Objects/TestHimo1.bmd"), 0x19200010);
    modelData->newSharedDisplayList(0x40000);
    modelData->simpleCalcMaterial((u16)0, (MtxPtr)j3dDefaultMtx);
    modelData->makeSharedDL();

    for (JSULink<TTestString> *link = mList.getFirst(); link != nullptr; link = link->getNext())
        link->getObject()->load(modelData);
}

void TTestStrSupervisor::createModel(JKRSolidHeap *heap, u32 p2)
{
    for (JSULink<TTestString> *link = mList.getFirst(); link != nullptr; link = link->getNext())
        link->getObject()->createModel(heap, p2);
}

TTestString::TTestString(const CrsData::SObject &obj) : GeographyObj(obj), JKRDisposer(), mLink(this)
{
    mNodeNum = obj.mParam1;
    if (mNodeNum < 2)
        mNodeNum = 5;
    mLength = obj.mParam2;
    if (mLength <= 0.0f)
        mLength = 1000.0f;
    mNodeLengths = new f32[mNodeNum];
    mNodeMgr = new ExStringNodeManager(mNodeNum, 30.0f, true, false, 0);

    if (sSupervisor == nullptr) {
        sSupervisor = new TTestStrSupervisor();
    }
    sSupervisor->mList.append(&mLink);

    mBarrel = (TTestStrBarrel *)GeographyObjManager::gMgr->createSubObj(0xe0d);
    mBarrel->mString = this;
}

TTestString::~TTestString()
{
    sSupervisor = nullptr;
    delete mNodeMgr;
}

// Inlines from StringObj.h, written out with the original header's assert lines
#define STRINGOBJ_NUM_ASSERT(line, num, max)                                                                    \
    (((0 <= (num)) && ((num) < (max))))                                                                         \
        ? (void)0                                                                                               \
        : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "StringObj.h", line,                          \
                                      "range over: %d <= num=%d < %d", 0, (num), (max)),                        \
           OSPanic("StringObj.h", line, "Halt"));

template <typename T>
static inline void setNodeLength(T *mgr, u32 num, f32 length)
{
    STRINGOBJ_NUM_ASSERT(202, num, mgr->mStrNodeList.getNumLinks());
    mgr->mStrNodeList.getNth(num)->getObject()->_34 = length;
}

template <typename T>
static inline void clrNodeFlag(T *mgr, u32 num)
{
    STRINGOBJ_NUM_ASSERT(112, num, mgr->mStrNodeList.getNumLinks());
    mgr->mStrNodeList.getNth(num)->getObject()->_30 = 0;
}

void TTestString::reset()
{
    resetObject();
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;
    PSMTXIdentity(mRotMtx);
    mNodeMgr->resetNodeAll(nullptr);

    if (mObjData->mParam3 == 0) {
        f32 length = mLength;
        for (u32 i = 0; i < mNodeNum - 1; i++) {
            length *= 0.5f;
            mNodeLengths[i] = length;
            setNodeLength(mNodeMgr, i, mNodeLengths[i]);
            clrNodeFlag(mNodeMgr, i);
        }
        mNodeLengths[mNodeNum - 1] = length;
        setNodeLength(mNodeMgr, mNodeNum - 1, mNodeLengths[mNodeNum - 1]);
    } else {
        f32 length = mLength / (f32)mNodeNum;
        mNodeMgr->setNodeLengthAll(length);
        for (u32 i = 0; i < mNodeNum - 1; i++) {
            mNodeLengths[i] = length;
            clrNodeFlag(mNodeMgr, i);
        }
    }
    setParameters();
}

void TTestString::load(J3DModelData *) {}

void TTestString::calc()
{
    JGeometry::TVec3f up;
    getUpDir(&up);
    *(JGeometry::TVec3f *)&mNodeMgr->_28 = up;
    mNodeMgr->calc();
    mNodeMgr->setNodePos(0, mObjData->position);

    JGeometry::TVec3f vel;
    vel.scale(-sDownVel, up);
    mNodeMgr->addNodeVel(mNodeNum - 1, vel);
}

void TTestString::attack(const JGeometry::TVec3f &vel)
{
    mNodeMgr->addNodeVel(mNodeNum - 1, vel);
}

void TTestString::attack(const JGeometry::TVec3f &pos, const JGeometry::TVec3f &vel)
{
    mNodeMgr->setNodePos(mNodeNum - 1, pos);
    mNodeMgr->addNodeVel(mNodeNum - 1, vel);
}
void TTestString::draw(u32 cameraNo)
{
    JGeometry::TVec3f pos1;
    JGeometry::TVec3f pos2;

    for (int i = 0; i < mNodeNum - 1; i++) {
        mNodeMgr->getNodePos(i, &pos1);
        mNodeMgr->getNodePos(i + 1, &pos2);
        if (i == 0) {
            mPoints[0].set(pos1);
            mPoints[1].set(pos1.x + sWidth, pos1.y, pos1.z);
            mPoints[2].set(pos1.x + 0.5f * sWidth, pos1.y, pos1.z + 0.85f * sWidth);
        }
        drawNode(mNodeLengths[i], pos1, pos2, cameraNo);
    }
}

void TTestString::drawNode(f32 length, JGeometry::TVec3f &p1, JGeometry::TVec3f &p2, u32 cameraNo)
{
    Mtx transMtx;
    Mtx texMtx;
    JGeometry::TVec3f side;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f up;

    MtxPtr camMtx = RaceMgr::getManager()->getCamera(cameraNo)->GetMtx();
    PSMTXTrans(transMtx, p1.x, p1.y, p1.z);

    up.set(0.0f, 0.0f, 1.0f);
    dir.set(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
    dir.normalize();
    side.cross(dir, up);
    side.normalize();
    up.cross(side, dir);
    up.normalize();

    mRotMtx[0][0] = side.x;
    mRotMtx[1][0] = side.y;
    mRotMtx[2][0] = side.z;
    mRotMtx[0][1] = dir.x;
    mRotMtx[1][1] = dir.y;
    mRotMtx[2][1] = dir.z;
    mRotMtx[0][2] = up.x;
    mRotMtx[1][2] = up.y;
    mRotMtx[2][2] = up.z;
    GXLoadPosMtxImm(camMtx, 0);

    PSMTXScale(texMtx, length / 50.0f, 1.0f, 0.0f);
    GXLoadTexMtxImm(texMtx, 30, GX_MTX2x4);

    JGeometry::TVec3f ofs[3];
    ofs[0].set(up * 0.33f * sWidth);
    ofs[1].set((side * -0.5f - up * 0.167f) * sWidth);
    ofs[2].set((side * 0.5f - up * 0.167f) * sWidth);
    JGeometry::TVec3f center;
    center.set(dir);
    center.scale(0.5f * length);

    drawCrevicePyramid(p1, ofs[1], ofs[2]);
    drawPrism(p1, p2, ofs[0], ofs[1], ofs[2]);

    mPoints[0].set(p2 + ofs[0]);
    mPoints[1].set(p2 + ofs[1]);
    mPoints[2].set(p2 + ofs[2]);
}

void TTestString::drawCrevicePyramid(JGeometry::TVec3f &p1, JGeometry::TVec3f &p2, JGeometry::TVec3f &p3)
{
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 5);
    GXPosition3f32(mPoints[0].x, mPoints[0].y, mPoints[0].z);
    GXWGFifo.u8 = 3;
    GXPosition3f32(mPoints[1].x, mPoints[1].y, mPoints[1].z);
    GXWGFifo.u8 = 0;
    GXPosition3f32(p1.x + p2.x, p1.y + p2.y, p1.z + p2.z);
    GXWGFifo.u8 = 4;
    GXPosition3f32(p1.x + p3.x, p1.y + p3.y, p1.z + p3.z);
    GXWGFifo.u8 = 1;
    GXPosition3f32(mPoints[2].x, mPoints[2].y, mPoints[2].z);
    GXWGFifo.u8 = 2;

    GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
    GXPosition3f32(mPoints[0].x, mPoints[0].y, mPoints[0].z);
    GXWGFifo.u8 = 3;
    GXPosition3f32(p1.x + p3.x, p1.y + p3.y, p1.z + p3.z);
    GXWGFifo.u8 = 0;
    GXPosition3f32(mPoints[2].x, mPoints[2].y, mPoints[2].z);
    GXWGFifo.u8 = 4;
}

void TTestString::drawPrism(JGeometry::TVec3f &p1, JGeometry::TVec3f &p2, JGeometry::TVec3f &p3, JGeometry::TVec3f &p4,
                            JGeometry::TVec3f &p5)
{
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 5);
    GXPosition3f32(p2.x + p3.x, p2.y + p3.y, p2.z + p3.z);
    GXWGFifo.u8 = 3;
    GXPosition3f32(p1.x + p3.x, p1.y + p3.y, p1.z + p3.z);
    GXWGFifo.u8 = 0;
    GXPosition3f32(p2.x + p4.x, p2.y + p4.y, p2.z + p4.z);
    GXWGFifo.u8 = 4;
    GXPosition3f32(p1.x + p4.x, p1.y + p4.y, p1.z + p4.z);
    GXWGFifo.u8 = 1;
    GXPosition3f32(p1.x + p5.x, p1.y + p5.y, p1.z + p5.z);
    GXWGFifo.u8 = 2;

    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 5);
    GXPosition3f32(p2.x + p4.x, p2.y + p4.y, p2.z + p4.z);
    GXWGFifo.u8 = 4;
    GXPosition3f32(p1.x + p5.x, p1.y + p5.y, p1.z + p5.z);
    GXWGFifo.u8 = 2;
    GXPosition3f32(p2.x + p5.x, p2.y + p5.y, p2.z + p5.z);
    GXWGFifo.u8 = 5;
    GXPosition3f32(p1.x + p3.x, p1.y + p3.y, p1.z + p3.z);
    GXWGFifo.u8 = 0;
    GXPosition3f32(p2.x + p3.x, p2.y + p3.y, p2.z + p3.z);
    GXWGFifo.u8 = 3;
}

void TTestString::getUpDir(JGeometry::TVec3f *up)
{
    JGeometry::TVec3f *dir = (JGeometry::TVec3f *)((u8 *)RCMGetCourse() + 0x234);
    f32 y = dir->y;
    f32 z = dir->z;
    f32 x = dir->x;
    up->x = x;
    up->y = y;
    up->z = z;
}

void TTestString::getPoint(JGeometry::TVec3f *pos)
{
    mNodeMgr->getNodePos(mNodeNum - 1, pos);
}

void TTestString::getPointDir(JGeometry::TVec3f *dir)
{
    JGeometry::TVec3f prev;
    JGeometry::TVec3f cur;
    mNodeMgr->getNodePos(mNodeNum - 2, &prev);
    mNodeMgr->getNodePos(mNodeNum - 1, &cur);
    dir->sub(cur, prev);
    dir->normalize();
}

void TTestString::setParameters()
{
    mNodeMgr->_24 = sAdjustRatio;
    mNodeMgr->_20 = sAirFric;
    mNodeMgr->_1c = sGravity;
}

void TTestString::createModel(JKRSolidHeap *, u32) {}


#include <JSystem/JAudio/JASFakeMatch2.h>
