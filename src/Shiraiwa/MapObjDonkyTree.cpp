#include "Shiraiwa/MapObjDonkyTree.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
#include "Sato/AnmController.h"

TMapObjDonkyTree::TMapObjDonkyTree(const CrsData::SObject &object) : GeoMarioTree1(object) {}

TMapObjDonkyTree::~TMapObjDonkyTree() {}

const char *TMapObjDonkyTree::getBmdFileName() {
    static const char *cTreeBmdName = "DonkyTree1.bmd";
    return cTreeBmdName;
}

GeoAnmTableEntry *TMapObjDonkyTree::getAnmTbl() {
    if (availBckAnm()) {
        return sBckAnmTable;
    }
    return nullptr;
}

u16 TMapObjDonkyTree::getSizeAnmTbl() {
    return sizeof(sBckAnmTable);
}

GeoAnmTableEntry TMapObjDonkyTree::sBckAnmTable[1] = {
    0, "DonkyTree1_wait.bck"
};

TMapObjDinoTree::TMapObjDinoTree(const CrsData::SObject &object) : GeoMarioTree1(object) {}

TMapObjDinoTree::~TMapObjDinoTree() {}

void TMapObjDinoTree::reset() {
    GeographyObj::resetObject();
    _58 = 0;
    mCheckKartHitFlags = 0;
    mGeoObjFlag &= ~2;
    if (getAnmCtrl()) {
        J3DFrameCtrl *ctrl = getAnmCtrl()->getFrameCtrl(0);
        ctrl->setAttribute(2);
        ctrl->setRate(((getGeoRnd()->get_ufloat_1() * 2 - 1) * 0.3f + 1.0f));
    }
}

const char *TMapObjDinoTree::getBmdFileName() {
    static const char *cTreeBmdName = "DinoTree1.bmd";
    return cTreeBmdName;
}

GeoAnmTableEntry *TMapObjDinoTree::getAnmTbl() {
    if (availBckAnm()) {
        return sBckAnmTable;
    }
    return nullptr;
}

u16 TMapObjDinoTree::getSizeAnmTbl() {
    return sizeof(sBckAnmTable);
}

GeoAnmTableEntry TMapObjDinoTree::sBckAnmTable[1] = {
    0, "DinoTree1_wait.bck"
};

TMapObjDesertTree::TMapObjDesertTree(const CrsData::SObject &object) : GeoMarioTree1(object) {
    mObjFlag &= ~0x100;
}

TMapObjDesertTree::~TMapObjDesertTree() {}

void TMapObjDesertTree::reset() {
    GeographyObj::resetObject();
    _58 = 1;
    mCheckKartHitFlags = -1;
    mGeoObjFlag |= 2;
}

const char *TMapObjDesertTree::getBmdFileName() {
    static const char *cTreeBmdName = "DesertTree1.bmd";
    return cTreeBmdName;
}

GeoAnmTableEntry *TMapObjDesertTree::getAnmTbl() {
    return nullptr;
}

u16 TMapObjDesertTree::getSizeAnmTbl() {
    return 0;
}

void TMapObjDesertTree::createColModel(J3DModelData *mdlData) {
    createBoundsCylinder(mdlData, 210.0f, 700.0f);
}
