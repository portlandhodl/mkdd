#include "Kawano/driverMgr.h"
#include "JSystem/J3D/J3DJoint.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DModelLoader.h"
#include "JSystem/J3D/J3DShape.h"
#include "JSystem/J3D/J3DTypes.h"
#include "JSystem/JGeometry/Vec.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JUtility/JUTNameTab.h"
#include "Kaneshige/ExModel.h"
#include "Kaneshige/RaceMgr.h"
#include "Kaneshige/TexLODControl.h"
#include "Osako/ResMgr.h"
#include "dolphin/gx/GXEnum.h"
#include "types.h"
#include <stdio.h>
#include "mathHelper.h"

const char *DriverLoader::sModelFileNameTable[2] = {
    "driver.bmd", "driver_200.bmd"
};

const char *DriverLoader::sAnmFileNameTable[82] = {
    "dummy",
    "a_drive_a.bca",
    "a_drive_b.bca",
    "b_tandem_a.bca",
    "b_tandem_b.bca",
    "c_tandem.bca",
    "a2b_change.bca",
    "b2a_change.bca",
    "S_a2b_change.bca",
    "S_b2a_change.bca",
    "M_a2b_change.bca",
    "M_b2a_change.bca",
    "L_a2b_change.bca",
    "L_b2a_change.bca",
    "b2c_change.bca",
    "c2b_change_a.bca",
    "b_wait_a.bca",
    "b_wait_b.bca",
    "c_wait_b.bca",
    "b_throw_a1.bca",
    "b_throw_a2.bca",
    "b_throw_b1.bca",
    "b_throw_b2.bca",
    "c_throw_a1.bca",
    "c_throw_a2.bca",
    "c_throw_b1.bca",
    "c_throw_b2.bca",
    "b_attack_la.bca",
    "b_attack_lb.bca",
    "b_attack_ra.bca",
    "b_attack_rb.bca",
    "c_attack_lb.bca",
    "c_attack_rb.bca",
    "b_jump_a.bca",
    "b_jump_b.bca",
    "c_jump_b.bca",
    "b_sit_a.bca",
    "b_sit_b.bca",
    "c_sit_b.bca",
    "d_wait_a.bca",
    "d_run.bca",
    "d_normal.bca",
    "d_good.bca",
    "d_bad.bca",
    "b_damage_a_a.bca",
    "b_damage_b_a.bca",
    "c_damage_a.bca",
    "c_damage_b.bca",
    "b_dash_a.bca",
    "b_dash_b.bca",
    "c_dash_b.bca",
    "b_tandem_ar.bca",
    "b_tandem_as.bca",
    "b_tandem_al.bca",
    "b_tandem_br.bca",
    "b_tandem_bs.bca",
    "b_tandem_bl.bca",
    "c_tandem_br.bca",
    "c_tandem_bs.bca",
    "c_tandem_bl.bca",
    "b_denger_l.bca",
    "b_denger_r.bca",
    "b_dwait_a.bca",
    "b_dwait_b.bca",
    "a_damage.bca",
    "a_piyo.bca",
    "b2a_change_p.bca",
    "a2b_change_p.bca",
    "b_fire.bca",
    "c_fire.bca",
    "e_1st_b.bca",
    "e_good_a.bca",
    "e_good_b.bca",
    "e_good_b2.bca",
    "e_bad_a.bca",
    "e_bad_b1.bca",
    "e_bad_b2.bca",
    "b_gut.bca",
    "c_gut.bca",
    "b_appeal.bca",
    "all_jug_a.bca",
    "all_speak.bca"
};

const u8 DriverLoader::mscAnm2SoundList[82] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x01, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x09, 0x0A, 0x05, 0x06, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x08, 0x00, 
    0x00, 0x00
};

const char *DriverLoader::sAnmSoundFileNameTable[12] = {
    "dummy", "d_run.bas", "d_normal.bas", "d_good.bas", 
    "d_bad.bas","e_1st_b.bas", "e_good_a.bas", "b_gut.bas", 
    "c_gut.bas","b_fire.bas", "c_fire.bas","d_runxxx.bas", 

};

const char *DriverLoader::mscpaDriverName[20] = {
    "BabyMario","BabyLuigi",
    "Patapata", "Nokonoko",
    "Peach", "Daisy",
    "Mario", "Luigi",
    "Wario", "Waluigi",
    "Yoshi", "Catherine",
    "DK", "DKjr",
    "Koopa", "KoopaJr",
    "Kinopio", "Kinopico",
    "KingTeresa", "BossPakkun"
};

const char *DriverLoader::mscpaDriverFolderName2[20] = {
    "cmn_Baby", "cmn_Baby",
    "cmn_Zako", "cmn_Zako",
    "cmn_Hime", "cmn_Hime",
    "cmn_Hige", "cmn_Hige",
    "cmn_Waru", "cmn_Waru",
    "cmn_Liz", "cmn_Liz",
    "cmn_DK", "cmn_DK",
    "cmn_Koopa", "cmn_Koopa",
    "cmn_Baby", "cmn_Baby",
    "cmn_Hige", "cmn_Waru",
};

const char *DriverLoader::sDriverFolderName3 = "cmn_Sound";

const u8 DriverLoader::msAnmSoundMRAMList[1] = {11};

const u8 DriverLoader::msAnmMRAMList[11] = {
    0x50, 0x51, 0x34, 0x37, 0x3A, 0x35, 0x38, 0x3B, 
    0x33, 0x36, 0x39, 
};

const char DriverLoader::mscaaMRAMAnmFileName[11][32] = {
    "all_juggle_a.bca",
    "all_speak.bca",
    "b_tandem_as.bca",
    "b_tandem_bs.bca",
    "c_tandem_bs.bca",
    "b_tandem_al.bca",
    "b_tandem_bl.bca",
    "c_tandem_bl.bca",
    "b_tandem_ar.bca",
    "b_tandem_br.bca",
    "c_tandem_br.bca",
};

const char DriverLoader::mscaaMRAMAnmSoundFileName[1][32] = {
    {"d_runxxx.bas"}
};

const char *DriverLoader::mscaaJntName[27] = {
    "skl_root", "spin",
    "leg_l1","leg_l2",
    "ankle_l1", "foot_l_pos",
    "leg_r1","leg_r2",
    "ankle_r1", "foot_r_pos",
    "arm_l1", "arm_l2",
    "wrist_l1", "hand_l_pos",
    "arm_r1", "arm_r2",
    "wrist_r1", "hand_r_pos",
    "face_1", "mouth_1",
    "skirt_1", "skirt_pos",
    "d_hair_1", "d_hair_2",
    "p_hair_1", "p_hair_2",
    "tie_1"
};

const u16 DriverLoader::mscaBoneJnt[4] = {
    11, 12, 3, 4
};

DriverLoader::DriverLoader(int dataID) {
    mDataID = dataID;

    for (int i = 0; i < 2; i++) {
        J3DModelData *mdlData = createModelData(dataID, i);
        mMdlData[i] = mdlData;
        if (i != 0) continue;

        for (int j = 0; j < 27; j++) {
            int idx = mdlData->getJointName()->getIndex(mscaaJntName[j]);
            if (idx != -1) {
                _c[j] = idx;
            }
            else {
                _c[j] = 0xffff;
            }
        }

        for (int j = 0; j < 4; j++) {
            u16 jntNo = _c[mscaBoneJnt[j]];
            if (jntNo == 0xffff) {
                _44[j] = 0.0f;
            }
            else {
                J3DTransformInfo &transformInfo = mdlData->getJointNodePointer(jntNo)->getTransformInfo();
                JGeometry::TVec3f t;
                t.set(transformInfo.mTranslation.x, transformInfo.mTranslation.y, transformInfo.mTranslation.z);
                _44[j] = t.length();
            }
        }
    }
    DriverManager::ptr();
}

int DriverLoader::convAnmIDToKeepID(DriverLoader::AnmEnum anm) {
    int ret = -1;

    for (int i = 0; i < 11; i++) {
        if (anm == msAnmMRAMList[i]) {
            ret = i;
            break;
        }
    }

    return ret;
}

void *DriverLoader::getAnmFile(int keepID) {
#line 494
    JUT_MINMAX_ASSERT(0, keepID, 11);

    char buf[64];
    snprintf(buf, sizeof(buf), "/Driver/%s/%s", getDriverName(mDataID), mscaaMRAMAnmFileName[keepID]);
    void *anmFile = ResMgr::getPtr(ResMgr::mcArcMRAM, buf);
    if (!anmFile) {
        snprintf(buf, sizeof(buf), "/Driver/%s/%s", getDriverFolderName2(mDataID), mscaaMRAMAnmFileName[keepID]);
        anmFile = ResMgr::getPtr(ResMgr::mcArcMRAM, buf);
    }
    return anmFile;
}

int DriverLoader::convSndIDToKeepID(DriverLoader::AnmSoundEnum anm) {
    int ret = -1;
    for (int i = 0; i < 1; i++) {
        if (anm == msAnmSoundMRAMList[i]) {
            ret = 0;
            break;
        }
    }

    return ret;
}

void *DriverLoader::getSoundDataFile(int keepID) {
#line 550
    JUT_MINMAX_ASSERT(0, keepID, 1);

    char buf[64];
    snprintf(buf, sizeof(buf), "/Driver/%s/%s", getDriverName(mDataID), mscaaMRAMAnmSoundFileName[keepID]);
    void *sndFile = ResMgr::getPtr(ResMgr::mcArcMRAM, buf);
    if (!sndFile) {
        snprintf(buf, sizeof(buf), "/Driver/%s/%s", sDriverFolderName3, mscaaMRAMAnmSoundFileName[keepID]);
        sndFile = ResMgr::getPtr(ResMgr::mcArcMRAM, buf);
    }
    return sndFile;
}

void *DriverLoader::getBmdFile(int dataID, int level) {
    char buf[64];
    snprintf(buf,sizeof(buf),"/Driver/%s/%s", mscpaDriverName[dataID], sModelFileNameTable[level]);
    return ResMgr::getPtr(ResMgr::mcArcMRAM, buf);
}

J3DModelData *DriverLoader::createModelData(int dataID, int level) {
#line 643
    JUT_MINMAX_ASSERT(0, dataID, 20);
    JUT_MINMAX_ASSERT(0, level, 2);
    void *bmd = getBmdFile(dataID, level);
    J3DModelData *mdlData = J3DModelLoaderDataBase::load(bmd, 0x11020030);
    ExModel::patchModelData(mdlData);
    ExModel::setLightMask(mdlData, GX_LIGHT1);

    for (u16 i = 0; i < mdlData->getShapeNum(); i++) {
        J3DShape *shape = mdlData->getShapeNodePointer(i);
        shape->setTexMtxLoadType(0x2000);
    }

    ExModel::setLODBias(mdlData, TexLODControl::getCharacterLODBias());
    RCMGetCourse()->setFogInfo(mdlData);
    mdlData->newSharedDisplayList(0x40000);
    return mdlData;
}

DriverManager *DriverManager::mspDriverManager;

DriverManager::DriverManager() {
    _2c = 0;
    char buf[64];
    snprintf(buf, sizeof(buf), "/Driver/Mario/driver.bmd");
    void *bmd = ResMgr::getPtr(ResMgr::mcArcMRAM, buf);

    mMdlData = J3DModelLoaderDataBase::load(bmd, 0x210010);
    ExModel::patchModelData(mMdlData);
    mBlendTimes[0] = 10;
    mBlendTimes[1] = 10;
    mBlendTimes[2] = 15;
    mBlendTimes[3] = 10;
    mBlendTimes[4] = 10;
    mBlendTimes[5] = 30;
    mBlendTimes[6] = 10;
    mBlendTimes[7] = 10;
    mBlendTimes[8] = 10;
    mBlendTimes[9] = 5;
    mBlendTimes[10] = 10;
    mBlendTimes[11] = 5;
    mBlendTimes[12] = 10;
    mBlendTimes[13] = 10;
    mBlendTimes[14] = 10;
    mBlendTimes[15] = 5;
    mBlendTimes[16] = 5;
    mBlendTimes[17] = 5;
    mBlendTimes[18] = 10;
    mBlendTimes[19] = 10;
    mBlendTimes[20] = 16;
    mBlendTimes[21] = 10;
    mBlendTimes[22] = 10;
    mBlendTimes[23] = 18;
    mBlendTimes[24] = 7;
    mBlendTimes[25] = 5;
    mBlendTimes[26] = 8;
    mBlendTimes[27] = 28;
    mBlendTimes[28] = 10;
    mBlendTimes[29] = 25;
    mBlendTimes[30] = 10;
    mBlendTimes[31] = 10;
    mBlendTimes[32] = 10;
    mBlendTimes[33] = 10;
    mBlendTimes[34] = 10;
    mBlendTimes[35] = 10;
    mBlendTimes[36] = 10;
    mBlendTimes[37] = 10;
    mBlendTimes[38] = 10;
    _5b = 30;
    _5c = 32;
    _5d = 10;
    _5e = 5;
    _5f = 5;
    _838 = 0;
    _83a = 60;
    _83c = 60;
    _18 = 1.0;
    _1c = 1.0;
    _20 = 2.0;
    _24 = 1;
    _28 = 1.0;
    _840 = 0.0;
    _848 = 0;
    mMdlCnt = 0;

    for (int i = 0; i < 20; i++) {
        char buf[64];
        _60[i][0] = 0;
        for (int j = 1; j < 82; j++) {
            snprintf(buf, sizeof(buf), "DriverAnm/%s/%s", DriverLoader::mscpaDriverName[i], DriverLoader::sAnmFileNameTable[j]);
            if (ResMgr::findResource(ResMgr::mcArcARAM, buf)) {
                _60[i][j] = 0;
            }
            else {
                _60[i][j] = 1;
            }
        }

        snprintf(buf, sizeof(buf), "DriverAnm/%s/e_good_b2.bca", DriverLoader::mscpaDriverName[i]);
        if (ResMgr::findResource(ResMgr::mcArcARAM, buf)) {
            _7e0[i] = 1;
        }
        else {
            _7e0[i] = 0;
        }

        _6c8[i][0] = 0;
        for (int j = 1; j < 12; j++) {
            snprintf(buf, sizeof(buf), "DriverAnm/%s/%s", DriverLoader::mscpaDriverName[i], DriverLoader::sAnmSoundFileNameTable[j]);
            if (ResMgr::findResource(ResMgr::mcArcARAM, buf)) {
                _6c8[i][j] = 0;
            }
            else {
                snprintf(buf, sizeof(buf), "DriverAnm/%s/%s", DriverLoader::mscpaDriverFolderName2[i], DriverLoader::sAnmSoundFileNameTable[j]);
                if (ResMgr::findResource(ResMgr::mcArcARAM, buf)) {
                    _6c8[i][j] = 1;
                }
                else {
                    _6c8[i][j] = 2;
                }
            }
        }

        snprintf(buf, sizeof(buf), "Driver/%s/speak.btp", DriverLoader::mscpaDriverName[i]);
        if (ResMgr::findResource(ResMgr::mcArcMRAM, buf)) {
            _7b8[i] = 1;
        }
        else {
            _7b8[i] = 0;
        }

        snprintf(buf, sizeof(buf), "Driver/%s/all_speak.bca", DriverLoader::mscpaDriverName[i]);
        if (ResMgr::findResource(ResMgr::mcArcMRAM, buf)) {
            _7cc[i] = 2;
        }
        else {
            snprintf(buf, sizeof(buf), "Driver/%s/all_speak.bca", DriverLoader::mscpaDriverFolderName2[i]);
            if (ResMgr::findResource(ResMgr::mcArcMRAM, buf)) {
                _7cc[i] = 1;
            }
            else {
                _7cc[i] = 0;
            }
        }
    }
}

DriverManager::~DriverManager() {
    mspDriverManager = nullptr;
}

DriverManager *DriverManager::ptr() {
    if (!mspDriverManager)
        mspDriverManager = new DriverManager();
    return mspDriverManager;
}

const char *DriverManager::getFolderName_Sound(int dataID, int keepID) {
    switch (_6c8[dataID][keepID]) {
    case 0: {
        return DriverLoader::getDriverName(dataID);
    }
    case 1: {
        return DriverLoader::getDriverFolderName2(dataID);
    }
    case 2:
    default:
        return DriverLoader::sDriverFolderName3;
    }
}

void DriverManager::SetDriverModelPtr(DriverModel *mdl) {
    mMdls[mMdlCnt] = mdl;
    mMdlCnt++;
}

bool DriverManager::IsAllARAMDMAFinished() {
    // UNUSED
}

void DriverManager::CheckARAMDMA(f64, int) {
    // UNUSED
}

#include "JSystem/JAudio/JASFakeMatch2.h"
