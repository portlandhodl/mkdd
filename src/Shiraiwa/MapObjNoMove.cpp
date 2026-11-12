#include "Sato/GeographyObj.h"
#include "JSystem/JAudio/JASFakeMatch2.h"

struct ObjBmdEntry
{
    u32 mKind;
    const char *mName;
};

class TMapObjNoMove : public GeographyObj
{
public:
    TMapObjNoMove(const CrsData::SObject &);
    virtual ~TMapObjNoMove();
    virtual void reset();
    virtual const char *getBmdFileName();
    virtual void calc();

    static ObjBmdEntry sObjList[4];
};

ObjBmdEntry TMapObjNoMove::sObjList[4] = {
    { 0x1199, "/Objects/DonkyWood.bmd" },
    { 0x1070, "/Objects/Wl_Dokan1.bmd" },
    { 0x13f3, "/Objects/Lights1.bmd" },
    { 0x138c, "/Objects/DeBalloon1.bmd" },
};

TMapObjNoMove::TMapObjNoMove(const CrsData::SObject &object) : GeographyObj(object)
{
    setObjFlagSimpleDraw();
}

TMapObjNoMove::~TMapObjNoMove()
{
}

void TMapObjNoMove::reset()
{
    resetObject();
    clrObjFlagCheckGeoHitting();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();
}

const char *TMapObjNoMove::getBmdFileName()
{
    for (u32 i = 0; i < ARRAY_SIZE(sObjList); i++) {
        if (mObjData->objID == sObjList[i].mKind)
            return sObjList[i].mName;
    }
    return nullptr;
}

void TMapObjNoMove::calc()
{
}
