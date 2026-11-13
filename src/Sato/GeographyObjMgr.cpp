#include <stdio.h>
#include <string.h>

#include "dolphin/mtx.h"
#include "dolphin/os.h"

#include "JSystem/J3D/J3DAnmLoader.h"
#include "JSystem/J3D/J3DModelLoader.h"
#include "JSystem/J3D/J3DSys.h"
#include "JSystem/J3D/J3DTransform.h"
#include "JSystem/JAudio/JASFakeMatch.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
#include "JSystem/JUtility/JUTAssert.h"

#include "Kaneshige/RaceMgr.h"
#include "Kaneshige/TexLODControl.h"
#include "Kaneshige/Objects/GeoAirJet.h"
#include "Kaneshige/Objects/GeoBus.h"
#include "Kaneshige/Objects/GeoCannon.h"
#include "Kaneshige/Objects/GeoCarShadow.h"
#include "Kaneshige/Objects/GeoDkCloud.h"
#include "Kaneshige/Objects/GeoHanaBody.h"
#include "Kaneshige/Objects/GeoHanaCar.h"
#include "Kaneshige/Objects/GeoItemCar.h"
#include "Kaneshige/Objects/GeoItemGen.h"
#include "Kaneshige/Objects/GeoKinoCar.h"
#include "Kaneshige/Objects/GeoKinojii.h"
#include "Kaneshige/Objects/GeoKuriCar.h"
#include "Kaneshige/Objects/GeoKuribo.h"
#include "Kaneshige/Objects/GeoNormCar.h"
#include "Kaneshige/Objects/GeoPuller.h"
#include "Kaneshige/Objects/GeoRabbitMark.h"
#include "Kaneshige/Objects/GeoShimmer.h"
#include "Kaneshige/SimpleDrawer.h"
#include "Kaneshige/Objects/GeoShine.h"
#include "Kaneshige/Objects/GeoSignal.h"
#include "Kaneshige/Objects/GeoSoundGen.h"
#include "Kaneshige/Objects/GeoSplash.h"
#include "Kaneshige/Objects/GeoStartLine.h"
#include "Kaneshige/Objects/GeoTideSensor.h"
#include "Kaneshige/Objects/GeoTruck.h"
#include "Kaneshige/Objects/GeoVision.h"
#include "Kaneshige/Objects/GeoWater.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/Objects/GeoItemBox.h"
#include "Sato/Objects/GeoJumpNarrow.h"
#include "Sato/Objects/GeoRFlower.h"
#include "Sato/Objects/GeoTree.h"
#include "Sato/Objects/MapObjDanceMonte.h"
#include "Sato/Objects/MapObjFireBall.h"
#include "Sato/Objects/MapObjMare.h"
#include "Sato/Objects/MapObjMareW.h"
#include "Sato/Objects/MapObjMashBalloon.h"
#include "Sato/Objects/MapObjMonte.h"
#include "Sato/Objects/MapObjNossie.h"
#include "Sato/Objects/MapObjSanbo.h"
#include "Sato/Objects/TTestString.h"
#include "Sato/Objects/MapObjPakkunCol.h"
#include "Sato/stMath.h"
#include "Shiraiwa/Balloon.h"
#include "Shiraiwa/JugemMain.h"
#include "Shiraiwa/JugemHeadItem.h"
#include "Shiraiwa/JugemItem.h"
#include "Shiraiwa/JugemRodItem.h"
#include "Shiraiwa/LensFlare.h"
#include "Shiraiwa/Objects/MapObjAntLion.h"
#include "Shiraiwa/Objects/MapObjAward.h"
#include "Shiraiwa/Objects/MapObjDemoObj.h"
#include "Shiraiwa/Objects/MapObjHanabi.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Shiraiwa/Objects/MapObjIceBlock.h"
#include "Shiraiwa/Objects/MapObjPool.h"
#include "Shiraiwa/Objects/MapObjWanwan.h"
#include "Shiraiwa/ShineArrow.h"
#include "Shiraiwa/Objects/MapObjGeyser.h"
#include "Shiraiwa/Objects/MapObjMoveItemBox.h"
#include "Shiraiwa/Objects/MapObjJetCoaster.h"
#include "Yamamoto/kartCtrl.h"

// Sato/ObjUtility.h is intentionally not included, because its declaration of
// getPtrMainLocaleArc has a wrong (void) return type while the target binary
// uses the returned pointer. Declare what this unit needs manually instead.
namespace ObjUtility
{
    void *getPtrMainArc(char *);
    void *getPtrCourseArc(char *);
    void *getPtrMainLocaleArc(char *);
    void chkAvailableEnemyByDistande(bool *, f32);
    Mtx &getKartBodyMtx(int);
} // namespace ObjUtility

static const f32 sDummy[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

GeographyObjManager *GeographyObjManager::gMgr = nullptr;

// ---------------------------------------------------------------------------
// Stub classes for objects whose headers do not exist yet.
// Sizes/ctor signatures verified against the target translation unit.
// ---------------------------------------------------------------------------

class TMapObjDossun : public GeographyObj
{
public:
    TMapObjDossun(const CrsData::SObject &);
    virtual ~TMapObjDossun() {}
    void calc() {}

private:
    u8 _stub[0x8c]; // 0x1d8
};

class GeoBombCar : public GeographyObj // NOTE: real header exists but pulls in Sato/ObjUtility.h (wrong getPtrMainLocaleArc decl)
{
public:
    GeoBombCar(const CrsData::SObject &);
    virtual ~GeoBombCar() {}
    void calc() {}

private:
    u8 _stub[0x154]; // 0x2a0
};

class TMapObjDonkyRock : public GeographyObj
{
public:
    TMapObjDonkyRock();
    virtual ~TMapObjDonkyRock() {}
    void calc() {}

private:
    u8 _stub[0x64]; // 0x1b0
};

class TMapObjDonkyTree : public GeographyObj
{
public:
    TMapObjDonkyTree(const CrsData::SObject &);
    virtual ~TMapObjDonkyTree() {}
    void calc() {}
};

class TMapObjFireBar : public GeographyObj
{
public:
    TMapObjFireBar(const CrsData::SObject &, u32 = 0x106a);
    virtual ~TMapObjFireBar() {}
    void calc() {}

private:
    u8 _stub[0x48]; // 0x194
};

class TMapObjPoihana : public GeographyObj
{
public:
    TMapObjPoihana(const CrsData::SObject &);
    virtual ~TMapObjPoihana() {}
    void calc() {}

private:
    u8 _stub[0x128]; // 0x274
};

class TMapObjWall : public GeographyObj
{
public:
    TMapObjWall(const CrsData::SObject &);
    virtual ~TMapObjWall() {}
    void calc() {}

private:
    u8 _stub[0x14]; // 0x160
};

class TSMAButterfly : public GeographyObj
{
public:
    TSMAButterfly();
    virtual ~TSMAButterfly() {}
    void calc() {}

private:
    u8 _stub[0x30]; // 0x17c
};

class TSMADonkyBird : public GeographyObj
{
public:
    TSMADonkyBird();
    virtual ~TSMADonkyBird() {}
    void calc() {}

private:
    u8 _stub[0x54]; // 0x1a0
};

class TSMAKamome : public GeographyObj
{
public:
    TSMAKamome();
    virtual ~TSMAKamome() {}
    void calc() {}

private:
    u8 _stub[0x4]; // 0x150
};

class TMapObjPeachTree : public GeographyObj
{
public:
    TMapObjPeachTree(const CrsData::SObject &);
    virtual ~TMapObjPeachTree() {}
    void calc() {}
};

class TMapObjSnowRock : public GeographyObj
{
public:
    TMapObjSnowRock(const CrsData::SObject &);
    virtual ~TMapObjSnowRock() {}
    void calc() {}

private:
    u8 _stub[0x3c]; // 0x188
};

class TTestRollObj : public GeographyObj
{
public:
    TTestRollObj(const CrsData::SObject &);
    virtual ~TTestRollObj() {}
    void calc() {}

private:
    u8 _stub[0xc]; // 0x158
};

class TMapObjFountain : public GeographyObj
{
public:
    TMapObjFountain(const CrsData::SObject &);
    virtual ~TMapObjFountain() {}
    void calc() {}

private:
    u8 _stub[0x20]; // 0x16c
};

class TMapObjHeyho : public GeographyObj
{
public:
    TMapObjHeyho(const CrsData::SObject &);
    virtual ~TMapObjHeyho() {}
    void calc() {}

private:
    u8 _stub[0x108]; // 0x254
};

class TMapObjSubHeyho : public GeographyObj
{
public:
    TMapObjSubHeyho();
    virtual ~TMapObjSubHeyho() {}
    void calc() {}

private:
    u8 _stub[0x7c]; // 0x1c8
};

class TMapObjSanbo : public GeographyObj
{
public:
    TMapObjSanbo(const CrsData::SObject &);
    virtual ~TMapObjSanbo() {}
    void calc() {}

private:
    u8 _stub[0x9c]; // 0x1e8
};

class TMapObjWlArrow : public GeographyObj
{
public:
    TMapObjWlArrow(const CrsData::SObject &);
    virtual ~TMapObjWlArrow() {}
    void calc() {}

private:
    u8 _stub[0x20]; // 0x16c
};

class TMapObjSnowMan : public GeographyObj
{
public:
    TMapObjSnowMan(const CrsData::SObject &);
    virtual ~TMapObjSnowMan() {}
    void calc() {}

private:
    u8 _stub[0x24]; // 0x170
};

class TMapObjNoMove : public GeographyObj
{
public:
    TMapObjNoMove(const CrsData::SObject &);
    virtual ~TMapObjNoMove() {}
    void calc() {}
};

class TMapObjSun : public GeographyObj
{
public:
    TMapObjSun(const CrsData::SObject &);
    virtual ~TMapObjSun() {}
    void calc() {}

private:
    u8 _stub[0x20]; // 0x16c
};

class TMapObjBubble : public GeographyObj
{
public:
    TMapObjBubble(const CrsData::SObject &);
    virtual ~TMapObjBubble() {}
    void calc() {}

private:
    u8 _stub[0x50]; // 0x19c
};

class TMapObjDesun : public GeographyObj
{
public:
    TMapObjDesun(const CrsData::SObject &);
    virtual ~TMapObjDesun() {}
    void calc() {}
};

class TMapObjSandPillar : public GeographyObj
{
public:
    TMapObjSandPillar(const CrsData::SObject &);
    virtual ~TMapObjSandPillar() {}
    void calc() {}

private:
    u8 _stub[0x6c]; // 0x1b8
};

class TMapObjFerriswheel : public GeographyObj
{
public:
    TMapObjFerriswheel(const CrsData::SObject &);
    virtual ~TMapObjFerriswheel() {}
    void calc() {}

private:
    u8 _stub[0x90]; // 0x1dc
};

class TMapObjDeBalloon : public GeographyObj
{
public:
    TMapObjDeBalloon(const CrsData::SObject &);
    virtual ~TMapObjDeBalloon() {}
    void calc() {}

private:
    u8 _stub[0x24]; // 0x170
};

class TMapObjRollTable : public GeographyObj
{
public:
    TMapObjRollTable(const CrsData::SObject &);
    virtual ~TMapObjRollTable() {}
    void calc() {}

private:
    u8 _stub[0x2c]; // 0x178
};

class TMapObjMeteor : public GeographyObj
{
public:
    TMapObjMeteor(const CrsData::SObject &);
    virtual ~TMapObjMeteor() {}
    void calc() {}

private:
    u8 _stub[0x44]; // 0x190
};


class TMapObjPakkun : public GeographyObj
{
public:
    TMapObjPakkun(const CrsData::SObject &);
    virtual ~TMapObjPakkun() {}
    void calc() {}

private:
    u8 _stub[0xfc]; // 0x248
};

class TMapObjDinoTree : public GeographyObj
{
public:
    TMapObjDinoTree(const CrsData::SObject &);
    virtual ~TMapObjDinoTree() {}
    void calc() {}
};

class TMapObjDemoFountain : public GeographyObj
{
public:
    TMapObjDemoFountain(const CrsData::SObject &);
    virtual ~TMapObjDemoFountain() {}
    void calc() {}

private:
    u8 _stub[0x24]; // 0x170
};

class TMapObjKpGear : public GeographyObj
{
public:
    TMapObjKpGear(const CrsData::SObject &);
    virtual ~TMapObjKpGear() {}
    void calc() {}
};

class TMapObjKpFireBar : public GeographyObj
{
public:
    TMapObjKpFireBar(const CrsData::SObject &);
    virtual ~TMapObjKpFireBar() {}
    void calc() {}

private:
    u8 _stub[0x48]; // 0x194
};



class TMapObjPeachTree2 : public GeographyObj
{
public:
    TMapObjPeachTree2(const CrsData::SObject &);
    virtual ~TMapObjPeachTree2() {}
    void calc() {}
};

class TMapObjYoshiHeli : public GeographyObj
{
public:
    TMapObjYoshiHeli(const CrsData::SObject &);
    virtual ~TMapObjYoshiHeli() {}
    void calc() {}

private:
    u8 _stub[0x74]; // 0x1c0
};

class TMapObjDesertTree : public GeographyObj
{
public:
    TMapObjDesertTree(const CrsData::SObject &);
    virtual ~TMapObjDesertTree() {}
    void calc() {}
};

class TMapObjWluSearchLight : public GeographyObj
{
public:
    TMapObjWluSearchLight(const CrsData::SObject &);
    virtual ~TMapObjWluSearchLight() {}
    void calc() {}
};

class TMapObjVentilator : public GeographyObj
{
public:
    TMapObjVentilator(const CrsData::SObject &);
    virtual ~TMapObjVentilator() {}
    void calc() {}

private:
    u8 _stub[0x40]; // 0x18c
};

class TMapObjUFO : public GeographyObj
{
public:
    TMapObjUFO(const CrsData::SObject &);
    virtual ~TMapObjUFO() {}
    void calc() {}

private:
    u8 _stub[0x40]; // 0x18c
};

class TMapObjDonkyRockGen : public GeographyObj
{
public:
    TMapObjDonkyRockGen(const CrsData::SObject &);
    virtual ~TMapObjDonkyRockGen() {}
    void calc() {}

private:
    u8 _stub[0x10]; // 0x15c
};

class TMapObjFireCircle : public GeographyObj
{
public:
    TMapObjFireCircle(const CrsData::SObject &);
    virtual ~TMapObjFireCircle() {}
    void calc() {}

private:
    u8 _stub[0x4c]; // 0x198
};

class TSMAButterflyGen : public GeographyObj
{
public:
    TSMAButterflyGen(const CrsData::SObject &);
    virtual ~TSMAButterflyGen() {}
    void calc() {}

private:
    u8 _stub[0x1c]; // 0x168
};

class TSMADonkyBirdGen : public GeographyObj
{
public:
    TSMADonkyBirdGen(const CrsData::SObject &);
    virtual ~TSMADonkyBirdGen() {}
    void calc() {}

private:
    u8 _stub[0x14]; // 0x160
};

class TSMABoidBirdManager : public GeographyObj
{
public:
    TSMABoidBirdManager(const CrsData::SObject &);
    virtual ~TSMABoidBirdManager() {}
    void calc() {}

private:
    u8 _stub[0x10]; // 0x15c
};



class TDistSnow : public GeographyObj
{
public:
    TDistSnow(const CrsData::SObject &);
    virtual ~TDistSnow() {}
    void calc() {}

private:
    u8 _stub[0x2c]; // 0x178
};

class StringBridge : public ExGeographyObj
{
public:
    StringBridge(const CrsData::SObject &);
    virtual ~StringBridge() {}
    void calc() {}

private:
    u8 _stub[0x1a0]; // 0x310
};

// ---------------------------------------------------------------------------
// Object creation function tables
// ---------------------------------------------------------------------------

GeoFuncTable::GeoFuncTblEntry GeoFuncTable::GeoFuncTbl[] = {
    { 0x1, 0, &GeographyObj::NewS<GeoItemBox>, 0, 0 },
    { 0xc, 0, &GeographyObj::NewS<GeoDoubleItemBox>, 0, 0 },
    { 0xa, 0, &GeographyObj::New<GeoF_ItemBox>, 0, 0 },
    { 0x9, 0, &GeographyObj::NewS<GeoStartLine>, 0, 0 },
    { 0xe75, 0, &GeographyObj::New<GeoMarioTree1>, 0, 0 },
    { 0xe76, 0, &GeographyObj::New<GeoMarioKinoko1>, 0, 0 },
    { 0xe77, 0, &GeographyObj::New<GeoMarioFlower1>, 0, 0 },
    { 0x125d, 0, &GeographyObj::New<TMapObjDossun>, 0, 0 },
    { 0xfa1, 0, &GeographyObj::New<GeoNormCar>, 0, 0 },
    { 0x1003, 0, &GeographyObj::NewS<GeoCarShadow>, 0, 0 },
    { 0x1001, 0, &GeographyObj::NewS<GeoBombShadow>, 0, 0 },
    { 0x1002, 0, &GeographyObj::NewS<GeoKinoShadow>, 0, 0 },
    { 0x1000, 0, &GeographyObj::NewS<GeoHanaShadow>, 0, 0 },
    { 0xfff, 0, &GeographyObj::NewS<GeoHanaBodyShadow>, 0, 0 },
    { 0xfa3, 0, &GeographyObj::New<GeoTruck>, 0, 0 },
    { 0xfa2, 0, &GeographyObj::New<GeoBus>, 0, 0 },
    { 0xfa5, 0, &GeographyObj::New<GeoBombCar>, 0, 0 },
    { 0xfa6, 0, &GeographyObj::New<GeoKinoCar>, 0, 0 },
    { 0xfa7, 0, &GeographyObj::New<GeoKuriCar>, 0, 0 },
    { 0xfa8, 0, &GeographyObj::New<GeoItemCar>, 0, 0 },
    { 0xfa9, 0, &GeographyObj::New<GeoHanaCar>, 0, 0 },
    { 0xfaa, 0, &GeographyObj::NewS<GeoHanaBody>, 0, 0 },
    { 0xff, 0, &GeographyObj::NewS<TJugem>, 0, 0 },
    { 0x101, 0, &GeographyObj::NewS<TJugemVoidRod>, 0, 0 },
    { 0x100, 0, &GeographyObj::NewS<TJugemHeadHear>, 0, 0 },
    { 0x104, 0, &GeographyObj::NewS<TJugemRodBoard>, 0, 0 },
    { 0x105, 0, &GeographyObj::NewS<TJugemRodBoard2>, 0, 0 },
    { 0x106, 0, &GeographyObj::NewS<TJugemRodBoardRev>, 0, 0 },
    { 0x107, 0, &GeographyObj::NewS<TJugemRodSignal>, 0, 0 },
    { 0x108, 0, &GeographyObj::NewS<TJugemRodPukuPuku>, 0, 0 },
    { 0x102, 0, &GeographyObj::NewS<TJugemFlag>, 0, 0 },
    { 0x109, 0, &GeographyObj::NewS<TJugemHeadLamp>, 0, 0 },
    { 0xfa4, 0, &GeographyObj::New<GeoSignal>, 0, 0 },
    { 0x1195, 0, &GeographyObj::New<GeoCannon>, 0, 0 },
    { 0xe78, 0, &GeographyObj::New<TMapObjWanwan>, 0, 0 },
    { 0xe79, 0, &GeographyObj::NewS<TMapObjWanwanChain>, 0, 0 },
    { 0xe7a, 0, &GeographyObj::NewS<TMapObjWanwanPile>, 0, 0 },
    { 0xe7e, 0, &GeographyObj::New<TMapObjSkyShip>, 0, 0 },
    { 0x1197, 0, &GeographyObj::NewS<TMapObjDonkyRock>, 0, 0 },
    { 0x1198, 0, &GeographyObj::New<TMapObjDonkyTree>, 0, 0 },
    { 0x1199, 0, &GeographyObj::New<TMapObjNoMove>, 0, 0 },
    { 0x11a0, 0, &GeographyObj::New<GeoDkCloud>, 0, 0 },
    { 0xda4, 0, &GeographyObj::NewS<GeoWaterUp>, 0, 0 },
    { 0xda5, 0, &GeographyObj::NewS<GeoWaterUp2>, 0, 0 },
    { 0xd49, 0, &GeographyObj::New<GeoWater>, 0, 0 },
    { 0x1069, 0, &GeographyObj::New<TMapObjFireBar>, 0, 0 },
    { 0x106d, 0, &GeographyObj::New<GeoVision>, 0, 0 },
    { 0xd4a, 0, &GeographyObj::New<TMapObjPoihana>, 0, 0 },
    { 0x106e, 0, &GeographyObj::New<TMapObjWall>, 0, 0 },
    { 0x119b, 0, &GeographyObj::NewS<TSMAButterfly>, 0, 0 },
    { 0x119d, 0, &GeographyObj::NewS<TSMADonkyBird>, 0, 0 },
    { 0xd4c, 0, &GeographyObj::NewS<TSMAKamome>, 0, 0 },
    { 0x119e, 0, &GeographyObj::New<TMapObjGeyser>, 0, 0 },
    { 0x119f, 0, &GeographyObj::New<TMapObjGeyserSearch>, 0, 0 },
    { 0xe0f, 0, &GeographyObj::New<TMapObjRollTable>, 0, 0 },
    { 0xe0d, 0, &GeographyObj::NewS<TTestStrBarrel>, 0, 0 },
    { 0xe0b, 0, &GeographyObj::NewS<TTestStrItem>, 0, 0 },
    { 0xd4d, 0, &GeographyObj::New<TMapObjPeachTree>, 0, 0 },
    { 0x13ed, 0, &GeographyObj::New<TMapObjSnowRock>, 0, 0 },
    { 0xe0c, 0, &GeographyObj::New<TTestRollObj>, 0, 0 },
    { 0xd4e, 0, &GeographyObj::New<TMapObjFountain>, 0, 0 },
    { 0x13ee, 0, &GeographyObj::New<TMapObjHeyho>, 0, 0 },
    { 0x13ef, 0, &GeographyObj::NewS<TMapObjSubHeyho>, 0, 0 },
    { 0x1389, 0, &GeographyObj::New<TMapObjSanbo>, 0, 0 },
    { 0x106f, 0, &GeographyObj::New<TMapObjWlArrow>, 0, 0 },
    { 0x13f0, 0, &GeographyObj::New<TMapObjSnowMan>, 0, 0 },
    { 0x13f2, 0, &GeographyObj::New<TMapObjIceBlock>, 0, 0 },
    { 0x1070, 0, &GeographyObj::New<TMapObjNoMove>, 0, 0 },
    { 0x13f3, 0, &GeographyObj::New<TMapObjNoMove>, 0, 0 },
    { 0xe7f, 0, &GeographyObj::New<GeoKuribo>, 0, 0 },
    { 0x11, 0, &GeographyObj::New<TMapObjSun>, 0, 0 },
    { 0x125e, 0, &GeographyObj::New<TMapObjBubble>, 0, 0 },
    { 0x138a, 0, &GeographyObj::New<TMapObjDesun>, 0, 0 },
    { 0x138b, 0, &GeographyObj::New<TMapObjSandPillar>, 0, 0 },
    { 0xce5, 0, &GeographyObj::New<TMapObjFerriswheel>, 0, 0 },
    { 0xdae, 0, &GeographyObj::New<TMapObjMoveItemBoxLimit>, 0, 0 },
    { 0x26b4, 0, &GeographyObj::NewS<GeoShine>, 0, 0 },
    { 0x26ae, 0, &GeographyObj::NewS<TBalloon>, 0, 0 },
    { 0xce6, 0, &GeographyObj::New<TMapObjJetCoasterHead>, 0, 0 },
    { 0xce7, 0, &GeographyObj::NewS<TMapObjJetCoasterBody>, 0, 0 },
    { 0xdaf, 0, &GeographyObj::New<TMapObjPool>, 0, 0 },
    { 0x138c, 0, &GeographyObj::New<TMapObjDeBalloon>, 0, 0 },
    { 0x125f, 0, &GeographyObj::New<TMapObjKpFire>, 0, 0 },
    { 0x138d, 0, &GeographyObj::NewS<TMapObjSanboHead>, 0, 0 },
    { 0x138e, 0, &GeographyObj::NewS<TMapObjSanboBody>, 0, 0 },
    { 0xdb0, 0, &GeographyObj::New<TMapObjRollTable>, 0, 0 },
    { 0x26b2, 0, &GeographyObj::New<TMapObjMoveItemBox>, 0, 0 },
    { 0x11a1, 0, &GeographyObj::New<TMapObjNossie>, 0, 0 },
    { 0xd4f, 0, &GeographyObj::New<TMapObjMareA>, 0, 0 },
    { 0xd50, 0, &GeographyObj::New<TMapObjMareB>, 0, 0 },
    { 0xd51, 0, &GeographyObj::New<TMapObjMareC>, 0, 0 },
    { 0xd52, 0, &GeographyObj::New<TMapObjMonteA>, 0, 0 },
    { 0xd53, 0, &GeographyObj::New<TMapObjMonteB>, 0, 0 },
    { 0xd54, 0, &GeographyObj::New<TMapObjMonteC>, 0, 0 },
    { 0xd58, 0, &GeographyObj::New<TMapObjMonteD>, 0, 0 },
    { 0xd59, 0, &GeographyObj::New<TMapObjMonteE>, 0, 0 },
    { 0xd5a, 0, &GeographyObj::New<TMapObjMareW_A>, 0, 0 },
    { 0xd5b, 0, &GeographyObj::New<TMapObjMareW_B>, 0, 0 },
    { 0xd5c, 0, &GeographyObj::New<TMapObjMareW_C>, 0, 0 },
    { 0xd5d, 0, &GeographyObj::New<TMapObjMareM_A>, 0, 0 },
    { 0x1327, 0, &GeographyObj::New<TMapObjMeteor>, 0, 0 },
    { 0xd55, 0, &GeographyObj::New<TMapObjUkleleMonte>, 0, 0 },
    { 0xd56, 0, &GeographyObj::New<TMapObjDanceMonteA>, 0, 0 },
    { 0xd57, 0, &GeographyObj::New<TMapObjDanceMonteB>, 0, 0 },
    { 0xd66, 0, &GeographyObj::New<TMapObjAwardItemBox>, 0, 0 },
    { 0xd67, 0, &GeographyObj::NewS<TMapObjAwardCupKinoko>, 0, 0 },
    { 0xd68, 0, &GeographyObj::NewS<TMapObjAwardCupFlower>, 0, 0 },
    { 0xd69, 0, &GeographyObj::NewS<TMapObjAwardCupStar>, 0, 0 },
    { 0xd6a, 0, &GeographyObj::NewS<TMapObjAwardCupSpecial>, 0, 0 },
    { 0xd6b, 0, &GeographyObj::NewS<TMapObjAwardCupReverse>, 0, 0 },
    { 0xd6c, 0, &GeographyObj::NewS<TMapObjAwardCupAllTour>, 0, 0 },
    { 0x138f, 0, &GeographyObj::New<TMapObjAntLion>, 0, 0 },
    { 0xe80, 0, &GeographyObj::New<TMapObjPakkun>, 0, 0 },
    { 0xd7c, 0, &GeographyObj::New<GeoKinojii>, 0, 0 },
    { 0x11a4, 0, &GeographyObj::New<TMapObjDinoTree>, 0, 0 },
    { 0xd7d, 0, &GeographyObj::New<TMapObjDemoFountain>, 0, 0 },
    { 0x1261, 0, &GeographyObj::New<TMapObjKpGear>, 0, 0 },
    { 0x1262, 0, &GeographyObj::New<TMapObjKpFireBar>, 0, 0 },
    { 0x11a5, 0, &GeographyObj::New<TMapObjSwimNossie>, 0, 0 },
    { 0x11a6, 0, &GeographyObj::New<TMapObjPtera>, 0, 0 },
    { 0xd7f, 0, &GeographyObj::New<TMapObjPeachTree2>, 0, 0 },
    { 0xed9, 0, &GeographyObj::New<TMapObjYoshiHeli>, 0, 0 },
    { 0x1392, 0, &GeographyObj::New<TMapObjDesertTree>, 0, 0 },
    { 0x1072, 0, &GeographyObj::New<TMapObjWluSearchLight>, 0, 0 },
    { 0xdb1, 0, &GeographyObj::New<TMapObjVentilator>, 0, 0 },
    { 0x26b5, 0, &GeographyObj::NewS<TShineArrow>, 0, 0 },
    { 0x1329, 0, &GeographyObj::New<TMapObjUFO>, 0, 0 },
    { 0xe82, 0, &GeographyObj::New<TMapObjMashBalloon>, 0, 0 },
    { 0xce8, 0, &GeographyObj::New<TMapObjPathYoshiHeli>, 0, 0 },
    { 0x106a, 0, &GeographyObj::NewS<TMapObjFireBall>, 0, 0 },
    { 0x1263, 0, &GeographyObj::NewS<TMapObjKpFireBarBall>, 0, 0 },
    { 0x1260, 0, &GeographyObj::NewS<TMapObjKpFireBall>, 0, 0 },
};
GeoFuncTable::GeoFuncTblEntry GeoFuncTable::GeoPrevDrawFuncTbl[] = {
    { 0xda2, 0, &GeographyObj::NewS<GeoWaterDown>, 0, 0 },
    { 0xda3, 0, &GeographyObj::NewS<GeoWaterMiddleD>, 0, 0 },
};
GeoFuncTable::GeoFuncTblEntry GeoFuncTable::GenGeoFuncTbl[] = {
    { 0x1196, 0, &GeographyObj::New<TMapObjDonkyRockGen>, 0, 0 },
    { 0x106b, 0, &GeographyObj::New<TMapObjFireCircle>, 0, 0 },
    { 0xe, 0, &GeographyObj::New<GeoItemGen>, 0, 0 },
    { 0x119a, 0, &GeographyObj::New<TSMAButterflyGen>, 0, 0 },
    { 0x119c, 0, &GeographyObj::New<TSMADonkyBirdGen>, 0, 0 },
    { 0xd4b, 0, &GeographyObj::New<TSMABoidBirdManager>, 0, 0 },
    { 0xe0e, 0, &GeographyObj::New<TTestString>, 0, 0 },
    { 0x26ad, 0, &GeographyObj::NewS<TBalloonString>, 0, 0 },
    { 0x10, 0, &GeographyObj::New<TLensFlare>, 0, 0 },
    { 0x13f1, 0, &GeographyObj::New<TDistSnow>, 0, 0 },
    { 0x26af, 0, &GeographyObj::NewS<TBalloonManager>, 0, 0 },
    { 0x13f4, 0, &GeographyObj::New<GeoAirJet>, 0, 0 },
    { 0x11a2, 0, &GeographyObj::NewS<TMapObjNossieColHead>, 0, 0 },
    { 0x11a3, 0, &GeographyObj::NewS<TMapObjNossieColFootFront>, 0, 0 },
    { 0x11a7, 0, &GeographyObj::NewS<TMapObjNossieColFootRear>, 0, 0 },
    { 0x11a8, 0, &GeographyObj::NewS<TMapObjNossieColBody>, 0, 0 },
    { 0xd7a, 0, &GeographyObj::New<TMapObjHanabi>, 0, 0 },
    { 0x1071, 0, &GeographyObj::New<GeoSplash>, 0, 0 },
    { 0xd7e, 0, &GeographyObj::New<GeoTideSensor>, 0, 0 },
    { 0x1390, 0, &GeographyObj::New<GeoPuller>, 0, 0 },
    { 0x1391, 0, &GeographyObj::New<GeoShimmer>, 0, 0 },
    { 0xe81, 0, &GeographyObj::NewS<TMapObjPakkunColHead>, 0, 0 },
    { 0x13, 0, &GeographyObj::New<GeoSoundGen>, 0, 0 },
    { 0x26b3, 0, &GeographyObj::New<GeoRabbitMark>, 0, 0 },
};
GeoFuncTable::GeoExFuncTblEntry GeoFuncTable::BlockGeoFuncTbl[] = {
    { 0x3, 0, &ExGeographyObj::ExNew<GeoJumpNarrow>, 0, 0 },
};
GeoFuncTable::GeoExFuncTblEntry GeoFuncTable::SpecialGeoFuncTbl[] = {
    { 0x7, 0, &ExGeographyObj::ExNew<StringBridge>, 0, 0 },
};

u32 TimeAttackInvalidGeoObj::TimeAttackInvalidObjTbl[] = { 0xA, 0xFA8, 0xDAE };

// ---------------------------------------------------------------------------
// anonymous namespace data
// ---------------------------------------------------------------------------

namespace
{
    f32 sKartBoundRadius[21] = {
        75.0f, 90.0f, 75.0f, 65.0f, 75.0f, 65.0f, 90.0f, 90.0f, 75.0f, 65.0f, 75.0f,
        65.0f, 75.0f, 65.0f, 75.0f, 65.0f, 65.0f, 65.0f, 90.0f, 90.0f, 100.0f
    };

    f32 sKartBoundOffsetY[21] = {
        75.0f, 75.0f, 75.0f, 85.0f, 110.0f, 85.0f, 110.0f, 95.0f, 75.0f, 80.0f, 75.0f,
        85.0f, 110.0f, 85.0f, 85.0f, 85.0f, 100.0f, 100.0f, 95.0f, 95.0f, 80.0f
    };

    f32 sKartBoundOffsetZ[21] = {
        0.0f, 25.0f, 20.0f, 10.0f, 15.0f, 0.0f, 30.0f, 20.0f, 0.0f, 30.0f, 25.0f,
        10.0f, 15.0f, 0.0f, 20.0f, 10.0f, 0.0f, 0.0f, 5.0f, 5.0f, 0.0f
    };

    u32 sRandomItemBoxTable[10] = { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 };

    f32 sNoColComKartToObjectLength = 7000.0f;
} // namespace





// ---------------------------------------------------------------------------
// GeographyObjManager
// ---------------------------------------------------------------------------

GeographyObjManager::GeographyObjManager(const CrsData &crsData)
    : mShineArrowMgr(nullptr)
{
    gMgr = this;
    mSpecialHeadObj = nullptr;
    checkMiniGameMode();
    stSetRndPermission(1, false);
    mGeoHitTable = ObjUtility::getPtrMainArc("/Objects/GeoHitTable.dat");
    mIsLoaded = false;
    mJugemNum = 0;
    setSequentialID();

    mModelData = new J3DModelData *[mGeoObjNum];
    mShadowModelData = new J3DModelData *[mGeoObjNum];
    mSimpleModelFlags = new u8[mGeoObjNum];
    for (int i = 0; i < mGeoObjNum; i++)
    {
        mModelData[i] = nullptr;
        mSimpleModelFlags[i] = 0;
        mShadowModelData[i] = nullptr;
    }
    _20 = 0;

    mSupervisorList.initiate();
    mGeoObjList.initiate();
    mPrevDrawObjList.initiate();
    mGenObjList.initiate();
    mJugemObjList.initiate();
    mBlockObjList.initiate();
    mSpObjList.initiate();
    mBoxObjList.initiate();
    for (u8 i = 0; i < 3; i++)
    {
        mItemBoxStockList[i].initiate();
    }

    clrKartHittingList();
    reset(crsData);
    SysDebug::getManager()->setHeapGroup("OBJECT MODEL", nullptr);
    load();
    SysDebug::getManager()->setHeapGroup("OBJECT MGR", nullptr);
    mIsLoaded = true;
}

void GeographyObjManager::checkMiniGameMode()
{
    for (u8 i = 0; i < 8; i++)
        mBalloonMgr[i] = nullptr;
    RaceInfo *raceInfo = RaceMgr::getManager()->mRaceInfo;
    mIsBalloonBattle = raceInfo->getRaceMode() == BALLOON_BATTLE;
    mIsBombBattle = RaceMgr::getManager()->mRaceInfo->getRaceMode() == BOMB_BATTLE;
    mIsRobberyBattle = RaceMgr::getManager()->mRaceInfo->getRaceMode() == ROBBERY_BATTLE;
    mIsEscapeBattle = RaceMgr::getManager()->mRaceInfo->getRaceMode() == ESCAPE_BATTLE;
}

GeographyObjManager::~GeographyObjManager()
{
    dispose();
}

void GeographyObjManager::clrKartHittingList()
{
    GeographyObj **list = mKartHittingList[0];
    for (int i = 0; i < 8; i++, list += 20)
    {
        for (u8 j = 0; j < 20; j++)
        {
            list[j] = nullptr;
        }
        mKartHitRefVec[i].zero();
        mKartHitDepth[i] = 0.0f;
        mKartHitItemBox[i] = false;
    }
}

void GeographyObjManager::setKartHitItemBoxTrigger(const int kart_index)
{
#line 287 "GeographyObjMgr.cpp"
    JUT_MINMAX_ASSERT(0, kart_index, 8);

    mKartHitItemBox[kart_index] = true;
}

bool GeographyObjManager::tstKartHitItemBoxTrigger(const int kart_index)
{
#line 300 "GeographyObjMgr.cpp"
    JUT_MINMAX_ASSERT(0, kart_index, 8);
    return mKartHitItemBox[kart_index];
}

void GeographyObjManager::setNewItemBoxObj(GeographyObj *obj)
{
    u32 sel = 0;
    if (obj->getKind() == 0xc)
        sel = 1;

    boxremove(obj, sel);

    u32 rnd = sRandomItemBoxTable[stGetRnd(1)->getRandomMax(9)];
    if (mItemBoxStockList[rnd].getNumLinks() == 0)
    {
        for (int i = 0; i < 3; i++)
        {
            if (mItemBoxStockList[i].getNumLinks() != 0)
            {
                rnd = i;
                break;
            }
        }
    }

    RaceMgr::getManager()->mRaceDrawer->getGeoDrawBuffer()->clrLock();

    GeographyObj *itemBox = mItemBoxStockList[rnd].getFirst()->getObject();
#line 393 "GeographyObjMgr.cpp"
    JUT_ASSERT_MSG(itemBox != nullptr, "can't set itembox");
#line 394 "GeographyObjMgr.cpp"
    boxappend(itemBox);
    itemBox->setObjData(*obj->mObjData, true);
    itemBox->mScale.zero();
}

void GeographyObjManager::resetAllGeoObj()
{
    JSUListIterator<GeoObjSupervisor> svIt(mSupervisorList.getFirst());
    while (svIt.isAvailable())
    {
        svIt->reset();
        ++svIt;
    }

    JSUListIterator<GeographyObj> it(mGenObjList.getFirst());
    while (it.isAvailable())
    {
        it->reset();
        ++it;
    }

    JSUListIterator<GeographyObj> it2(mGeoObjList.getFirst());
    while (it2.isAvailable())
    {
        it2->reset();
        ++it2;
    }

    JSUListIterator<GeographyObj> it3(mBoxObjList.getFirst());
    while (it3.isAvailable())
    {
        it3->reset();
        ++it3;
    }

    JSUListIterator<GeographyObj> it4(mPrevDrawObjList.getFirst());
    while (it4.isAvailable())
    {
        it4->reset();
        ++it4;
    }

    JSUListIterator<GeographyObj> it5(mJugemObjList.getFirst());
    while (it5.isAvailable())
    {
        it5->reset();
        ++it5;
    }

    JSUListIterator<ExGeographyObj> it6(mBlockObjList.getFirst());
    while (it6.isAvailable())
    {
        it6->reset();
        ++it6;
    }

    JSUListIterator<ExGeographyObj> it7(mSpObjList.getFirst());
    while (it7.isAvailable())
    {
        it7->reset();
        ++it7;
    }
}

void GeographyObjManager::checkObjList(const CrsData &crsData)
{
    int state;
    if (!mIsLoaded)
    {
        if (mNormObjNum != 0)
            state = 1;
        else
            state = 0;
    }
    else
        state = 2;

    if (state == 1)
    {
        for (int i = 0; i < crsData.getObjectEntryNum(); i++)
        {
            CrsData::SObject *object = &crsData.getObjectTable()[i];
            bool timeAttackInvalid = false;
            for (u16 j = 0; j < 3; j++)
            {
                if (!RaceMgr::getManager()->isItemBoxValid() && object->objID == (int)TimeAttackInvalidGeoObj::TimeAttackInvalidObjTbl[j])
                {
                    timeAttackInvalid = true;
                    break;
                }
            }
            if (timeAttackInvalid)
                continue;
            if (!object->isAvailableObject())
                continue;
            if (object->objID == 1)
                continue;

            bool itemBox = false;
            switch (object->objID)
            {
            case 0xa:
            case 0xdae:
            case 0xfa8:
                itemBox = true;
            }
            if (itemBox)
                chkCreateItemBoxObj(object, object->objID);
            else
                chkCreateNormalObj(object, object->objID);
        }
    }
}

void GeographyObjManager::checkItemBoxObjList(const CrsData &crsData)
{
    int state;
    if (!mIsLoaded)
    {
        if (mNormObjNum != 0)
            state = 1;
        else
            state = 0;
    }
    else
        state = 2;

    if (state == 2)
    {
        RaceMgr::getManager()->mRaceDrawer->getGeoDrawBuffer()->clrLock();

        for (u8 pass = 0; pass < 2; pass++)
        {
            for (int i = 0; i < crsData.getObjectEntryNum(); i++)
            {
                CrsData::SObject *object = &crsData.getObjectTable()[i];
                if (!object->isAvailableObject())
                    continue;
                if (object->objID != 1)
                    continue;
                if (!RaceMgr::getManager()->isItemBoxValid())
                    continue;

                if (pass == 0)
                {
                    int sel;
                    switch (object->mParam2)
                    {
                    case 1:
                        sel = 0;
                        break;
                    case 3:
                        sel = 1;
                        break;
                    default:
                        sel = -1;
                        break;
                    }
                    if (sel < 0)
                        continue;
                    GeographyObj *itemBox = mItemBoxStockList[sel].getFirst()->getObject();
                    if (itemBox != nullptr)
                    {
                        boxappend(itemBox);
                        itemBox->setObjData(*object, false);
                    }
                }
                else
                {
                    if (object->mParam2 != 0)
                        continue;

                    u32 sel = sRandomItemBoxTable[stGetRnd(1)->getRandomMax(9)];
                    if (mItemBoxStockList[sel].getNumLinks() == 0)
                    {
                        for (int j = 0; j < 3; j++)
                        {
                            if (mItemBoxStockList[j].getNumLinks() != 0)
                            {
                                sel = j;
                                break;
                            }
                        }
                    }
                    GeographyObj *itemBox = mItemBoxStockList[sel].getFirst()->getObject();
                    if (itemBox != nullptr)
                    {
                        boxappend(itemBox);
                        itemBox->setObjData(*object, false);
                    }
                }
            }
        }
    }
    else if (state == 1)
    {
        for (int i = 0; i < crsData.getObjectEntryNum(); i++)
        {
            CrsData::SObject *object = &crsData.getObjectTable()[i];
            if (!object->isAvailableObject())
                continue;
            if (object->objID != 1)
                continue;
            if (!RaceMgr::getManager()->isItemBoxValid())
                continue;

            if (object->mParam2 == 0)
            {
                stSetRndPermission(1, true);
                u32 sel = sRandomItemBoxTable[stGetRnd(1)->getRandomMax(9)];
                if (mItemBoxStockList[sel].getNumLinks() == 0)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        if (mItemBoxStockList[j].getNumLinks() != 0)
                        {
                            sel = j;
                            break;
                        }
                    }
                }
                GeographyObj *itemBox = mItemBoxStockList[sel].getFirst()->getObject();
                stSetRndPermission(1, false);
                boxappend(itemBox);
                itemBox->setObjData(*object, false);
            }
            else
            {
                int kind = 1;
                if (object->mParam2 == 3)
                    kind = 0xc;
#line 696 "GeographyObjMgr.cpp"
                JUT_ASSERT_MSG(kind != 0, "can't hold itemBox");
#line 697 "GeographyObjMgr.cpp"
                GeographyObj *itemBox = chkCreateItemBoxObj(object, kind);
                itemBox->setObjData(*object, false);
            }
        }
    }
}

GeographyObj *GeographyObjManager::chkCreateNormalObj(const CrsData::SObject *object, u32 kind)
{
    GeographyObj *obj = createObj(kind, object);
    if (obj != nullptr)
    {
        append(obj);
        appendKindObjList(obj);
    }
    return obj;
}

GeographyObj *GeographyObjManager::chkCreateItemBoxObj(const CrsData::SObject *object, u32 kind)
{
    GeographyObj *obj = createObj(kind, object);
    if (obj != nullptr)
    {
        boxappend(obj);
        appendKindObjList(obj);
    }
    return obj;
}

void GeographyObjManager::appendKindObjList(GeographyObj *obj)
{
    u16 seqID = obj->_110;
#line 796 "GeographyObjMgr.cpp"
    JUT_ASSERT(seqID < ( GeoFuncTableMax + GeoPrevDrawFuncTableMax + GenGeoFuncTableMax ));
#line 797 "GeographyObjMgr.cpp"
    if (mKindObjTbl[seqID] == nullptr)
        mKindObjTbl[seqID] = obj;
}

void GeographyObjManager::checkJugemObjList(const CrsData &crsData)
{
    if (mIsLoaded)
        return;

    mJugemNum = RaceMgr::getManager()->mRaceInfo->mConsoleNum;
#line 824 "GeographyObjMgr.cpp"
    JUT_ASSERT_MSG(mJugemNum > 0, "mJugemNum > 0");
#line 825 "GeographyObjMgr.cpp"
    mJugem = new TJugem *[mJugemNum];

    for (int kartIdx = 0; kartIdx < mJugemNum; kartIdx++)
    {
        mJugem[kartIdx] = nullptr;

        TJugemVoidRod *voidRod = nullptr;
        TJugemRodBoard *rodB1 = nullptr;
        TJugemRodBoard2 *rodB2 = nullptr;
        TJugemRodBoardRev *rodRev = nullptr;
        TJugemRodPukuPuku *rodPuku = nullptr;
        TJugemHeadHear *headHear = nullptr;
        TJugemFlag *flag = nullptr;
        TJugemHeadLamp *headLamp = nullptr;
        u32 kind;
        TJugem *jugem;

        CrsData::SObject *object = crsData.getObjectTable();
        for (kind = 0x109; kind >= 0xff; kind--)
        {
            GeographyObj *obj = createObj(kind, object);
            if (obj == nullptr)
                continue;

            u32 objKind = obj->getKind();
            if (objKind == 0x101)
            {
                voidRod = (TJugemVoidRod *)obj;
                voidRod->setJugemRodItem(rodRev, 4);
                voidRod->setJugemRodItem(rodB2, 3);
                voidRod->setJugemRodItem(rodB1, 1);
                voidRod->setJugemRodItem(rodPuku, 2);
            }
            else if (objKind == 0x104)
                rodB1 = (TJugemRodBoard *)obj;
            else if (objKind == 0x105)
                rodB2 = (TJugemRodBoard2 *)obj;
            else if (objKind == 0x106)
                rodRev = (TJugemRodBoardRev *)obj;
            else if (objKind == 0x107)
                rodPuku = (TJugemRodPukuPuku *)obj;
            else if (objKind == 0x102)
                flag = (TJugemFlag *)obj;
            else if (objKind == 0x100)
                headHear = (TJugemHeadHear *)obj;
            else if (objKind == 0x109)
                headLamp = (TJugemHeadLamp *)obj;
            else if (objKind == 0xff)
            {
                jugem = (TJugem *)obj;
                jugem->setJugemItem(voidRod, 1);
                *(void **)((char *)jugem + 0x26c) = *(void **)((char *)voidRod + 0x190); // TODO: name these members
                jugem->setJugemItem(flag, 2);
                jugem->setJugemHeadItem(headHear, 0);
                jugem->setCameraNum(kartIdx);
                jugem->setKartNum(RaceMgr::getManager()->getTargetKartNo(kartIdx));
                jugem->setJugemHeadItem(headLamp, 1);
                mJugem[kartIdx] = jugem;
            }
        }

        for (kind = 0x109; kind >= 0xff; kind--)
        {
            jugemappend(mJugem[kartIdx]);
            jugemappend(headHear);
            jugemappend(headLamp);
            jugemappend(voidRod);
            jugemappend(flag);
            jugemappend(rodPuku);
            jugemappend(rodB1);
            jugemappend(rodB2);
            jugemappend(rodRev);
        }
    }
}

void GeographyObjManager::setSequentialID()
{
    GeoFuncTableMax = 132;
    GeoPrevDrawFuncTableMax = 2;

    u16 seq = 0;
    GeoFuncTable::GeoFuncTblEntry *entry = GeoFuncTable::GeoFuncTbl;
    u16 num = GeoFuncTableMax + GeoPrevDrawFuncTableMax;
    mSeqObjAnmTbl = new GeoObjAnmData *[num];
    for (u16 i = 0; i < num; i++, seq++, entry++)
    {
        entry->sequentialID = seq;
        mSeqObjAnmTbl[i] = nullptr;
    }

    GenGeoFuncTableMax = 24;
    GeoFuncTable::GeoFuncTblEntry *genEntry = GeoFuncTable::GenGeoFuncTbl;
    for (u16 i = 0; i < GenGeoFuncTableMax; i++, seq++, genEntry++)
    {
        genEntry->sequentialID = seq;
    }

    u16 num2 = GeoFuncTableMax + GeoPrevDrawFuncTableMax + GenGeoFuncTableMax;
    mKindObjTbl = new GeographyObj *[num2];
    for (u16 i = 0; i < num2; i++)
    {
        mKindObjTbl[i] = nullptr;
    }

    BlockGeoFuncTableMax = 1;
    GeoFuncTable::GeoExFuncTblEntry *blockEntry = GeoFuncTable::BlockGeoFuncTbl;
    for (u8 i = 0; i < BlockGeoFuncTableMax; i++, seq++, blockEntry++)
    {
        blockEntry->sequentialID = seq;
    }

    SpecialGeoFuncTableMax = 1;
    GeoFuncTable::GeoExFuncTblEntry *specialEntry = GeoFuncTable::SpecialGeoFuncTbl;
    for (u8 i = 0; i < SpecialGeoFuncTableMax; i++, seq++, specialEntry++)
    {
        specialEntry->sequentialID = seq;
    }

    mGeoObjNum = num2 + BlockGeoFuncTableMax + SpecialGeoFuncTableMax;
}

void GeographyObjManager::countObjNum(const CrsData::SObject *obj)
{
    u32 kind = obj->objID;
    GeoFuncTable::GeoFuncTblEntry *entry = GeoFuncTable::GeoFuncTbl;
    for (u16 i = 0; i < GeoFuncTableMax; i++, entry++) {
        if (entry->kind == kind) {
            if (kind == 1)
                mItemBoxObjNum++;
            mNormObjNum++;
            return;
        }
    }
    GeoFuncTable::GeoFuncTblEntry *prevEntry = GeoFuncTable::GeoPrevDrawFuncTbl;
    for (u16 i = 0; i < GeoPrevDrawFuncTableMax; i++, prevEntry++) {
        if (prevEntry->kind == kind) {
            mPrevDrawGeoObjNum++;
            return;
        }
    }
    GeoFuncTable::GeoFuncTblEntry *genEntry = GeoFuncTable::GenGeoFuncTbl;
    for (u16 i = 0; i < GenGeoFuncTableMax; i++, genEntry++) {
        if (genEntry->kind == kind) {
            mGenGeoObjNum++;
            return;
        }
    }
    GeoFuncTable::GeoExFuncTblEntry *blockEntry = GeoFuncTable::BlockGeoFuncTbl;
    for (u16 i = 0; i < BlockGeoFuncTableMax; i++, blockEntry++) {
        if (blockEntry->kind == kind) {
            mBlockGeoObjNum++;
            return;
        }
    }
    GeoFuncTable::GeoExFuncTblEntry *specialEntry = GeoFuncTable::SpecialGeoFuncTbl;
    for (u16 i = 0; i < SpecialGeoFuncTableMax; i++, specialEntry++) {
        if (specialEntry->kind == kind) {
            mSpecialGeoObjNum++;
            return;
        }
    }
}

GeographyObj *GeographyObjManager::createObj(u32 kind, const CrsData::SObject *object)
{
    GeographyObj *obj = nullptr;
    GeoFuncTable::GeoFuncTblEntry *entry = GeoFuncTable::GeoFuncTbl;
    for (u16 i = 0; i < GeoFuncTableMax; i++, entry++)
    {
        if (entry->kind == kind)
        {
            obj = entry->createFunc(*object);
            obj->_110 = entry->sequentialID;
        }
    }
    return obj;
}

GeographyObj *GeographyObjManager::createSubObj(u32 kind)
{
    GeographyObj *obj = createObj(kind, nullptr);
    if (obj != nullptr)
        append(obj);
    return obj;
}

inline GeographyObj *GeographyObjManager::createPrevDrawObj(u32 kind, const CrsData::SObject *object)
{
    GeographyObj *obj = nullptr;
    GeoFuncTable::GeoFuncTblEntry *entry = GeoFuncTable::GeoPrevDrawFuncTbl;
    for (u16 i = 0; i < GeoPrevDrawFuncTableMax; i++, entry++)
    {
        if (entry->kind == kind)
        {
            obj = entry->createFunc(*object);
            obj->_110 = entry->sequentialID;
        }
    }
    return obj;
}

GeographyObj *GeographyObjManager::createPrevDrawSubObj(u32 kind)
{
    GeographyObj *obj = createPrevDrawObj(kind, nullptr);
    if (obj != nullptr)
        prevdraw_append(obj);
    return obj;
}

inline GeographyObj *GeographyObjManager::createGenObj(u32 kind, const CrsData::SObject *object)
{
    GeographyObj *obj = nullptr;
    GeoFuncTable::GeoFuncTblEntry *entry = GeoFuncTable::GenGeoFuncTbl;
    for (int i = 0; i < GenGeoFuncTableMax; i++)
    {
        if (entry->kind == kind)
        {
            obj = entry->createFunc(*object);
            obj->_110 = entry->sequentialID;
            break;
        }
        entry++;
    }
    return obj;
}

GeographyObj *GeographyObjManager::createGenSubObj(u32 kind)
{
    GeographyObj *obj = createGenObj(kind, nullptr);
    if (obj != nullptr)
    {
        genappend(obj);
        obj->createColModel(nullptr);
    }
    return obj;
}

#pragma push
#pragma auto_inline off
void GeographyObjManager::checkGenObjList(const CrsData &crsData)
{
    int state;
    if (!mIsLoaded)
    {
        if (mGenGeoObjNum != 0)
            state = 1;
        else
            state = 0;
    }
    else
        state = 2;

    if (state == 2)
    {
    }
    else if (state == 1)
    {
        for (int i = 0; i < crsData.getObjectEntryNum(); i++)
        {
            CrsData::SObject *object = &crsData.getObjectTable()[i];
            if (object->isAvailableObject())
                createAppendGenObj(object->objID, object);
        }
    }

    if (state != 2)
    {
        if (isMiniGame() && !mIsBombBattle && !mIsEscapeBattle)
            createAllBalloonObj();
    }

    if (mIsEscapeBattle && mShineArrowMgr == nullptr)
    {
        mShineArrowMgr = new TShineArrowManager();
        mShineArrowMgr->create();
    }
}
#pragma pop

GeographyObj *GeographyObjManager::createAppendGenObj(u32 kind, const CrsData::SObject *object)
{
    GeographyObj *obj = createGenObj(kind, object);
    if (obj != nullptr)
    {
        genappend(obj);
        appendKindObjList(obj);
        obj->createColModel(nullptr);
    }
    return obj;
}

void GeographyObjManager::createAllBalloonObj()
{
    u8 teamNum = 1;
    if (mIsBalloonBattle)
        teamNum = 3;

#line 1364 "GeographyObjMgr.cpp"
    u8 playerNum = RaceMgr::getManager()->getPlayerNumber();
    JUT_ASSERT_MSG(playerNum > 0, "playernum > 0");

    s8 map[8];
    for (int i = 0; i < 8; i++)
        map[i] = -1;

    for (u32 i = 0; i < RaceMgr::getManager()->getConsoleNumber(); i++)
        map[RaceMgr::getManager()->getTargetKartNo(i)] = i;

    for (u8 i = 0; i < playerNum; i++)
    {
        mBalloonMgr[i] = (TBalloonManager *)createAppendGenObj(0x26af, nullptr);
        mBalloonMgr[i]->init(i, map[i], teamNum);
    }
}

inline GeographyObj *GeographyObjManager::chkCreatePrevDrawObj(const CrsData::SObject *object, u32 kind)
{
    GeographyObj *obj = createPrevDrawObj(kind, object);
    if (obj != nullptr)
    {
        prevdraw_append(obj);
        appendKindObjList(obj);
    }
    return obj;
}

inline void GeographyObjManager::checkPrevDrawObjList(const CrsData &crsData)
{
    int state;
    if (!mIsLoaded)
    {
        if (mPrevDrawGeoObjNum != 0)
            state = 1;
        else
            state = 0;
    }
    else
        state = 2;

    if (state == 1)
    {
        for (int i = 0; i < crsData.getObjectEntryNum(); i++)
        {
            CrsData::SObject *object = &crsData.getObjectTable()[i];
            if (object->isAvailableObject())
                chkCreatePrevDrawObj(object, object->objID);
        }
    }
}

inline GeographyObj *GeographyObjManager::createSubItemBoxObj(u32 kind)
{
    GeographyObj *obj = createObj(kind, nullptr);
    if (obj != nullptr)
        boxappend(obj);
    return obj;
}

inline void GeographyObjManager::createAllItemBox()
{
    if (RaceMgr::getManager()->isItemBoxValid() && !mIsLoaded)
    {
        int normalNum = (f32)mItemBoxObjNum;
        int doubleNum = 0.6f * (f32)mItemBoxObjNum;
        for (u16 i = 0; i < (u16)normalNum; i++)
        {
            GeographyObj *box = createSubItemBoxObj(1);
            if (box != nullptr)
                boxremove(box, 0);
        }
        for (u16 i = 0; i < (u16)doubleNum; i++)
        {
            GeographyObj *box = createSubItemBoxObj(0xc);
            if (box != nullptr)
                boxremove(box, 1);
        }
    }
}

inline ExGeographyObj *GeographyObjManager::createBlockObj(u32 kind, const CrsData::SObject *object)
{
    ExGeographyObj *obj = nullptr;
    GeoFuncTable::GeoExFuncTblEntry *entry = GeoFuncTable::BlockGeoFuncTbl;
    for (int i = 0; i < BlockGeoFuncTableMax; i++)
    {
        if (entry->kind == kind)
        {
            obj = entry->createFunc(*object);
            obj->_110 = entry->sequentialID;
            break;
        }
        entry++;
    }
    return obj;
}

inline void GeographyObjManager::checkBlockObjList(const CrsData &crsData)
{
    int state;
    if (!mIsLoaded)
    {
        if (mBlockGeoObjNum != 0)
            state = 1;
        else
            state = 0;
    }
    else
        state = 2;

    if (state == 2)
    {
    }
    else if (state == 1)
    {
        for (int i = 0; i < crsData.getObjectEntryNum(); i++)
        {
            CrsData::SObject *object = &crsData.getObjectTable()[i];
            ExGeographyObj *obj = createBlockObj(object->objID, object);
            if (obj != nullptr)
                blockappend(obj);
        }
    }
}

inline ExGeographyObj *GeographyObjManager::createSpecialObj(u32 kind, const CrsData::SObject *object)
{
    ExGeographyObj *obj = nullptr;
    GeoFuncTable::GeoExFuncTblEntry *entry = GeoFuncTable::SpecialGeoFuncTbl;
    for (int i = 0; i < SpecialGeoFuncTableMax; i++)
    {
        if (entry->kind == kind)
        {
            obj = entry->createFunc(*object);
            obj->_110 = entry->sequentialID;
            if (mSpecialHeadObj == nullptr)
                mSpecialHeadObj = obj;
            break;
        }
        entry++;
    }
    return obj;
}

inline void GeographyObjManager::checkSpObjList(const CrsData &crsData)
{
    int state;
    if (!mIsLoaded)
    {
        if (mSpecialGeoObjNum != 0)
            state = 1;
        else
            state = 0;
    }
    else
        state = 2;

    if (state == 2)
    {
    }
    else if (state == 1)
    {
        for (int i = 0; i < crsData.getObjectEntryNum(); i++)
        {
            CrsData::SObject *object = &crsData.getObjectTable()[i];
            ExGeographyObj *obj = createSpecialObj(object->objID, object);
            if (obj != nullptr)
                spappend(obj);
        }
    }
}

inline void GeographyObjManager::initClassCreateNumFunc()
{
    for (u16 i = 0; i < GeoFuncTableMax + GeoPrevDrawFuncTableMax + GenGeoFuncTableMax; i++)
    {
        if (mKindObjTbl[i] != nullptr)
            mKindObjTbl[i]->initClassCreateNum();
    }
}

inline void GeographyObjManager::checkSupervisorObjList()
{
    for (u16 i = 0; i < GeoFuncTableMax + GeoPrevDrawFuncTableMax + GenGeoFuncTableMax; i++)
    {
        if (mKindObjTbl[i] != nullptr)
        {
            GeoObjSupervisor *supervisor = mKindObjTbl[i]->getSupervisor();
            if (supervisor != nullptr)
                mSupervisorList.append(&supervisor->mLink);
        }
    }
}

void GeographyObjManager::setObjectByCrsData(const CrsData &crsData)
{
    if (crsData.mObjectData == nullptr)
        return;

    setGeoObjNum(crsData);
    checkObjList(crsData);
    checkPrevDrawObjList(crsData);

    createAllItemBox();

    checkItemBoxObjList(crsData);
    checkGenObjList(crsData);

    initClassCreateNumFunc();

    checkJugemObjList(crsData);

    checkSupervisorObjList();

    checkBlockObjList(crsData);
    checkSpObjList(crsData);

    if (mIsLoaded)
        resetAllGeoObj();
}

void GeographyObjManager::setGeoObjNum(const CrsData &crsData)
{
    if (mIsLoaded)
        return;

    mNormObjNum = mPrevDrawGeoObjNum = mGenGeoObjNum = mBlockGeoObjNum = mSpecialGeoObjNum = mItemBoxObjNum = 0;
    for (u16 i = 0; i < (int)crsData.getObjectEntryNum(); i++)
        countObjNum(&crsData.getObjectTable()[i]);
}

// Not inline: compiling it here pools its strings ahead of setModelData's.
static void checkObjResource(void *data, char *path) // fabricated
{
    if (data == nullptr)
        OSReport("not found %s\n", path);

#line 1603 "GeographyObjMgr.cpp"
    JUT_ASSERT_F(data != nullptr, "can't find %s", path);
}

void GeographyObjManager::setModelData(GeographyObj *obj)
{
    u16 seqID = obj->_110;
    if (mModelData[seqID] != nullptr)
        return;

    u32 tevStageNum = obj->getJ3DModelDataTevStageNum();
    const char *name = obj->getBmdFileName();

    char buf[64];
    char *path;
    if (obj != nullptr && obj->mObjFlag & 0x20)
    {
        snprintf(buf, sizeof(buf), "/Objects/%s", name);
        path = buf;
    }
    else
    {
        path = (char *)name;
    }

    void *data;
    if (obj->mObjFlag & 0x2)
    {
        data = ObjUtility::getPtrMainArc(path);
        if (data == nullptr)
            data = ObjUtility::getPtrCourseArc(path);
    }
    else
    {
        data = ObjUtility::getPtrCourseArc(path);
        if (data == nullptr)
            data = ObjUtility::getPtrMainArc(path);
        if (data == nullptr)
            data = ObjUtility::getPtrMainLocaleArc(path);
    }

    if (RaceMgr::getManager()->mRaceInfo->mRaceMode == AWARD_DEMO && data == nullptr)
        data = (*(JKRArchive **)((char *)RaceMgr::getManager() + 0x90))->getResource(path); // mAwardArc

    checkObjResource(data, path);
#line 1604 "GeographyObjMgr.cpp"
    mModelData[seqID] = (J3DModelData *)J3DModelLoaderDataBase::load(data, tevStageNum | 0x19200010);

    if (obj->getKind() >= 0xff && obj->getKind() < 0x10a)
        ExModel::setLightMask(mModelData[seqID], GX_LIGHT1);

    if (obj->mObjFlag & 0x400)
        ExModel::setLightMask(mModelData[seqID], GX_LIGHT7);

    ExModel::patchModelData(mModelData[seqID]);

    if (obj->mObjFlag & 0x100)
        ExModel::setLODBias(mModelData[seqID], TexLODControl::getManager()->getLODBias(TexLODControl::cLODBiasID_Geography));

    RaceMgr::getManager()->getCourse()->setFogInfo(mModelData[seqID]);
    mModelData[seqID]->newSharedDisplayList(0x40000);

    if (obj->mObjFlag & 1)
    {
        mModelData[seqID]->simpleCalcMaterial(0, *(Mtx*)&j3dDefaultMtx);
        mModelData[seqID]->makeSharedDL();
        mSimpleModelFlags[seqID] = 1;
    }

    if (obj->getKind() == 0x26ae)
        mSimpleModelFlags[seqID] = 0;
}

void GeographyObjManager::createModel(JKRSolidHeap *heap, u32 param)
{
    GeographyObj **temp = new GeographyObj *[mGeoObjNum];
    GeographyObj **ptr = temp;
    for (u16 i = 0; i < mGeoObjNum; i++, ptr++)
        *ptr = nullptr;

    SysDebug::getManager()->setHeapGroup("OBJECT MODEL", nullptr);

    for (JSUListIterator<GeoObjSupervisor> svIt(mSupervisorList.getFirst()); svIt.isAvailable(); ++svIt)
        svIt->createModel(heap, param);

    JSUListIterator<GeographyObj> it1(mGeoObjList.getFirst());
    while (it1.isAvailable())
    {
        createModel_norm(it1, temp, heap, param);
        it1.goNext();
    }

    JSUListIterator<GeographyObj> it2(mBoxObjList.getFirst());
    while (it2.isAvailable())
    {
        createModel_norm(it2, temp, heap, param);
        it2.goNext();
    }

    JSUListIterator<GeographyObj> it3(mPrevDrawObjList.getFirst());
    while (it3.isAvailable())
    {
        createModel_norm(it3, temp, heap, param);
        it3.goNext();
    }

    for (JSUListIterator<GeographyObj> genIt(mGenObjList.getFirst()); genIt.isAvailable(); ++genIt)
        createModel_shadow(genIt.getObject(), temp, heap, param);

    for (u8 i = 0; i < 3; i++)
    {
        JSUListIterator<GeographyObj> it(mItemBoxStockList[i].getFirst());
        while (it.isAvailable())
        {
            createModel_norm(it, temp, heap, param);
            it.goNext();
        }
    }

    JSUListIterator<GeographyObj> jugIt(mJugemObjList.getFirst());
    while (jugIt.isAvailable())
    {
        createModel_norm(jugIt, temp, heap, param);
        jugIt.goNext();
    }

    for (JSUListIterator<ExGeographyObj> blIt(mBlockObjList.getFirst()); blIt.isAvailable(); ++blIt)
    {
        blIt->createModel(heap, param);
        makeSharedDL(blIt.getObject(), temp);
        createModel_shadow(blIt.getObject(), temp, heap, param);
    }

    for (JSUListIterator<ExGeographyObj> spIt(mSpObjList.getFirst()); spIt.isAvailable(); ++spIt)
        makeSharedDL(spIt.getObject(), temp);

    delete[] temp;
    stSetRndPermission(1, true);
    resetAllGeoObj();
}

void GeographyObjManager::createModel_norm(const JSUListIterator<GeographyObj> &it, GeographyObj **out, JKRSolidHeap *heap, u32 param)
{
    GeographyObj *obj = it.getObject();
    loadAnimation(obj, out);
    it.getObject()->createModel(heap, param);
    registerAnimation(obj);
    obj->configAnimationMode();
    makeSharedDL(obj, out);
    createModel_shadow(it.getObject(), out, heap, param);
    if (obj->mObjFlag & 4)
        obj->mModel._18 |= 1;
}

void GeographyObjManager::createModel_shadow(GeographyObj *obj, GeographyObj **out, JKRSolidHeap *heap, u32 param)
{
    SysDebug::getManager()->setHeapGroup("OBJECT SHADOW MODEL", nullptr);
    obj->createShadowModel(heap, param);
    makeShadowSharedDL(obj, out);
    SysDebug::getManager()->setHeapGroup("OBJECT MODEL", nullptr);
    obj->lockMdl();
    if (obj->mShadowMdl != nullptr)
        obj->mShadowMdl->getModel()->lock();
    if ((obj->mObjFlag & 8) && obj->mShadowMdl != nullptr)
        obj->mShadowMdl->_18 |= 1;
}

struct GeoObjTransAnmInfo
{
    /* 0x00 */ void *mData;
    /* 0x04 */ J3DAnmTransform *mTrans;
    /* 0x08 */ J3DMtxCalc *mCalc;

    GeoObjTransAnmInfo() // 0x80225e44
    {
        mData = 0;
        mTrans = 0;
        mCalc = 0;
    }
};

struct GeoObjTransAnmData
{
    /* 0x00 */ GeoObjTransAnmInfo *mInfo;
    /* 0x04 */ u8 mNum;
};

template <class T>
struct GeoObjMatAnmInfo
{
    /* 0x00 */ void *mData;
    /* 0x04 */ T *mAnm;
};

struct GeoObjMatAnmData
{
    GeoObjMatAnmData()
    {
        mColor = nullptr;
        mTexSRT = nullptr;
        mTexPattern = nullptr;
        mTevReg = nullptr;
        mInfo.mNumBrks = mInfo.mNumBtps = mInfo.mNumBtks = mInfo.mNumBpks = 0;
    }

    /* 0x00 */ GeoObjMatAnmInfo<J3DAnmColor> *mColor;
    /* 0x04 */ GeoObjMatAnmInfo<J3DAnmTextureSRTKey> *mTexSRT;
    /* 0x08 */ GeoObjMatAnmInfo<J3DAnmTexPattern> *mTexPattern;
    /* 0x0c */ GeoObjMatAnmInfo<J3DAnmTevRegKey> *mTevReg;
    /* 0x10 */ MatAnmInfo mInfo;
};

struct GeoObjAnmData
{
    /* 0x00 */ GeoObjTransAnmData *mTrans;
    /* 0x04 */ GeoObjMatAnmData *mMat;
    /* 0x08 */ u8 mNum;
};

static inline void *getObjAnmResource(GeographyObj *obj, char *path) // fabricated
{
    void *data;
    if (obj->mObjFlag & 0x2)
    {
        data = ObjUtility::getPtrMainArc(path);
        if (data == nullptr)
            data = ObjUtility::getPtrCourseArc(path);
    }
    else
    {
        data = ObjUtility::getPtrCourseArc(path);
        if (data == nullptr)
            data = ObjUtility::getPtrMainArc(path);
        if (data == nullptr)
            data = ObjUtility::getPtrMainLocaleArc(path);
    }

    if (RaceMgr::getManager()->mRaceInfo->mRaceMode == AWARD_DEMO && data == nullptr)
        data = (*(JKRArchive **)((char *)RaceMgr::getManager() + 0x90))->getResource(path); // mAwardArc

    checkObjResource(data, path);
    return data;
}

void GeographyObjManager::loadAnimation(GeographyObj *obj, GeographyObj **loaded)
{
    if (loaded[obj->_110] != nullptr)
        return;

    if (obj->getAnmTbl() == nullptr)
    {
        obj->loadAnimation();
        return;
    }

    u8 transNum;
    MatAnmInfo matInfo;
    transNum = 0;
    matInfo.mNumBrks = matInfo.mNumBtps = matInfo.mNumBtks = matInfo.mNumBpks = 0;
    chkAnmTableNum(obj, &transNum, &matInfo);

    if (transNum != 0)
    {
        if (mSeqObjAnmTbl[obj->_110] == nullptr)
        {
            mSeqObjAnmTbl[obj->_110] = new GeoObjAnmData;
            mSeqObjAnmTbl[obj->_110]->mTrans = nullptr;
            mSeqObjAnmTbl[obj->_110]->mMat = nullptr;
        }
        mSeqObjAnmTbl[obj->_110]->mTrans = new GeoObjTransAnmData;
        mSeqObjAnmTbl[obj->_110]->mTrans->mInfo = new GeoObjTransAnmInfo[transNum];
    }

    if ((u8)(matInfo.mNumBpks + matInfo.mNumBtks + matInfo.mNumBtps + matInfo.mNumBrks) != 0)
    {
        if (mSeqObjAnmTbl[obj->_110] == nullptr)
        {
            mSeqObjAnmTbl[obj->_110] = new GeoObjAnmData;
            mSeqObjAnmTbl[obj->_110]->mTrans = nullptr;
            mSeqObjAnmTbl[obj->_110]->mMat = nullptr;
        }
        mSeqObjAnmTbl[obj->_110]->mMat = new GeoObjMatAnmData();
        GeoObjMatAnmData *mat = mSeqObjAnmTbl[obj->_110]->mMat;
        if (matInfo.mNumBpks != 0)
            mat->mColor = new GeoObjMatAnmInfo<J3DAnmColor>[matInfo.mNumBpks];
        if (matInfo.mNumBtks != 0)
            mat->mTexSRT = new GeoObjMatAnmInfo<J3DAnmTextureSRTKey>[matInfo.mNumBtks];
        if (matInfo.mNumBtps != 0)
            mat->mTexPattern = new GeoObjMatAnmInfo<J3DAnmTexPattern>[matInfo.mNumBtps];
        if (matInfo.mNumBrks != 0)
            mat->mTevReg = new GeoObjMatAnmInfo<J3DAnmTevRegKey>[matInfo.mNumBrks];
    }

    mSeqObjAnmTbl[obj->_110]->mNum = transNum + (u8)(matInfo.mNumBpks + matInfo.mNumBtks + matInfo.mNumBtps + matInfo.mNumBrks);

    GeoAnmTableEntry *tbl = obj->getAnmTbl();
    GeoObjTransAnmData *trans = mSeqObjAnmTbl[obj->_110]->mTrans;
    u8 num = mSeqObjAnmTbl[obj->_110]->mNum;
    GeoObjMatAnmData *mat = mSeqObjAnmTbl[obj->_110]->mMat;
    if (trans != nullptr)
        trans->mNum = 0;
    if (mat != nullptr)
        mat->mInfo.mNumBrks = mat->mInfo.mNumBtps = mat->mInfo.mNumBtks = mat->mInfo.mNumBpks = 0;

    for (u8 i = 0; i < num; i++, tbl++)
    {
        const char *name = tbl->fileName;
        u32 len = strlen(name);
        char buf[64];
        if (obj->mObjFlag & 0x20)
            snprintf(buf, sizeof(buf), "/Objects/%s", tbl->fileName);
        else
            strcpy(buf, tbl->fileName);

        const char *ext = &name[len - 3];
        if (strcmp(ext, "bck") == 0 || strcmp(ext, "bca") == 0)
        {
            GeoObjTransAnmInfo *info = &trans->mInfo[trans->mNum];
            info->mData = getObjAnmResource(obj, buf);
            J3DAnmObjTrans::setupTransAnmData(&info->mTrans, &info->mCalc, obj->mModel.getModelData(), info->mData);
            trans->mNum++;
        }

        if (strcmp(ext, "bpk") == 0)
        {
            GeoObjMatAnmInfo<J3DAnmColor> *info = &mat->mColor[mat->mInfo.mNumBpks];
            info->mData = getObjAnmResource(obj, buf);
            J3DAnmObjMaterial::setupColorAnmData(&info->mAnm, obj->mModel.getModelData(), info->mData);
            mat->mInfo.mNumBpks++;
        }
        else if (strcmp(ext, "btk") == 0)
        {
            GeoObjMatAnmInfo<J3DAnmTextureSRTKey> *info = &mat->mTexSRT[mat->mInfo.mNumBtks];
            info->mData = getObjAnmResource(obj, buf);
            J3DAnmObjMaterial::setupTexSRTAnmData(&info->mAnm, obj->mModel.getModelData(), info->mData);
            mat->mInfo.mNumBtks++;
        }
        else if (strcmp(ext, "btp") == 0)
        {
            GeoObjMatAnmInfo<J3DAnmTexPattern> *info = &mat->mTexPattern[mat->mInfo.mNumBtps];
            info->mData = getObjAnmResource(obj, buf);
            J3DAnmObjMaterial::setupTexPatternAnmData(&info->mAnm, obj->mModel.getModelData(), info->mData);
            mat->mInfo.mNumBtps++;
        }
        else if (strcmp(ext, "brk") == 0)
        {
            GeoObjMatAnmInfo<J3DAnmTevRegKey> *info = &mat->mTevReg[mat->mInfo.mNumBrks];
            info->mData = getObjAnmResource(obj, buf);
            J3DAnmObjMaterial::setupTevRegAnmData(&info->mAnm, obj->mModel.getModelData(), info->mData);
            mat->mInfo.mNumBrks++;
        }
    }
}

void GeographyObjManager::chkAnmTableNum(GeographyObj *obj, u8 *transNum, MatAnmInfo *matInfo)
{
    *transNum = 0;
    matInfo->mNumBpks = matInfo->mNumBtks = matInfo->mNumBtps = matInfo->mNumBrks = 0;

    GeoAnmTableEntry *tbl = obj->getAnmTbl();
    u8 num = obj->getSizeAnmTbl() / sizeof(GeoAnmTableEntry);
    for (u8 i = 0; i < num; i++, tbl++)
    {
        const char *name = tbl->fileName;
        const char *ext = &name[strlen(name) - 3];
        if (strcmp(ext, "bck") == 0 || strcmp(ext, "bca") == 0)
            (*transNum)++;
        else if (strcmp(ext, "bpk") == 0)
            matInfo->mNumBpks++;
        else if (strcmp(ext, "btk") == 0)
            matInfo->mNumBtks++;
        else if (strcmp(ext, "btp") == 0)
            matInfo->mNumBtps++;
        else if (strcmp(ext, "brk") == 0)
            matInfo->mNumBrks++;
    }
}

void GeographyObjManager::registerAnimation(GeographyObj *obj)
{
    if (mSeqObjAnmTbl[(u16)obj->_110] == nullptr)
        return;

    obj->NewAnmCtrl();

    GeoObjTransAnmData *trans = mSeqObjAnmTbl[(u16)obj->_110]->mTrans;
    if (trans != nullptr)
    {
        AnmController *ctrl;
        u8 num = trans->mNum;
        ctrl = obj->mAnmCtrl;
        ctrl->mTrans = new AnmControlTrans();
        ctrl->mTrans->initAnm(num, &obj->mModel);
        GeoObjTransAnmInfo *info = trans->mInfo;
        for (u8 i = 0; i < num; i++, info++)
            obj->mAnmCtrl->RegisterTrans(i, info->mTrans, info->mCalc);
    }

    GeoObjMatAnmData *mat = mSeqObjAnmTbl[(u16)obj->_110]->mMat;
    if (mat != nullptr)
    {
        obj->mAnmCtrl->InitRegistrationMat(mat->mInfo, &obj->mModel);

        u8 num = mat->mInfo.mNumBpks;
        GeoObjMatAnmInfo<J3DAnmColor> *color = mat->mColor;
        for (u8 i = 0; i < num; i++, color++)
            obj->mAnmCtrl->RegisterMatColor(i, color->mAnm);

        num = mat->mInfo.mNumBtks;
        GeoObjMatAnmInfo<J3DAnmTextureSRTKey> *srt = mat->mTexSRT;
        for (u8 i = 0; i < num; i++, srt++)
            obj->mAnmCtrl->RegisterMatTexSRT(i, srt->mAnm);

        num = mat->mInfo.mNumBtps;
        GeoObjMatAnmInfo<J3DAnmTexPattern> *pattern = mat->mTexPattern;
        for (u8 i = 0; i < num; i++, pattern++)
            obj->mAnmCtrl->RegisterMatTexPattern(i, pattern->mAnm);

        num = mat->mInfo.mNumBrks;
        GeoObjMatAnmInfo<J3DAnmTevRegKey> *tevReg = mat->mTevReg;
        for (u8 i = 0; i < num; i++, tevReg++)
            obj->mAnmCtrl->RegisterMatTevReg(i, tevReg->mAnm);
    }
}

void GeographyObjManager::makeSharedDL(GeographyObj *obj, GeographyObj **out)
{
    u32 id = obj->_110;
    if (out[(u16)id] == nullptr)
    {
        obj->makeSharedDL();
        out[(u16)id] = obj;
    }
}

void GeographyObjManager::makeShadowSharedDL(GeographyObj *obj, GeographyObj **out)
{
    u32 id = obj->_110;
    if (obj->mShadowMdl != nullptr && out[id] == nullptr)
        obj->mShadowMdl->makeSharedDL();
}

void GeographyObjManager::getKartCollidePosRadius(JGeometry::TVec3f &pos, f32 &radius, int kartNo)
{
    pos = mKartCollidePos[kartNo];
    radius = mKartCollideRadius[kartNo];
}

void GeographyObjManager::calc()
{
    if (mEnemyMakeCheck)
        ObjUtility::chkAvailableEnemyByDistande(mEnemyAvailable, sNoColComKartToObjectLength);

    for (int i = 0; i < RaceMgr::getManager()->mRaceInfo->mKartNum; i++)
    {
        if (!(KartCtrl::getKartCtrl()->GetGameStatus(i) & 0x2000))
            mEnemyAvailable[i] = false;
    }

    clrKartHittingList();

    for (JSUListIterator<GeoObjSupervisor> it(mSupervisorList.getFirst()); it.isAvailable(); ++it)
        it->calc();

    for (JSUListIterator<GeographyObj> it(mGenObjList.getFirst()); it.isAvailable(); ++it)
    {
        it->calc();
        it->calcBoundsGlPos();
    }

    for (JSUListIterator<GeographyObj> it(mGeoObjList.getFirst()); it.isAvailable(); ++it)
        Calc_norm(it);
    for (JSUListIterator<GeographyObj> it(mBoxObjList.getFirst()); it.isAvailable(); ++it)
        Calc_norm(it);
    for (JSUListIterator<GeographyObj> it(mPrevDrawObjList.getFirst()); it.isAvailable(); ++it)
        Calc_norm(it);
    for (JSUListIterator<GeographyObj> it(mJugemObjList.getFirst()); it.isAvailable(); ++it)
        Calc_norm(it);

    for (JSUListIterator<ExGeographyObj> it(mBlockObjList.getFirst()); it.isAvailable(); ++it)
        it->calc();
    for (JSUListIterator<ExGeographyObj> it(mSpObjList.getFirst()); it.isAvailable(); ++it)
        it->calc();

    for (int i = 0; i < RaceMgr::getManager()->mRaceInfo->mKartNum; i++)
    {
        KartCtrl::getKartCtrl()->GetBodyPos(i, &mKartCollidePos[i]);

        const Mtx &bodyMtx = ObjUtility::getKartBodyMtx(i);
        JGeometry::TVec3f up(bodyMtx[0][1], bodyMtx[1][1], bodyMtx[2][1]);
        JGeometry::TVec3f fwd(bodyMtx[0][2], bodyMtx[1][2], bodyMtx[2][2]);

        EKartID kartType = RaceMgr::getManager()->mRaceInfo->getKartInfo(i)->getKartID();
#line 2176 "GeographyObjMgr.cpp"
        JUT_MINMAX_ASSERT(0, kartType, 21);
#line 2177 "GeographyObjMgr.cpp"
        up *= sKartBoundOffsetY[kartType];
        fwd *= sKartBoundOffsetZ[kartType];
        mKartCollidePos[i] += up;
        mKartCollidePos[i] += fwd;
        mKartCollideRadius[i] = sKartBoundRadius[kartType];
    }

    for (JSUListIterator<GeographyObj> it(mGeoObjList.getFirst()); it.isAvailable(); ++it)
    {
        it->clrAllIsHitKartFlg();
        it->mGeoObjFlag &= ~4;
        it->mColItemObj = nullptr;
    }
    for (JSUListIterator<GeographyObj> it(mGenObjList.getFirst()); it.isAvailable(); ++it)
    {
        it->clrAllIsHitKartFlg();
        it->mGeoObjFlag &= ~4;
        it->mColItemObj = nullptr;
    }
    for (JSUListIterator<GeographyObj> it(mJugemObjList.getFirst()); it.isAvailable(); ++it)
        it->clrAllIsHitKartFlg();

    for (int i = 0; i < RaceMgr::getManager()->mRaceInfo->mKartNum; i++)
    {
        JGeometry::TVec3f pos = mKartCollidePos[i];
        f32 radius = mKartCollideRadius[i];
        for (JSUListIterator<GeographyObj> it(mBoxObjList.getFirst()); it.isAvailable(); ++it)
            Collide_norm(it, i, pos, radius);

        if (IsCheckKartToObject(i))
        {
            for (JSUListIterator<GeographyObj> it(mGeoObjList.getFirst()); it.isAvailable(); ++it)
                Collide_norm(it, i, pos, radius);
            for (JSUListIterator<GeographyObj> it(mGenObjList.getFirst()); it.isAvailable(); ++it)
                Collide_norm(it, i, pos, radius);
        }
    }
}

bool GeographyObjManager::IsCheckKartToObject(const int kart_index) const
{
#line 2359 "GeographyObjMgr.cpp"
    JUT_MINMAX_ASSERT(0, kart_index, 8);
    return mEnemyAvailable[kart_index];
}

void GeographyObjManager::Collide_norm(const JSUListIterator<GeographyObj> &it, const int &kartNo, const JGeometry::TVec3f &pos, const f32 &radius)
{
    if (!(it->mCheckKartHitFlags & (1 << kartNo)))
        return;
    if (it->mObjFlag & 0x80)
        return;
    if (it->IsHitKart(pos, radius, &mKartHitRefVec[kartNo]))
    {
        it->setIsHitKartFlg(kartNo);
        it->doKartColCallBack(kartNo);
        mKartHittingList[kartNo][it->_58] = it.getObject();
        mKartHitDepth[kartNo] = PSVECMag(&mKartHitRefVec[kartNo]);
    }
    else
    {
        it->clrIsHitKartSoundFlg(kartNo);
    }
}

void GeographyObjManager::Calc_norm(const JSUListIterator<GeographyObj> &it)
{
    it->calc_norm();
    if (it->mGeoObjFlag & 0x10)
    {
        it->mModel.hide();
        it->clrObjFlagCheckGeoHitting();
        it->clrObjFlagCheckItemHitting();
        it->clrAllCheckKartHitFlag();
    }
}

static inline void loadShadowModelData(GeographyObjManager *mgr, GeographyObj *obj)
{
    void *data;
    u32 seqIdx = (u16)obj->_110;
    if (mgr->mShadowModelData[seqIdx] == nullptr)
    {
        const char *name = obj->getShadowBmdFileName();
        if (name != nullptr)
        {
            char buf[64];
            char *path;
            if (obj != nullptr && obj->mObjFlag & 0x20)
            {
                snprintf(buf, sizeof(buf), "/Objects/%s", name);
                path = buf;
            }
            else
            {
                path = (char *)name;
            }
            if (obj->mObjFlag & 0x2)
            {
                data = ObjUtility::getPtrMainArc(path);
                if (data == nullptr)
                    data = ObjUtility::getPtrCourseArc(path);
            }
            else
            {
                data = ObjUtility::getPtrCourseArc(path);
                if (data == nullptr)
                    data = ObjUtility::getPtrMainArc(path);
                if (data == nullptr)
                    data = ObjUtility::getPtrMainLocaleArc(path);
            }

            if (RaceMgr::getManager()->mRaceInfo->mRaceMode == AWARD_DEMO && data == nullptr)
                data = (*(JKRArchive **)((char *)RaceMgr::getManager() + 0x90))->getResource(path); // mAwardArc

            checkObjResource(data, path);
#line 1604 "GeographyObjMgr.cpp"
            mgr->mShadowModelData[seqIdx] = (J3DModelData *)J3DModelLoaderDataBase::load(data, 0x19210010);
            ExModel::patchModelData(mgr->mShadowModelData[seqIdx]);
            mgr->mShadowModelData[seqIdx]->newSharedDisplayList(0x40000);
        }
    }
}

template <>
void GeographyObjManager::loadObjects(JSUList<ExGeographyObj> *list, JSUListIterator<ExGeographyObj> &iterator);
template <>
void GeographyObjManager::loadObjects(JSUList<GeographyObj> *list, JSUListIterator<GeographyObj> &iterator);

void GeographyObjManager::load()
{
    JSUListIterator<GeographyObj> it;
    JSUListIterator<ExGeographyObj> exIt;
    loadObjects(&mGeoObjList, it);
    for (u8 i = 0; i < 3; i++)
        loadObjects(&mItemBoxStockList[i], it);
    loadObjects(&mBoxObjList, it);
    loadObjects(&mPrevDrawObjList, it);
    loadObjects(&mJugemObjList, it);

    for (it = mGenObjList.getFirst(); it.isAvailable(); ++it)
    {
        loadShadowModelData(this, it.getObject());
        it->loadShadowModel(getShadowModelData((u16)it->_110), it->getKind());
    }

    loadObjects(&mBlockObjList, exIt);

    for (JSUListIterator<GeoObjSupervisor> svIt(mSupervisorList.getFirst()); svIt.isAvailable(); ++svIt)
        svIt->load();

    JSUListIterator<ExGeographyObj> spIt;
    for (spIt = mSpObjList.getFirst(); spIt.isAvailable(); ++spIt)
    {
        setModelData(spIt.getObject());
        spIt.getObject()->loadmodel(getModelData((u16)spIt.getObject()->_110));
    }
}

template <>
void GeographyObjManager::loadObjects(JSUList<ExGeographyObj> *list, JSUListIterator<ExGeographyObj> &iterator)
{
    for (iterator = list->getFirst(); iterator.isAvailable(); ++iterator)
    {
        setModelData(iterator.getObject());
        GeographyObj *obj = iterator.getObject();
        obj->loadmodel(getModelData(obj->_110));
        loadShadowModelData(this, iterator.getObject());
        iterator.getObject()->loadShadowModel(getShadowModelData((u16)iterator.getObject()->_110), iterator.getObject()->getKind());
    }
}

template <>
void GeographyObjManager::loadObjects(JSUList<GeographyObj> *list, JSUListIterator<GeographyObj> &iterator)
{
    for (iterator = list->getFirst(); iterator.isAvailable(); ++iterator)
    {
        setModelData(iterator.getObject());
        GeographyObj *obj = iterator.getObject();
        obj->loadmodel(getModelData(obj->_110));
        loadShadowModelData(this, iterator.getObject());
        iterator->loadShadowModel(getShadowModelData((u16)iterator->_110), iterator->getKind());
    }
}

J3DModelData *GeographyObjManager::getModelData(u32 num)
{
    return mModelData[num];
}

J3DModelData *GeographyObjManager::getShadowModelData(u32 num)
{
    return mShadowModelData[num];
}

void GeographyObjManager::dispose()
{
    gMgr = nullptr;
}

void GeographyObjManager::reset(const CrsData &crsData)
{
    ((GeoItemBox *)this)->setInactiveTime();

    for (JSUListIterator<GeographyObj> it(mBoxObjList.getFirst()); it.isAvailable();)
    {
        GeographyObj *obj = it.getObject();
        ++it;
        switch (obj->getKind())
        {
        case 1:
            boxremove(obj, 0);
            break;
        case 0xc:
            boxremove(obj, 1);
            break;
        }
    }

    clrKartHittingList();

    setObjectByCrsData(crsData);

    mEnemyMakeCheck = 0;
    for (int i = 0; i < 8; i++)
        mEnemyAvailable[i] = true;

    RaceInfo *raceInfo = RaceMgr::getManager()->mRaceInfo;
    if (raceInfo->mKartNum - raceInfo->mPlayerNum > 0)
        mEnemyMakeCheck = true;

    raceInfo = RaceMgr::getManager()->mRaceInfo;
    if (raceInfo->mKartNum == raceInfo->mPlayerNum)
        mEnemyMakeCheck = false;
}

void GeographyObjManager::append(GeographyObj *obj)
{
    mGeoObjList.append(&obj->mObjLink);
    obj->mModel.show();
}

void GeographyObjManager::prevdraw_append(GeographyObj *obj)
{
    mPrevDrawObjList.append(&obj->mObjLink);
}

void GeographyObjManager::genappend(GeographyObj *obj)
{
    mGenObjList.append(&obj->mObjLink);
}

void GeographyObjManager::jugemappend(GeographyObj *obj)
{
    mJugemObjList.append(&obj->mObjLink);
    obj->mModel.show();
}

void GeographyObjManager::blockappend(ExGeographyObj *obj)
{
    mBlockObjList.append(&obj->mExLinks);
}

void GeographyObjManager::spappend(ExGeographyObj *obj)
{
    mSpObjList.append(&obj->mExLinks);
}

void GeographyObjManager::boxappend(GeographyObj *obj)
{
    mBoxObjList.append(&obj->mObjLink);
    obj->mModel.show();
}

void GeographyObjManager::boxremove(GeographyObj *obj, u32 index)
{
    mBoxObjList.remove(&obj->mObjLink);
    mItemBoxStockList[index].append(&obj->mObjLink);
    if (obj->getShadowModel() != nullptr)
        obj->hideAllShadow();
}

void GeographyObjManager::update()
{
    for (JSUListIterator<GeographyObj> it(mGeoObjList.getFirst()); it.isAvailable(); ++it)
        it->update_norm();

    for (JSUListIterator<GeographyObj> it(mBoxObjList.getFirst()); it.isAvailable(); ++it)
        it->update_norm();
}

void GeographyObjManager::update_foreground()
{
    for (JSUListIterator<GeographyObj> it(mJugemObjList.getFirst()); it.isAvailable(); ++it)
        it->update_norm();
}

void GeographyObjManager::update_groundObj()
{
    for (JSUListIterator<GeographyObj> it(mPrevDrawObjList.getFirst()); it.isAvailable(); ++it)
        it->update();

    for (JSUListIterator<ExGeographyObj> it(mBlockObjList.getFirst()); it.isAvailable(); ++it)
        it->update();

    for (JSUListIterator<ExGeographyObj> it(mSpObjList.getFirst()); it.isAvailable(); ++it)
        it->update();
}

void GeographyObjManager::viewCalc(u32 viewNo)
{
    J3DUClipper *clipper = KartCtrl::getKartCtrl()->getKartCam(viewNo)->GetClipper();
    const Mtx &viewMtx = j3dSys.getViewMtx();
    JSUList<GeographyObj> *lists[2] = { &mGeoObjList, &mBoxObjList };

    for (u8 i = 0; i < 2; i++)
    {
        for (JSUListIterator<GeographyObj> it(lists[i]->getFirst()); it.isAvailable(); ++it)
        {
            GeographyObj *obj = it.getObject();
            if (obj->tstObjFlagHidding())
            {
                obj->mModel.hide();
            }
            else if (!(obj->mObjFlag & 0x200))
            {
                f32 scale = obj->getAxisMaxScale();
                obj->mModel.clipBySphere(viewNo, clipper, viewMtx, scale);
            }
            it->viewCalc(viewNo);
        }
    }
}

void GeographyObjManager::viewCalc_foreground(u32 viewNo)
{
    J3DUClipper *clipper = KartCtrl::getKartCtrl()->getKartCam(viewNo)->GetClipper();
    const Mtx &viewMtx = j3dSys.getViewMtx();
    for (JSUListIterator<GeographyObj> it(mJugemObjList.getLast()); it.isAvailable(); --it)
    {
        GeographyObj *obj = it.getObject();
        if (obj->tstObjFlagHidding())
        {
            obj->mModel.hide();
        }
        else if (!(obj->mObjFlag & 0x200))
        {
            f32 scale = obj->getAxisMaxScale();
            obj->mModel.clipBySphere(viewNo, clipper, viewMtx, scale);
        }
        it->viewCalc(viewNo);
    }
}

void GeographyObjManager::viewCalc_groundObj(u32 viewNo)
{
    J3DUClipper *clipper = KartCtrl::getKartCtrl()->getKartCam(viewNo)->GetClipper();
    const Mtx &viewMtx = j3dSys.getViewMtx();
    for (JSUListIterator<GeographyObj> it(mPrevDrawObjList.getFirst()); it.isAvailable(); ++it)
    {
        GeographyObj *obj = it.getObject();
        if (obj->tstObjFlagHidding())
        {
            obj->mModel.hide();
        }
        else if (!(obj->mObjFlag & 0x200))
        {
            f32 scale = obj->getAxisMaxScale();
            obj->mModel.clipBySphere(viewNo, clipper, viewMtx, scale);
        }
        it->viewCalc(viewNo);
    }

    for (JSUListIterator<ExGeographyObj> it(mBlockObjList.getFirst()); it.isAvailable(); ++it)
    {
        ExGeographyObj *obj = it.getObject();
        if (obj->tstObjFlagHidding())
        {
            obj->mModel.hide();
        }
        else if (!(obj->mObjFlag & 0x200))
        {
            f32 scale = obj->getAxisMaxScale();
            obj->mModel.clipBySphere(viewNo, clipper, viewMtx, scale);
        }
        it->viewCalc(viewNo);
    }

    for (JSUListIterator<ExGeographyObj> it(mSpObjList.getFirst()); it.isAvailable(); ++it)
        it->viewCalc(viewNo);
}

void GeographyObjManager::setCurrentViewNo(u32 viewNo)
{
    for (JSUListIterator<GeographyObj> it(mGeoObjList.getFirst()); it.isAvailable(); ++it)
        it->setCurrentViewNo(viewNo);

    for (JSUListIterator<GeographyObj> it(mBoxObjList.getFirst()); it.isAvailable(); ++it)
        it->setCurrentViewNo(viewNo);
}

void GeographyObjManager::setCurrentViewNo_foreground(u32 viewNo)
{
    for (JSUListIterator<GeographyObj> it(mJugemObjList.getFirst()); it.isAvailable(); ++it)
        it->setCurrentViewNo(viewNo);
}

void GeographyObjManager::setCurrentViewNo_groundObj(u32 viewNo)
{
    for (JSUListIterator<GeographyObj> it(mPrevDrawObjList.getFirst()); it.isAvailable(); ++it)
        it->setCurrentViewNo(viewNo);

    for (JSUListIterator<ExGeographyObj> it(mBlockObjList.getFirst()); it.isAvailable(); ++it)
        it->setCurrentViewNo(viewNo);

    for (JSUListIterator<ExGeographyObj> it(mSpObjList.getFirst()); it.isAvailable(); ++it)
        it->setCurrentViewNo(viewNo);
}

ExGeographyObj *GeographyObjManager::search_boundEx(const JGeometry::TVec3f &pos, GeographyObj *searcher)
{
    for (JSUListIterator<ExGeographyObj> it(mBlockObjList.getFirst()); it.isAvailable(); ++it)
    {
        ExGeographyObj *obj = it.getObject();
        if (obj == searcher)
            continue;
        if (obj->getExBounds()->IsHitBound(obj->mPos, pos))
            return it.getObject();
    }
    return nullptr;
}

ExGeographyObj *GeographyObjManager::search_boundSp(const JGeometry::TVec3f &pos, GeographyObj *searcher)
{
    for (JSUListIterator<ExGeographyObj> it(mSpObjList.getFirst()); it.isAvailable(); ++it)
    {
        if (it.getObject() != searcher && it->Search_Bound(pos))
            return it.getObject();
    }
    return nullptr;
}

ExGeographyObj *GeographyObjManager::search_boundSp(const JGeometry::TVec3f &pos, f32 radius)
{
    for (JSUListIterator<ExGeographyObj> it(mSpObjList.getFirst()); it.isAvailable(); ++it)
    {
        if (it->Search_BoundRadius(pos, radius))
            return it.getObject();
    }
    return nullptr;
}

void GeographyObjManager::drawPrim(u32 cameraNo)
{
#line 262 "GeographyObjMgr.cpp"
    JUT_MINMAX_ASSERT(0, cameraNo, 4);
}

void GeographyObjManager::drawSimpleModel(u32 viewNo)
{
    SimpleDrawer drawer;
    for (u16 i = 0; i < GeoFuncTableMax; i++)
    {
        if (mSimpleModelFlags[i] != 0 && mModelData[i] != nullptr)
        {
            drawer.drawInit(mModelData[i]);
            while (drawer.loadPreDrawSetting())
            {
                for (JSUListIterator<GeographyObj> it(mGeoObjList.getFirst()); it.isAvailable(); ++it)
                {
                    GeographyObj *obj = it.getObject();
                    if (i == obj->_110)
                        obj->simpleDraw(viewNo);
                }
                for (JSUListIterator<GeographyObj> it(mBoxObjList.getFirst()); it.isAvailable(); ++it)
                {
                    GeographyObj *obj = it.getObject();
                    if (i == obj->_110)
                        obj->simpleDraw(viewNo);
                }
            }
        }
    }
}

void GeographyObjManager::drawSimpleModel_groundObj(u32 viewNo)
{
    SimpleDrawer drawer;
    for (u16 i = GeoFuncTableMax; i < (GeoFuncTableMax + GeoPrevDrawFuncTableMax); i++)
    {
        if (mSimpleModelFlags[i] != 0 && mModelData[i] != nullptr)
        {
            drawer.drawInit(mModelData[i]);
            while (drawer.loadPreDrawSetting())
            {
                for (JSUListIterator<GeographyObj> it(mPrevDrawObjList.getFirst()); it.isAvailable(); ++it)
                {
                    if (i == it->_110)
                        it->simpleDraw(viewNo);
                }
            }
        }
    }

    for (JSUListIterator<ExGeographyObj> it(mSpObjList.getFirst()); it.isAvailable(); ++it)
        it->simpleDraw(viewNo);
}

void GeographyObjManager::drawGXObj(u32 cameraNo)
{
    if (TTestString::sSupervisor != nullptr)
        TTestString::sSupervisor->drawAll(cameraNo);

    if (TBalloonString::sSupervisor != nullptr)
        TBalloonString::sSupervisor->drawString(cameraNo);
}

TJugem *GeographyObjManager::getJugem(int camera_index) const
{
#line 3235 "GeographyObjMgr.cpp"
    JUT_MINMAX_ASSERT(0, camera_index, mJugemNum);
    return mJugem[camera_index];
}

TBalloonManager *GeographyObjManager::getBalloonMgr(int kart_index) const
{
#line 3248 "GeographyObjMgr.cpp"
    JUT_MINMAX_ASSERT(0, kart_index, 8);
    return mBalloonMgr[kart_index];
}
