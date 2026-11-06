#include "Sato/J3DEfctIceRock.h"
#include "Kaneshige/KartInfo.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/J3DEffectMgr.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/JPEffectPerformer.h"
#include "Sato/ObjUtility.h"

#include <JSystem/JAudio/JASFakeMatch2.h>
#include <JSystem/JGeometry.h>
#include <JSystem/JUtility/JUTAssert.h>

J3DAnmTransform *J3DEfctIceRock::sBckAnmTrans[1] = { 0 };
J3DMtxCalc *J3DEfctIceRock::sBckMtxCalc[1] = { 0 };
J3DAnmTextureSRTKey *J3DEfctIceRock::sTexSRTKey;

struct IceOffsetData {
    f32 mScaleX;
    f32 mScaleY;
    f32 mScaleZ;
    f32 mOffsetY;
    f32 mOffsetZ;
};

const char *sJPAFileName[3] = {
    "mk_freeze_d",
    "mk_freeze_b",
    "mk_freeze_c",
};

IceOffsetData sIceOffsetData[21] = {
    { 0.92f, 0.92f, 0.92f, -80.0f, -10.0f },
    { 1.1f, 1.1f, 1.3f, -90.0f, 10.0f },
    { 0.95f, 0.95f, 1.0f, -70.0f, 5.0f },
    { 0.9f, 0.9f, 0.9f, -100.0f, 0.0f },
    { 0.95f, 0.95f, 0.95f, -80.0f, 5.0f },
    { 0.92f, 0.92f, 0.92f, -60.0f, -10.0f },
    { 1.3f, 1.3f, 1.5f, -90.0f, 25.0f },
    { 1.25f, 1.1f, 1.25f, -100.0f, 0.0f },
    { 0.92f, 0.92f, 0.92f, -80.0f, -10.0f },
    { 0.92f, 0.92f, 1.05f, -70.0f, 25.0f },
    { 0.95f, 0.95f, 1.0f, -70.0f, 5.0f },
    { 0.9f, 0.9f, 0.9f, -100.0f, 0.0f },
    { 0.95f, 0.95f, 0.95f, -80.0f, 5.0f },
    { 0.92f, 0.92f, 0.92f, -60.0f, -10.0f },
    { 1.1f, 1.0f, 1.2f, -80.0f, 0.0f },
    { 0.92f, 0.92f, 0.92f, -75.0f, 0.0f },
    { 0.92f, 0.92f, 0.92f, -90.0f, 0.0f },
    { 0.92f, 0.92f, 0.92f, -90.0f, 0.0f },
    { 1.1f, 1.1f, 1.1f, -95.0f, 0.0f },
    { 1.1f, 1.1f, 1.1f, -95.0f, 0.0f },
    { 1.2f, 1.2f, 1.5f, -90.0f, -20.0f },
};


J3DEfctIceRock::J3DEfctIceRock() {}

void J3DEfctIceRock::loadAnmData() {
    J3DModelData *mdlData = J3DEffectMgr::sJ3DMgr->getModelData();
    for (u8 i = 0; i < 1; i++) {
        void *bck = ObjUtility::getPtrMainArc((char *)getBckFileName((u8)i));
        J3DAnmObjTrans::setupTransAnmData(&sBckAnmTrans[i], &sBckMtxCalc[i], mdlData, bck);
    }
    void *btk = ObjUtility::getPtrMainArc((char *)getBtkFileName());
    J3DAnmObjMaterial::setupTexSRTAnmData(&sTexSRTKey, mdlData, btk);
}

const char *J3DEfctIceRock::getBmdFileName() {
    return "/Effect/IceRock.bmd";
}

const char *J3DEfctIceRock::getBckFileName(u8 index) {
    const char *name = nullptr;
    if (index == 0) name = "/Effect/IceRock_app.bca";
    return name;
}

const char *J3DEfctIceRock::getBtkFileName() {
    return "/Effect/IceRock.btk";
}

extern void *__nw__FUl(unsigned long);
extern void getSDevice__12JUTAssertionFv();
extern void showAssert_f__12JUTAssertionFUlPCciPCce();
extern void OSPanic();
extern char _SDA_BASE_[];
extern int __vt__14AnmControlBase;
extern int __vt__33TAnmControlBase;
extern int __vt__15AnmControlTrans;
extern void __ct__10JSUPtrLinkFPv(void *, void *);
extern void registration__15AnmControlTransFUcP15J3DAnmTransformP10J3DMtxCalc(void *, unsigned char, J3DAnmTransform *, J3DMtxCalc *);
extern void InitRegistrationMat__13AnmControllerFRC10MatAnmInfoP7ExModel(void *, const MatAnmInfo &, ExModel *);
extern void initFrameCtrl__13J3DAnmObjBaseFP10J3DAnmBase(void *, J3DAnmBase *);
const char ice_str_RaceMgr[] = "RaceMgr.h";
const char ice_str_range[] = "range over: %d <= index=%d < %d";
const char ice_str_Halt[] = "Halt";
const char ice_str_AnmController[] = "AnmController.h";
const char ice_str_rangeAnm[] = "range over: %d <= AnmNo=%d < %d";
extern J3DAnmTransform *sBckAnmTrans__14J3DEfctIceRock[];
__declspec(section ".sdata") static J3DAnmTransform *bckTbl[1];
__declspec(section ".sdata") static J3DMtxCalc *calcTbl[1];
extern J3DMtxCalc *sBckMtxCalc__14J3DEfctIceRock[];
extern J3DAnmTextureSRTKey *sTexSRTKey__14J3DEfctIceRock;

asm void J3DEfctIceRock::onlyinit() {
    nofralloc
        stwu r1, -32(r1)
        mflr r0
        stw r0, 36(r1)
        stmw r28, 16(r1)
        mr r31, r3
        li r3, 32
        bl __nw__FUl
        mr. r29, r3
        beq L_oi_nonew
        lis r3, __vt__14AnmControlBase@ha
        li r0, 0
        addi r3, r3, __vt__14AnmControlBase@l
        mr r4, r29
        stw r3, 0(r29)
        addi r3, r29, 8
        stb r0, 4(r29)
        stb r0, 5(r29)
        stb r0, 6(r29)
        bl __ct__10JSUPtrLinkFPv
        lis r4, __vt__33TAnmControlBase@ha
        lis r3, __vt__15AnmControlTrans@ha
        addi r0, r4, __vt__33TAnmControlBase@l
        li r4, 0
        stw r0, 0(r29)
        addi r0, r3, __vt__15AnmControlTrans@l
        stw r4, 24(r29)
        stw r0, 0(r29)
L_oi_nonew:
        stw r29, 224(r31)
        addi r5, r31, 4
        li r4, 1
        lwz r3, 224(r31)
        lwz r12, 0(r3)
        lwz r12, 8(r12)
        mtctr r12
        bctrl
        li r28, 0
        li r29, bckTbl@l
        li r30, calcTbl@l
        b L_oi_loopcheck
L_oi_loop:
        rlwinm r0, r28, 2, 22, 29
        lwz r3, 224(r31)
        lwzx r5, r29, r0
        mr r4, r28
        lwzx r6, r30, r0
        bl registration__15AnmControlTransFUcP15J3DAnmTransformP10J3DMtxCalc
        addi r28, r28, 1
L_oi_loopcheck:
        clrlwi r0, r28, 24
        cmplwi r0, 1
        blt L_oi_loop
        li r6, 0
        li r0, 1
        stb r6, 9(r1)
        addi r3, r31, 220
        addi r4, r1, 8
        addi r5, r31, 4
        stb r6, 11(r1)
        stb r6, 10(r1)
        stb r6, 8(r1)
        stb r0, 9(r1)
        bl InitRegistrationMat__13AnmControllerFRC10MatAnmInfoP7ExModel
        lwz r3, 236(r31)
        lwz r29, sTexSRTKey__14J3DEfctIceRock
        lwz r3, 4(r3)
        lbz r30, 5(r3)
        cmplwi r30, 0
        bgt L_oi_ok
        bl getSDevice__12JUTAssertionFv
        lis r4, ice_str_AnmController@ha
        lis r5, ice_str_rangeAnm@ha
        addi r6, r5, ice_str_rangeAnm@l
        mr r9, r30
        addi r4, r4, ice_str_AnmController@l
        li r5, 321
        li r7, 0
        li r8, 0
        crclr 4*cr1+eq
        bl showAssert_f__12JUTAssertionFUlPCciPCce
        lis r3, ice_str_AnmController@ha
        li r4, 321
        addi r3, r3, ice_str_AnmController@l
        li r5, ice_str_Halt@l
        crclr 4*cr1+eq
        bl OSPanic
L_oi_ok:
        lwz r3, 236(r31)
        lwz r3, 4(r3)
        lwz r3, 24(r3)
        stw r29, 28(r3)
        lwz r4, 28(r3)
        bl initFrameCtrl__13J3DAnmObjBaseFP10J3DAnmBase
        lwz r3, 236(r31)
        li r0, 0
        lwz r3, 4(r3)
        stb r0, 6(r3)
        lmw r28, 16(r1)
        lwz r0, 36(r1)
        mtlr r0
        addi r1, r1, 32
        blr
}

void J3DEfctIceRock::init() {
    *(u32 *)_90 = 0;
    mAnmCtrl.Reset();
    _f2 = 0;
    _f0 = 0;
}

void J3DEfctIceRock::calc() {
    mAnmCtrl.FrameProc();
    setModelMtx();
}

void J3DEfctIceRock::setTrigger() {
    *(u32 *)_90 = 1;
    JGeometry::TVec3f pos;
    ObjUtility::getKartPos(mKartNo, &pos);

    for (u8 i = 0; i < 3; i++) {
        JPABaseEmitter *emt = GetJPAMgr()->createEmt(sJPAFileName[i], pos);
        if (emt != nullptr && !TstEmitterModeNotAvailableFlg(mKartNo, emt)) {
#line 170 "RaceMgr.h"
            JUT_MINMAX_ASSERT(0, mKartNo, 8);
        }
    }
}

void J3DEfctIceRock::update() {
    mAnmCtrl.SetAnimation();
    mModel.update(0);
}

extern RaceMgr *sRaceManager__7RaceMgr;
extern const f32 ice_flt_zero;
const f32 ice_flt_zero = 0.0f;
extern void getSDevice__12JUTAssertionFv();
extern void showAssert_f__12JUTAssertionFUlPCciPCce();
extern void OSPanic();
extern char _SDA_BASE_[];
extern void PSMTXScale();
extern void PSMTXTrans();
extern void PSMTXConcat();
extern void getKartBodyMtx__10ObjUtilityFi();
extern void setBaseTRMtx__7ExModelFPA4_f();

asm void J3DEfctIceRock::setModelMtx() {
    nofralloc
        stwu r1, -176(r1)
        mflr r0
        stw r0, 180(r1)
        li r0, 0
        stmw r29, 164(r1)
        mr r31, r3
        lwz r30, 216(r3)
        lwz r29, sRaceManager__7RaceMgr
        cmpwi r30, 0
        blt L_smm_skip
        cmpwi r30, 8
        bge L_smm_skip
        li r0, 1
L_smm_skip:
        clrlwi. r0, r0, 24
        bne L_smm_noassert
        bl getSDevice__12JUTAssertionFv
        lis r4, ice_str_RaceMgr@ha
        lis r5, ice_str_range@ha
        addi r6, r5, ice_str_range@l
        mr r8, r30
        addi r4, r4, ice_str_RaceMgr@l
        li r5, 170
        li r7, 0
        li r9, 8
        crclr 4*cr1+eq
        bl showAssert_f__12JUTAssertionFUlPCciPCce
        lis r3, ice_str_RaceMgr@ha
        li r4, 170
        addi r3, r3, ice_str_RaceMgr@l
        li r5, ice_str_Halt@l
        crclr 4*cr1+eq
        bl OSPanic
L_smm_noassert:
        mulli r3, r30, 24
        lwz r4, 56(r29)
        addi r0, r3, 48
        lwzx r3, r4, r0
        cmplwi r3, 0
        beq L_smm_null
        lwz r0, 0(r3)
        b L_smm_have
L_smm_null:
        li r0, 255
L_smm_have:
        mulli r5, r0, 20
        lis r4, sIceOffsetData@ha
        addi r3, r1, 104
        addi r0, r4, sIceOffsetData@l
        add r29, r0, r5
        lfs f1, 0(r29)
        lfs f2, 4(r29)
        lfs f3, 8(r29)
        bl PSMTXScale
        lfs f1, ice_flt_zero
        addi r3, r1, 56
        lfs f2, 12(r29)
        lfs f3, 16(r29)
        bl PSMTXTrans
        addi r3, r1, 56
        addi r4, r1, 104
        addi r5, r1, 8
        bl PSMTXConcat
        lwz r3, 216(r31)
        bl getKartBodyMtx__10ObjUtilityFi
        addi r4, r1, 8
        mr r5, r4
        bl PSMTXConcat
        addi r3, r31, 4
        addi r4, r1, 8
        bl setBaseTRMtx__7ExModelFPA4_f
        lmw r29, 164(r1)
        lwz r0, 180(r1)
        mtlr r0
        addi r1, r1, 176
        blr
}


inline J3DEfctIceRock::~J3DEfctIceRock() {}

