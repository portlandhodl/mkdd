#include "Shiraiwa/Objects/MapObjPeachTree.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

GeoAnmTableEntry TMapObjPeachTree::sBckAnmTable[1] = {
    0, "PeachTree1_wait.bck"
};

TMapObjPeachTree::TMapObjPeachTree(const CrsData::SObject &obj) : GeoMarioTree1(obj) {}

TMapObjPeachTree::~TMapObjPeachTree() {}

const char *TMapObjPeachTree::getBmdFileName() {
    static const char *cTreeBmdName = "PeachTree1.bmd";
    return cTreeBmdName;
}

GeoAnmTableEntry *TMapObjPeachTree::getAnmTbl() {
    if (availBckAnm()) {
        return sBckAnmTable;
    }
    return nullptr;
}

void TMapObjPeachTree::createColModel(J3DModelData *modelData) {
    GeographyObj::createBoundsCylinder(modelData, 210.0f, 500.0f);
}

u16 TMapObjPeachTree::getSizeAnmTbl() {
    return sizeof(sBckAnmTable);
}

GeoAnmTableEntry TMapObjPeachTree2::sBckAnmTable[1] = {
    0, "PeachTree2_wait.bck"
};

TMapObjPeachTree2::TMapObjPeachTree2(const CrsData::SObject &obj) : GeoMarioTree1(obj) {}

TMapObjPeachTree2::~TMapObjPeachTree2() {}

const char *TMapObjPeachTree2::getBmdFileName() {
    static const char *cTreeBmdName = "PeachTree2.bmd";
    return cTreeBmdName;
}

GeoAnmTableEntry *TMapObjPeachTree2::getAnmTbl() {
    if (availBckAnm()) {
        return sBckAnmTable;
    }
    return nullptr;
}

u16 TMapObjPeachTree2::getSizeAnmTbl() {
    return sizeof(sBckAnmTable);
}

void TMapObjPeachTree2::createColModel(J3DModelData *modelData) {
    GeographyObj::createBoundsCylinder(modelData, 110.0f, 500.0f);
}
