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

f32 TTestStrBarrel::sItemVelScale = 0.6f;
f32 TTestStrBarrel::sKartVelScale = 3.0f;

f32 TTestString::sAdjustRatio = 0.2f;
f32 TTestString::sAirFric = 0.999999f;
f32 TTestString::sGravity = -0.2f;
f32 TTestString::sWidth = 20.0f;
f32 TTestString::sDownVel = 5.0f;

TTestStrSupervisor *TTestString::sSupervisor;

f32 TTestStrSupervisor::sStringOffset[18] = {
    -5.0f, 50.0f, -5.77f, 5.0f, 50.0f, -5.77f, 0.0f, 50.0f, 4.23f,
    -5.0f, 0.0f, -5.77f, 5.0f, 0.0f, -5.77f, 0.0f, 0.0f, 4.23f,
};

f32 TTestStrSupervisor::sStringTexOffset[12] = {
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
    JMAVECScaleAdd(&mPos, &mPos, &dir, -100.0f);

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

void TTestStrBarrel::checkItemHitting() {}

void TTestStrBarrel::createColModel(J3DModelData *)
{
    createBoundsSphere(5.0f, 0.0f);
}

void TTestStrBarrel::doKartColCallBack(int) {}

__declspec(weak) u32 TTestStrBarrel::getSoundID() const { return 0x10063; }

void TTestStrBarrel::vibrate(int kartNo)
{
    JGeometry::TVec3f vel;
    ObjUtility::getKartVel(kartNo, &vel);
    if (vel.squared() > 4.0f) {
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
    up.x = 0.0f;
    up.y = up.x;
    up.z = 0.0f;
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
    createBoundsSphere(5.0f, 0.0f);
}

static void forceJSUListDtor(JSUList<TTestString> *list) { list->~JSUList<TTestString>(); }

TTestStrSupervisor::~TTestStrSupervisor() { delete mTexture; }

void TTestStrSupervisor::drawAll(u32 cameraNo)
{
    GXColor color = { 0x42, 0x48, 0, 0 };
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
    GXSetArray(GX_VA_TEX0, sStringTexOffset, 8);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    for (JSULink<TTestString> *link = mList.getFirst(); link != nullptr; link = link->getNext())
        link->getObject()->draw(cameraNo);
}

void TTestStrSupervisor::load()
{
    ResTIMG *timg = (ResTIMG *)ObjUtility::getPtrCourseArc("/Objects/TestHimo1Tex.bti");
    if (timg == nullptr) {
        JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "TestString.cpp", 387, "", "timg != 0");
        OSPanic("TestString.cpp", 387, "Halt");
    }
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
        mLength = 50.0f;
    mNodeLengths = new f32[mNodeNum];
    mNodeMgr = new ExStringNodeManager(mNodeNum, 115.0f, true, false, 0);

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

void TTestString::reset()
{
    resetObject();
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;
    PSMTXIdentity(mRotMtx);
    mNodeMgr->resetNodeAll(nullptr);

    f32 length = mLength;
    if (mObjData->mParam1 == 0) {
        for (int i = 0; i < mNodeNum - 1; i++) {
            length *= 0.5f;
            mNodeLengths[i] = length;
            StringNode *node = mNodeMgr->mStrNodeList.getNth(i)->getObject();
            node->_34 = mNodeLengths[i];
            node->_30 = 0;
        }
        mNodeLengths[mNodeNum - 1] = length;
        StringNode *node = mNodeMgr->mStrNodeList.getNth(mNodeNum - 1)->getObject();
        node->_34 = mNodeLengths[mNodeNum - 1];
    } else {
        length /= (f32)mNodeNum;
        mNodeMgr->setNodeLengthAll(length);
        for (int i = 0; i < mNodeNum - 1; i++) {
            mNodeLengths[i] = length;
            StringNode *node = mNodeMgr->mStrNodeList.getNth(i)->getObject();
            node->_34 = length;
            node->_30 = 0;
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

    f32 downVel = -sDownVel;
    JGeometry::TVec3f vel(0.0f, downVel, 0.0f);
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
            f32 half = 0.5f;
            f32 slant = 0.85f;
            mPoints[0].x = pos1.x;
            mPoints[0].y = pos1.y;
            mPoints[0].z = pos1.z;
            mPoints[1].x = pos1.x + sWidth;
            mPoints[1].y = pos1.y;
            mPoints[1].z = pos1.z;
            mPoints[2].x = pos1.x + half * sWidth;
            mPoints[2].y = pos1.y;
            mPoints[2].z = pos1.z + slant * sWidth;
        }
        drawNode(mNodeLengths[i], pos1, pos2, cameraNo);
    }
}

void TTestString::drawNode(f32, JGeometry::TVec3f &, JGeometry::TVec3f &, u32) {}

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
