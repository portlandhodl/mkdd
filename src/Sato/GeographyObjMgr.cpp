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

class TMapObjGeyser : public GeographyObj
{
public:
    TMapObjGeyser(const CrsData::SObject &);
    virtual ~TMapObjGeyser() {}
    void calc() {}

private:
    u8 _stub[0x7c]; // 0x1c8
};

class TMapObjGeyserSearch : public GeographyObj
{
public:
    TMapObjGeyserSearch(const CrsData::SObject &);
    virtual ~TMapObjGeyserSearch() {}
    void calc() {}

private:
    u8 _stub[0x7c]; // 0x1c8
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

class TMapObjMoveItemBoxLimit : public GeographyObj
{
public:
    TMapObjMoveItemBoxLimit(const CrsData::SObject &);
    virtual ~TMapObjMoveItemBoxLimit() {}
    void calc() {}

private:
    u8 _stub[0xec]; // 0x238
};

class TMapObjJetCoasterHead : public GeographyObj
{
public:
    TMapObjJetCoasterHead(const CrsData::SObject &);
    virtual ~TMapObjJetCoasterHead() {}
    void calc() {}

private:
    u8 _stub[0x2c]; // 0x178
};

class TMapObjJetCoasterBody : public GeographyObj
{
public:
    TMapObjJetCoasterBody(u32 = 0);
    virtual ~TMapObjJetCoasterBody() {}
    void calc() {}

private:
    u8 _stub[0x1c]; // 0x168
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

class TMapObjKpFire : public GeographyObj
{
public:
    TMapObjKpFire(const CrsData::SObject &);
    virtual ~TMapObjKpFire() {}
    void calc() {}

private:
    u8 _stub[0x58]; // 0x1a4
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

class TMapObjMoveItemBox : public GeographyObj
{
public:
    TMapObjMoveItemBox(const CrsData::SObject &);
    virtual ~TMapObjMoveItemBox() {}
    void calc() {}

private:
    u8 _stub[0xd4]; // 0x220
};

class TMapObjNossie : public GeographyObj
{
public:
    TMapObjNossie(const CrsData::SObject &);
    virtual ~TMapObjNossie() {}
    void calc() {}

private:
    u8 _stub[0x60]; // 0x1ac
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

class TMapObjUkleleMonte : public GeographyObj
{
public:
    TMapObjUkleleMonte(const CrsData::SObject &);
    virtual ~TMapObjUkleleMonte() {}
    void calc() {}

private:
    u8 _stub[0x24]; // 0x170
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

class TMapObjSwimNossie : public GeographyObj
{
public:
    TMapObjSwimNossie(const CrsData::SObject &);
    virtual ~TMapObjSwimNossie() {}
    void calc() {}

private:
    u8 _stub[0x24]; // 0x170
};

class TMapObjPtera : public GeographyObj
{
public:
    TMapObjPtera(const CrsData::SObject &);
    virtual ~TMapObjPtera() {}
    void calc() {}

private:
    u8 _stub[0x3c]; // 0x188
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

class TShineArrow : public GeographyObj
{
public:
    TShineArrow();
    virtual ~TShineArrow() {}
    void calc() {}

private:
    u8 _stub[0xe4]; // 0x230
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
    for (int i = 0; i < 8; i++)
    {
        for (u8 j = 0; j < 20; j++)
        {
            mKartHittingList[i][j] = nullptr;
        }
        mKartHitRefVec[i].zero();
        mKartHitItemBox[i] = false;
        mKartHitDepth[i] = 0.0f;
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
    if (mIsLoaded)
        state = 2;
    else if (mNormObjNum != 0)
        state = 1;
    else
        state = 0;

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
    if (mIsLoaded)
        state = 2;
    else if (mNormObjNum != 0)
        state = 1;
    else
        state = 0;

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

    RaceInfo *raceInfo = RaceMgr::getManager()->mRaceInfo;
    for (u8 kartIdx = 0; kartIdx < (u32)mJugemNum; kartIdx++)
    {
        mJugem[kartIdx] = nullptr;

        TJugem *jugem = nullptr;
        TJugemHeadHear *headHear = nullptr;
        TJugemHeadLamp *headLamp = nullptr;
        TJugemVoidRod *voidRod = nullptr;
        TJugemFlag *flag = nullptr;
        TJugemRodPukuPuku *rodPuku = nullptr;
        TJugemRodBoardRev *rodRev = nullptr;
        TJugemRodBoard2 *rodB2 = nullptr;
        TJugemRodBoard *rodB1 = nullptr;

        CrsData::SObject *object = crsData.getObjectTable();
        for (int kind = 0x109; kind >= 0xff; kind--)
        {
            GeographyObj *obj = createObj(kind, object);
            if (obj == nullptr)
                continue;

            switch (obj->getKind())
            {
            case 0x101:
                voidRod = (TJugemVoidRod *)obj;
                voidRod->setJugemRodItem(rodRev, 4);
                voidRod->setJugemRodItem(rodPuku, 3);
                voidRod->setJugemRodItem(rodB1, 1);
                voidRod->setJugemRodItem(rodB2, 2);
                break;
            case 0x104:
                rodB1 = (TJugemRodBoard *)obj;
                break;
            case 0x105:
                rodB2 = (TJugemRodBoard2 *)obj;
                break;
            case 0x106:
                rodRev = (TJugemRodBoardRev *)obj;
                break;
            case 0x107:
                rodPuku = (TJugemRodPukuPuku *)obj;
                break;
            case 0x102:
                flag = (TJugemFlag *)obj;
                break;
            case 0x100:
                headHear = (TJugemHeadHear *)obj;
                break;
            case 0x109:
                headLamp = (TJugemHeadLamp *)obj;
                break;
            case 0xff:
                jugem = (TJugem *)obj;
                jugem->setJugemItem(voidRod, 1);
                *(void **)((char *)jugem + 0x26c) = *(void **)((char *)voidRod + 0x190); // TODO: name these members
                jugem->setJugemItem(flag, 2);
                jugem->setJugemHeadItem(headHear, 0);
                jugem->setCameraNum(kartIdx);
#line 283 "GeographyObjMgr.cpp"
                JUT_MINMAX_ASSERT(0, kartIdx, 4);
#line 284 "GeographyObjMgr.cpp"
                jugem->setKartNum((u8)raceInfo->mTargetKarts[kartIdx]);
                jugem->setJugemHeadItem(headLamp, 1);
                mJugem[kartIdx] = jugem;
                break;
            }
        }

        for (int kind = 0x109; kind >= 0xff; kind--)
        {
            jugemappend(mJugem[kartIdx]);
            jugemappend(headHear);
            jugemappend(headLamp);
            jugemappend(voidRod);
            jugemappend(flag);
            jugemappend(rodPuku);
            jugemappend(rodB2);
            jugemappend(rodB1);
            jugemappend(rodRev);
        }
    }
}

void GeographyObjManager::setSequentialID()
{
    GeoFuncTableMax = 132;
    GeoPrevDrawFuncTableMax = 2;

    u16 sum = GeoFuncTableMax + GeoPrevDrawFuncTableMax;
    mSeqObjAnmTbl = new GeoObjSupervisor **[sum];

    u16 seq = 0;
    GeoFuncTable::GeoFuncTblEntry *entry = GeoFuncTable::GeoFuncTbl;
    for (u16 i = 0; i < sum; i++, entry++)
    {
        entry->sequentialID = seq;
        mSeqObjAnmTbl[i] = nullptr;
        seq++;
    }

    GenGeoFuncTableMax = 24;
    GeoFuncTable::GeoFuncTblEntry *genEntry = GeoFuncTable::GenGeoFuncTbl;
    for (u16 i = 0; i < GenGeoFuncTableMax; i++, genEntry++)
    {
        genEntry->sequentialID = seq;
        seq++;
    }

    mKindObjTbl = new GeographyObj *[(u16)(GeoFuncTableMax + GeoPrevDrawFuncTableMax + GenGeoFuncTableMax)];
    for (u16 i = 0; i < (u16)(GeoFuncTableMax + GeoPrevDrawFuncTableMax + GenGeoFuncTableMax); i++)
    {
        mKindObjTbl[i] = nullptr;
    }

    BlockGeoFuncTableMax = 1;
    GeoFuncTable::GeoExFuncTblEntry *blockEntry = GeoFuncTable::BlockGeoFuncTbl;
    for (u8 i = 0; i < BlockGeoFuncTableMax; i++, blockEntry++)
    {
        blockEntry->sequentialID = seq;
        seq++;
    }

    SpecialGeoFuncTableMax = 1;
    GeoFuncTable::GeoExFuncTblEntry *specialEntry = GeoFuncTable::SpecialGeoFuncTbl;
    for (u8 i = 0; i < SpecialGeoFuncTableMax; i++, specialEntry++)
    {
        specialEntry->sequentialID = seq;
        seq++;
    }

    mGeoObjNum = (u16)(sum + GenGeoFuncTableMax + BlockGeoFuncTableMax + SpecialGeoFuncTableMax);
}

void GeographyObjManager::countObjNum(const CrsData::SObject *obj)
{
    GeoFuncTable::GeoFuncTblEntry *entry = GeoFuncTable::GeoFuncTbl;
    int kind = obj->objID;
    u16 i = 0;
    while (i < GeoFuncTableMax)
    {
        if (entry->kind == kind)
        {
            if (kind == 1)
                mItemBoxObjNum++;
            mNormObjNum++;
            return;
        }
        i++;
        entry++;
    }

    entry = GeoFuncTable::GeoPrevDrawFuncTbl;
    i = 0;
    while (i < GeoPrevDrawFuncTableMax)
    {
        if (entry->kind == kind)
        {
            mPrevDrawGeoObjNum++;
            return;
        }
        i++;
        entry++;
    }

    entry = GeoFuncTable::GenGeoFuncTbl;
    i = 0;
    while (i < GenGeoFuncTableMax)
    {
        if (entry->kind == kind)
        {
            mGenGeoObjNum++;
            return;
        }
        i++;
        entry++;
    }

    GeoFuncTable::GeoExFuncTblEntry *exEntry = GeoFuncTable::BlockGeoFuncTbl;
    i = 0;
    while (i < BlockGeoFuncTableMax)
    {
        if (exEntry->kind == kind)
        {
            mBlockGeoObjNum++;
            return;
        }
        i++;
        exEntry++;
    }

    exEntry = GeoFuncTable::SpecialGeoFuncTbl;
    i = 0;
    while (i < SpecialGeoFuncTableMax)
    {
        if (exEntry->kind == kind)
        {
            mSpecialGeoObjNum++;
            return;
        }
        i++;
        exEntry++;
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
    GeographyObj *obj = nullptr;
    GeoFuncTable::GeoFuncTblEntry *entry = GeoFuncTable::GeoFuncTbl;
    for (u16 i = 0; i < GeoFuncTableMax; i++, entry++)
    {
        if (entry->kind == kind)
        {
            obj = entry->createFunc(*(const CrsData::SObject *)nullptr);
            obj->_110 = entry->sequentialID;
        }
    }
    if (obj != nullptr)
        append(obj);
    return obj;
}

GeographyObj *GeographyObjManager::createPrevDrawSubObj(u32 kind)
{
    GeographyObj *obj = nullptr;
    GeoFuncTable::GeoFuncTblEntry *entry = GeoFuncTable::GeoPrevDrawFuncTbl;
    for (u16 i = 0; i < GeoPrevDrawFuncTableMax; i++, entry++)
    {
        if (entry->kind == kind)
        {
            obj = entry->createFunc(*(const CrsData::SObject *)nullptr);
            obj->_110 = entry->sequentialID;
        }
    }
    if (obj != nullptr)
        prevdraw_append(obj);
    return obj;
}

GeographyObj *GeographyObjManager::createGenSubObj(u32 kind)
{
    GeoFuncTable::GeoFuncTblEntry *entry = GeoFuncTable::GenGeoFuncTbl;
    GeographyObj *obj = nullptr;
    for (int i = GenGeoFuncTableMax; i != 0; i--)
    {
        if (entry->kind == kind)
        {
            obj = entry->createFunc(*(const CrsData::SObject *)nullptr);
            obj->_110 = entry->sequentialID;
            break;
        }
        entry++;
    }
    if (obj != nullptr)
    {
        genappend(obj);
        obj->createColModel(nullptr);
    }
    return obj;
}

void GeographyObjManager::checkGenObjList(const CrsData &)
{
}

GeographyObj *GeographyObjManager::createAppendGenObj(u32, const CrsData::SObject *)
{
    return nullptr;
}

void GeographyObjManager::createAllBalloonObj()
{
    u8 teamNum = 1;
    if (mIsBalloonBattle)
        teamNum = 3;

    RaceInfo *raceInfo = RaceMgr::getManager()->mRaceInfo;
#line 1364 "GeographyObjMgr.cpp"
    u8 playerNum = (u8)raceInfo->mPlayerNum;
    JUT_ASSERT_MSG(playerNum > 0, "playernum > 0");

    u8 map[8];
    u8 *mapEnd = map;
    int i;
    for (i = 8; i != 0; i--, mapEnd++)
        *mapEnd = -1;

    for (u8 i = 0; i < raceInfo->mConsoleNum; i++)
    {
#line 283 "GeographyObjMgr.cpp"
        JUT_MINMAX_ASSERT(0, i, 4);
#line 1390 "GeographyObjMgr.cpp"
        map[raceInfo->mTargetKarts[i]] = i;
    }

    for (u8 i = 0; i < playerNum; i++)
    {
        GeographyObj *mgr = nullptr;
        GeoFuncTable::GeoFuncTblEntry *entry = GeoFuncTable::GenGeoFuncTbl;
        for (int j = GenGeoFuncTableMax; j != 0; j--)
        {
            if (entry->kind == 0x26af)
            {
                mgr = entry->createFunc(*(const CrsData::SObject *)nullptr);
                mgr->_110 = entry->sequentialID;
                break;
            }
            entry++;
        }

        if (mgr != nullptr)
        {
            genappend(mgr);
            appendKindObjList(mgr);
            mgr->createColModel(nullptr);
            mBalloonMgr[i] = (TBalloonManager *)mgr;
            ((TBalloonManager *)mgr)->init(i, map[i], teamNum);
        }
    }
}

void GeographyObjManager::setObjectByCrsData(const CrsData &)
{
}

void GeographyObjManager::setGeoObjNum(const CrsData &)
{
}

void GeographyObjManager::setModelData(GeographyObj *obj)
{
    u16 seqID = obj->_110;
    if (mModelData[seqID] != nullptr)
        return;

    u32 tevStageNum = obj->getJ3DModelDataTevStageNum();
    const char *name = obj->getBmdFileName();

    char buf[64];
    const char *path = name;
    if (obj != nullptr && obj->mObjFlag & 0x20)
    {
        snprintf(buf, sizeof(buf), "/Objects/%s", name);
        path = buf;
    }

    void *data;
    if (obj->mObjFlag & 0x2)
    {
        data = ObjUtility::getPtrMainArc((char *)path);
        if (data == nullptr)
            data = ObjUtility::getPtrCourseArc((char *)path);
    }
    else
    {
        data = ObjUtility::getPtrCourseArc((char *)path);
        if (data == nullptr)
            data = ObjUtility::getPtrMainArc((char *)path);
        if (data == nullptr)
            data = ObjUtility::getPtrMainLocaleArc((char *)path);
    }

    if (RaceMgr::getManager()->mRaceInfo->mRaceMode == AWARD_DEMO && data == nullptr)
        data = (*(JKRArchive **)((char *)RaceMgr::getManager() + 0x90))->getResource(path); // mAwardArc

    if (data == nullptr)
        OSReport("not found %s\n", name);

#line 1603 "GeographyObjMgr.cpp"
    JUT_ASSERT_F(data != nullptr, "can't find %s", name);
#line 1604 "GeographyObjMgr.cpp"
    mModelData[seqID] = (J3DModelData *)J3DModelLoaderDataBase::load(data, (tevStageNum << 16) | 0x192000 | 0x10);

    if (obj->getKind() >= 0xff && obj->getKind() < 0x10a)
        ExModel::setLightMask(mModelData[seqID], GX_LIGHT2);

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
    SysDebug::getManager()->setHeapGroup("OBJECT SHADOW", nullptr);
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
    /* 0x00 */ void *mTransAnm;
    /* 0x04 */ void *_04;
    /* 0x08 */ void *_08;

    GeoObjTransAnmInfo() // 0x80225e44
    {
        mTransAnm = 0;
        _04 = 0;
        _08 = 0;
    }
};

void GeographyObjManager::loadAnimation(GeographyObj *obj, GeographyObj **)
{
    (new GeoObjTransAnmInfo[obj->_110]);
}

void GeographyObjManager::chkAnmTableNum(GeographyObj *, u8 *, MatAnmInfo *)
{
}

void GeographyObjManager::registerAnimation(GeographyObj *)
{
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

void GeographyObjManager::load()
{
    JSUListIterator<GeographyObj> it;
    loadObjects(&mGeoObjList, it);
    for (u8 i = 0; i < 3; i++)
        loadObjects(&mItemBoxStockList[i], it);
    loadObjects(&mBoxObjList, it);
    loadObjects(&mPrevDrawObjList, it);
    loadObjects(&mJugemObjList, it);

    for (JSUListIterator<GeographyObj> genIt(mGenObjList.getFirst()); genIt.isAvailable(); ++genIt)
    {
        GeographyObj *obj = genIt.getObject();
        if (mShadowModelData[(u16)obj->_110] == nullptr)
        {
            const char *name = obj->getShadowBmdFileName();
            if (name != nullptr)
            {
                char buf[64];
                const char *path = name;
                void *data;
                if (obj != nullptr && obj->mObjFlag & 0x20)
                {
                    snprintf(buf, sizeof(buf), "/Objects/%s", name);
                    path = buf;
                }

                if (obj->mObjFlag & 0x2)
                {
                    data = ObjUtility::getPtrMainArc((char *)path);
                    if (data == nullptr)
                        data = ObjUtility::getPtrCourseArc((char *)path);
                }
                else
                {
                    data = ObjUtility::getPtrCourseArc((char *)path);
                    if (data == nullptr)
                        data = ObjUtility::getPtrMainArc((char *)path);
                    if (data == nullptr)
                        data = ObjUtility::getPtrMainLocaleArc((char *)path);
                }
                if (RaceMgr::getManager()->mRaceInfo->mRaceMode == AWARD_DEMO && data == nullptr)
                    data = (*(JKRArchive **)((char *)RaceMgr::getManager() + 0x90))->getResource(path); // mAwardArc

                if (data == nullptr)
                    OSReport("not found %s\n", name);
#line 1603 "GeographyObjMgr.cpp"
                JUT_ASSERT_F(data != nullptr, "can't find %s", name);
#line 1604 "GeographyObjMgr.cpp"
                mShadowModelData[(u16)obj->_110] = J3DModelLoaderDataBase::load(data, 0x192110);
                ExModel::patchModelData(mShadowModelData[(u16)obj->_110]);
                mShadowModelData[(u16)obj->_110]->newSharedDisplayList(0x40000);
            }
        }
        genIt.getObject()->loadShadowModel(getShadowModelData((u16)genIt.getObject()->_110), genIt.getObject()->getKind());
    }

    {
        JSUListIterator<ExGeographyObj> exIt;
        loadObjects(&mBlockObjList, exIt);
    }

    for (JSUListIterator<GeoObjSupervisor> svIt(mSupervisorList.getFirst()); svIt.isAvailable(); ++svIt)
        svIt->load();

    for (JSUListIterator<ExGeographyObj> spIt(mSpObjList.getFirst()); spIt.isAvailable(); ++spIt)
    {
        setModelData(spIt.getObject());
        spIt.getObject()->loadmodel(getModelData((u16)spIt.getObject()->_110));
    }
}

// Defined after GeographyObjManager::load so the template is instantiated
// out-of-line instead of being inlined into load.
template <typename L, typename I>
void GeographyObjManager::loadObjects(L *list, I &iterator)
{
    for (iterator = list->getFirst(); iterator.isAvailable(); ++iterator)
    {
        GeographyObj *obj = iterator.getObject();
        setModelData(obj);
        obj->loadmodel(getModelData(obj->_110));

        u32 seqIdx = (u16)obj->_110;
        if (mShadowModelData[seqIdx] == nullptr)
        {
            const char *name = obj->getShadowBmdFileName();
            if (name != nullptr)
            {
                char buf[64];
                const char *path = name;
                if (obj != nullptr && obj->mObjFlag & 0x20)
                {
                    snprintf(buf, sizeof(buf), "/Objects/%s", name);
                    path = buf;
                }
                void *data;
                if (obj->mObjFlag & 0x2)
                {
                    data = ObjUtility::getPtrMainArc((char *)path);
                    if (data == nullptr)
                        data = ObjUtility::getPtrCourseArc((char *)path);
                }
                else
                {
                    data = ObjUtility::getPtrCourseArc((char *)path);
                    if (data == nullptr)
                        data = ObjUtility::getPtrMainArc((char *)path);
                    if (data == nullptr)
                        data = ObjUtility::getPtrMainLocaleArc((char *)path);
                }
                if (RaceMgr::getManager()->mRaceInfo->mRaceMode == AWARD_DEMO && data == nullptr)
                    data = (*(JKRArchive **)((char *)RaceMgr::getManager() + 0x90))->getResource(path); // mAwardArc

                if (data == nullptr)
                    OSReport("not found %s\n", name);
#line 1603 "GeographyObjMgr.cpp"
                JUT_ASSERT_F(data != nullptr, "can't find %s", name);
#line 1604 "GeographyObjMgr.cpp"
                mShadowModelData[seqIdx] = (J3DModelData *)J3DModelLoaderDataBase::load(data, 0x192110);
                ExModel::patchModelData(mShadowModelData[seqIdx]);
                mShadowModelData[seqIdx]->newSharedDisplayList(0x40000);
            }
        }
        obj->loadShadowModel(getShadowModelData(seqIdx), obj->getKind());
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
        if (obj->mExBounds->IsHitBound(obj->mPos, pos))
            return obj;
    }
    return nullptr;
}

ExGeographyObj *GeographyObjManager::search_boundSp(const JGeometry::TVec3f &pos, GeographyObj *searcher)
{
    for (JSUListIterator<ExGeographyObj> it(mSpObjList.getFirst()); it.isAvailable(); ++it)
    {
        ExGeographyObj *obj = it.getObject();
        if (obj != searcher && obj->Search_Bound(pos))
            return obj;
    }
    return nullptr;
}

ExGeographyObj *GeographyObjManager::search_boundSp(const JGeometry::TVec3f &pos, f32 radius)
{
    for (JSUListIterator<ExGeographyObj> it(mSpObjList.getFirst()); it.isAvailable(); ++it)
    {
        ExGeographyObj *obj = it.getObject();
        if (obj->Search_BoundRadius(pos, radius))
            return obj;
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
            do
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
            } while (drawer.loadPreDrawSetting());
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
            do
            {
                for (JSUListIterator<GeographyObj> it(mPrevDrawObjList.getFirst()); it.isAvailable(); ++it)
                {
                    GeographyObj *obj = it.getObject();
                    if (i == obj->_110)
                        obj->simpleDraw(viewNo);
                }
            } while (drawer.loadPreDrawSetting());
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
