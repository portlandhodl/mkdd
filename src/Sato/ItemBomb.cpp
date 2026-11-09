#include "Sato/ItemBomb.h"

#include "Sato/J3DAnmObject.h"
#include "Sato/JPEffectMgr.h"
#include "JSystem/J3D/J3DAnmLoader.h"
#include "JSystem/J3D/J3DModel.h"
#include "Osako/ResMgr.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/ObjUtility.h"

namespace
{
    u8 sMaxBombReactFrame = 20;
    f32 sFindNearKartLength = 1400.0f;

    const char *sColorAnmName[7] = {
        "/Item/item_bomb_flash.bpk",
        "/Item/item_bomb_wait.bpk",
        "/Item/item_bomb_flash_fast.bpk",
        "/Item/item_bomb_p1.bpk",
        "/Item/item_bomb_p2.bpk",
        "/Item/item_bomb_p3.bpk",
        "/Item/item_bomb_p4.bpk",
    };
} // namespace

J3DAnmTransform *ItemBomb::mAnmTrans[3] = {nullptr, nullptr, nullptr};
J3DMtxCalc *ItemBomb::mAnmCalc[3] = {nullptr, nullptr, nullptr};
J3DAnmColor *ItemBomb::sColorAnm[7] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
Vec ItemBomb::sHandOffsetPos;
J3DAnmColor *ItemBomb::sNowColorAnm;
int ItemBomb::sLocatorJointNo;

const char *ItemBomb::getBmdFileName()
{
    return "/Item/item_bomb_L.bmd";
}

void ItemBomb::loadAnmData(J3DModelData *model)
{
    for (int i = 0; i < 3; i++) {
        void *anm = ResMgr::getPtr(ResMgr::mcArcMRAM, getBckName(i));
        mAnmTrans[i] = (J3DAnmTransform *)J3DAnmLoaderDataBase().load(anm, J3DLOADER_UNK_FLAG0);
        mAnmCalc[i] = J3DNewMtxCalcAnm(model->getMtxCalcType() & 0xf, mAnmTrans[i]);
    }
    for (u8 i = 0; i < 7; i++) {
        void *anm = ObjUtility::getPtrMainArc((char *)sColorAnmName[i]);
        J3DAnmObjMaterial::setupColorAnmData(&sColorAnm[i], model, anm);
    }
    sLocatorJointNo = model->getJointName()->getIndex("ef_fire");
}

const char *ItemBomb::getBckName(int idx)
{
    const char *ret = nullptr;
    switch (idx) {
    case 0:
        ret = "/Item/item_bomb_L_wait_a.bck";
        break;
    case 1:
        ret = "/Item/item_bomb_L_down.bck";
        break;
    case 2:
        ret = "/Item/item_bomb_L_wait_b.bck";
        break;
    }
    return ret;
}

void ItemBomb::setAnimation(BombBck type, int frame)
{
    mAnmType = type;
    mAnmFrame = frame;
    mAnmRepeat = 0;
}

void ItemBomb::deleteEffect()
{
    if (mBlastEmt == nullptr)
        return;

    GetJPAMgr()->deleteEmt(mBlastEmt);
    mBlastEmt = nullptr;
}

ItemObj::ItemColFlag ItemBomb::calcColReaction(ItemObj *other)
{
    return cColFlag_4;
}

void ItemBomb::doEquip()
{
    if (GetGeoObjMgr()->isBombBattle()) {
        mMulti->MoveEquip();
    } else {
        doHandOneEquip(false, 0.0f);
    }
}
void ItemBomb::doSpRelease()
{
    doRelease();
}

const bool ItemBomb::checkKartColDisappear(int kart)
{
    if (mDirectHitKartNo == -1)
        mDirectHitKartNo = kart;
    mIsColKart = 1;
    return false;
}

void ItemBomb::setCurrentViewNo(u32 viewNo)
{
    Mtx lightMtx;
    mModel.setCurrentViewNo(viewNo);
    ObjUtility::getCamDependLightMtx(viewNo, mPos, lightMtx);
    mModel.setEffectMtx(lightMtx, 0);
}

bool ItemBomb::findAnyKart()
{
    u8 found = 0;
    int kart = ObjUtility::findNearKart(mPos, sFindNearKartLength, mOwnerNum);
    if (kart >= 0 && kart != mOwnerNum)
        found = 1;
    return found != 0;
}

u8 ItemBomb::getMaxBombReactFrame()
{
    return sMaxBombReactFrame;
}

ItemBombSuc::ItemBombSuc(u32 kind) : ItemObjSuc(kind) {}

void ItemBombSuc::reset()
{
    ItemObjSuc::reset();
    mKartReaction = 15;
    _1fc = 3;
    mSucKind = 8;
}

