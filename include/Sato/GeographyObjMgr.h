#ifndef GEOGRAPHYOBJMGR_H
#define GEOGRAPHYOBJMGR_H

#include "JSystem/JGeometry/Vec.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "JSystem/J3D/J3DModel.h"
#include "Kaneshige/Course/CrsData.h"
#include "Sato/GeographyObj.h"
#include "Shiraiwa/Balloon.h"
#include "Shiraiwa/JugemMain.h"

class TShineArrowManager;

class GeoFuncTable
{
public:
    struct GeoFuncTblEntry // 0xc
    {
        u16 kind;                                             // 0
        u16 _2;                                               // 2
        GeographyObj *(*createFunc)(const CrsData::SObject &); // 4
        u16 sequentialID;                                     // 8
        u16 _a;                                               // a
    };

    struct GeoExFuncTblEntry // 0xc
    {
        u16 kind;                                            // 0
        u16 _2;                                              // 2
        ExGeographyObj *(*createFunc)(const CrsData::SObject &); // 4
        u16 sequentialID;                                    // 8
        u16 _a;                                              // a
    };

    static GeoFuncTblEntry GeoFuncTbl[];
    static GeoFuncTblEntry GeoPrevDrawFuncTbl[];
    static GeoFuncTblEntry GenGeoFuncTbl[];
    static GeoExFuncTblEntry BlockGeoFuncTbl[];
    static GeoExFuncTblEntry SpecialGeoFuncTbl[];
};

class TimeAttackInvalidGeoObj
{
public:
    static u32 TimeAttackInvalidObjTbl[];
};

class GeographyObjManager : JKRDisposer
{
public:
    GeographyObjManager(const CrsData &);
    virtual ~GeographyObjManager();

    void checkMiniGameMode();
    void clrKartHittingList();

    void setKartHitItemBoxTrigger(const int kart_index);
    bool tstKartHitItemBoxTrigger(const int kart_index);

    void setNewItemBoxObj(GeographyObj *obj);

    void resetAllGeoObj();

    void checkObjList(const CrsData &);
    void checkItemBoxObjList(const CrsData &);
    void checkJugemObjList(const CrsData &);
    void checkGenObjList(const CrsData &);

    void setSequentialID();
    void countObjNum(const CrsData::SObject *);

    GeographyObj *createObj(u32, const CrsData::SObject *);
    GeographyObj *createSubObj(u32 id);
    GeographyObj *createPrevDrawSubObj(u32 id);
    GeographyObj *createGenSubObj(u32);
    GeographyObj *createAppendGenObj(u32, const CrsData::SObject *);

    GeographyObj *chkCreateNormalObj(const CrsData::SObject *, u32);
    GeographyObj *chkCreateItemBoxObj(const CrsData::SObject *, u32);

    void appendKindObjList(GeographyObj *);

    void createAllBalloonObj();

    void setObjectByCrsData(const CrsData &);
    void setGeoObjNum(const CrsData &);
    void setModelData(GeographyObj *);

    void createModel(JKRSolidHeap *, u32);
    void createModel_norm(const JSUListIterator<GeographyObj> &, GeographyObj **, JKRSolidHeap *, u32);
    void createModel_shadow(GeographyObj *, GeographyObj **, JKRSolidHeap *, u32);

    void loadAnimation(GeographyObj *, GeographyObj **);
    void chkAnmTableNum(GeographyObj *, u8 *, MatAnmInfo *);
    void registerAnimation(GeographyObj *);

    void makeSharedDL(GeographyObj *, GeographyObj **);
    void makeShadowSharedDL(GeographyObj *, GeographyObj **);

    void getKartCollidePosRadius(JGeometry::TVec3f &, f32 &, int);

    void calc();
    bool IsCheckKartToObject(const int) const;
    void Collide_norm(const JSUListIterator<GeographyObj> &, const int &, const JGeometry::TVec3f &, const f32 &);
    void Calc_norm(const JSUListIterator<GeographyObj> &);

    void load();
    template <typename L, typename I>
    void loadObjects(L *list, I &iterator);

    J3DModelData *getModelData(u32 num);
    J3DModelData *getShadowModelData(u32 num);

    void dispose();
    void reset(const CrsData &);

    void update();
    void update_foreground();
    void update_groundObj();

    void viewCalc(u32 cameraNo);
    void viewCalc_foreground(u32 cameraNo);
    void viewCalc_groundObj(u32 cameraNo);

    void setCurrentViewNo(u32 cameraNo);
    void setCurrentViewNo_foreground(u32 cameraNo);
    void setCurrentViewNo_groundObj(u32 cameraNo);

    ExGeographyObj *search_boundEx(const JGeometry::TVec3f &pos, GeographyObj *obj);
    ExGeographyObj *search_boundSp(const JGeometry::TVec3f &pos, GeographyObj *obj);
    ExGeographyObj *search_boundSp(const JGeometry::TVec3f &pos, f32);

    void drawPrim(u32 cameraNo);
    void drawGXObj(u32 cameraNo);
    void drawSimpleModel(u32 cameraNo);
    void drawSimpleModel_groundObj(u32 cameraNo);

    TJugem *getJugem(int camera_index) const;
    TBalloonManager *getBalloonMgr(int kart_index) const;

    void append(GeographyObj *);
    void prevdraw_append(GeographyObj *);
    void genappend(GeographyObj *);
    void jugemappend(GeographyObj *);
    void blockappend(ExGeographyObj *);
    void spappend(ExGeographyObj *);
    void boxappend(GeographyObj *);
    void boxremove(GeographyObj *, u32);

    void setObjsAnmOff(); // fabricated name?

    f32 getKartHitDepthNormalObj(int kart_index) const { return mKartHitDepth[kart_index]; }
    static void createMgr(const CrsData &crsData) { gMgr = new GeographyObjManager(crsData); }
    static GeographyObjManager *getGeographyObjManager() { return gMgr; }

    bool isMiniGame() const { return mIsBalloonBattle || mIsBombBattle || mIsRobberyBattle || mIsEscapeBattle; }
    bool isRobberBattle() const { return mIsRobberyBattle; }
    bool isBalloonBattle() const { return mIsBalloonBattle; }
    bool isEscapeBattle() const { return mIsEscapeBattle; }
    bool isBombBattle() const { return mIsBombBattle; }

    void *getGeoHitTable() const { return mGeoHitTable; }

    inline GeographyObj **getKartReactHitObjectList(int myNum) {
        return &mKartHittingList[myNum][0];
    }
    inline const JGeometry::TVec3f *getKartHitRefVecNormalObj(int myNum) const {
        return &mKartHitRefVec[myNum];
    }

    static GeographyObjManager *gMgr;

    J3DModelData **mModelData;                // 18
    u8 *mSimpleModelFlags;                    // 1C
    u32 _20;                                  // 20
    J3DModelData **mShadowModelData;          // 24
    GeographyObj *mKartHittingList[8][20];    // 28
    f32 mKartHitDepth[8];                     // 2a8
    JGeometry::TVec3f mKartHitRefVec[8];      // 2c8
    TBalloonManager *mBalloonMgr[8];          // 328
    bool mKartHitItemBox[8];                  // 348
    JSUList<GeographyObj> mGeoObjList;        // 350
    JSUList<GeographyObj> mPrevDrawObjList;   // 35c
    JSUList<GeographyObj> mGenObjList;        // 368
    JSUList<GeographyObj> mJugemObjList;      // 374
    JSUList<GeographyObj> mBoxObjList;        // 380
    JSUList<ExGeographyObj> mBlockObjList;    // 38c
    JSUList<ExGeographyObj> mSpObjList;       // 398
    JSUList<GeoObjSupervisor> mSupervisorList; // 3a4
    JSUList<GeographyObj> mItemBoxStockList[3]; // 3b0
    GeographyObj **mKindObjTbl;               // 3d4
    u16 GeoFuncTableMax;                      // 3d8
    u16 GeoPrevDrawFuncTableMax;              // 3da
    u16 GenGeoFuncTableMax;                   // 3dc
    u16 BlockGeoFuncTableMax;                 // 3de
    u16 SpecialGeoFuncTableMax;               // 3e0
    u16 mGeoObjNum;                           // 3e2
    u8 mEnemyMakeCheck;                       // 3e4
    bool mEnemyAvailable[8];                  // 3e5
    u8 _3ed[0xb];                             // 3ed
    GeoObjSupervisor ***mSeqObjAnmTbl;        // 3f8 (actual struct defined in cpp)
    TJugem **mJugem;                          // 3fc
    TShineArrowManager *mShineArrowMgr;       // 400
    u16 mNormObjNum;                          // 404
    u16 mPrevDrawGeoObjNum;                   // 406
    u16 mGenGeoObjNum;                        // 408
    u16 mBlockGeoObjNum;                      // 40a
    u16 mSpecialGeoObjNum;                    // 40c
    u16 mItemBoxObjNum;                       // 40e
    JGeometry::TVec3f mKartCollidePos[8];     // 410
    f32 mKartCollideRadius[8];                // 470
    bool mIsLoaded;                           // 490
    bool mIsBalloonBattle;                    // 491
    bool mIsBombBattle;                       // 492
    bool mIsRobberyBattle;                    // 493
    bool mIsEscapeBattle;                     // 494
    u8 _495[3];                               // 495
    int mJugemNum;                            // 498
    void *mGeoHitTable;                       // 49C
    ExGeographyObj *mSpecialHeadObj;          // 4A0
}; // size 0x4A4

inline GeographyObjManager *GetGeoObjMgr() { return GeographyObjManager::getGeographyObjManager(); };

inline void CreateGeoObjMgr(const CrsData *crsData) { GeographyObjManager::createMgr(*crsData); }

#endif
