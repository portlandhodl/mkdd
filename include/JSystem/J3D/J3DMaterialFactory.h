#ifndef _JSYSTEM_J3D_J3DMATERIALFACTORY_H
#define _JSYSTEM_J3D_J3DMATERIALFACTORY_H

#include <dolphin/gx.h>
#include <dolphin/mtx.h>

#include "JSystem/J3D/J3DFileBlock.h"
#include "JSystem/J3D/J3DGXColor.h"
#include "JSystem/J3D/J3DInd.h"
#include "JSystem/J3D/J3DMaterial.h"
#include "JSystem/J3D/J3DPE.h"
#include "JSystem/J3D/J3DTexMtx.h"
#include "JSystem/J3D/J3DTevBlock.h"
#include "JSystem/J3D/J3DTypes.h"
#include "types.h"


struct J3DDisplayListInit {
    u32 _00; // _00
    u32 _04; // _04
};

// J3DFogInfo lives in J3DPE.h (included above); keep this typedef-free.



struct J3DMaterial_SaveBlock;

struct J3DIndInitData {
    J3DIndInitData(J3DMaterial*, J3DMaterial_SaveBlock*);
    void init();
    void write_InSaver(u8**);

    u8 mEnabled;                          // _00
    u8 mIndTexStageNum;                   // _01
    u8 _02[2];                            // _02
    J3DIndTexOrderInfo mIndTexOrders[3];  // _04
    u8 _10[4];                            // _10
    J3DIndTexMtx mIndTexMatrices[3];      // _14
    J3DIndTexCoordScaleInfo mIndTexCoordScales[3]; // _68
    u8 _74[4];                            // _74
    J3DIndTevStageInfo mIndTevStageInfos[16]; // _78
};

struct J3DLightInfo {
    u8 _00[0x30];
};

struct J3DPatchingInfo {
    u16 _00;   // _00
    u16 _02;   // _02
    u16 _04;   // _04
    u16 _06;   // _06
    u16 _08;   // _08
    u16 _0A;   // _0A
    u8 _0C[4]; // _0C
};


extern const J3DTevSwapModeInfo j3dDefaultTevSwapMode;


struct J3DTexCoord2Info {
};

/**
 * @size{0x14C}
 */
struct J3DMaterialInitData {
    J3DMaterialInitData();
    J3DMaterialInitData(J3DMaterial*, J3DMaterial_SaveBlock*);
    void init();
    void write_InSaver(u8**);
    J3DMaterialInitData& operator=(const J3DMaterialInitData&);
    bool operator==(const J3DMaterialInitData&);

    u8 _00;                           // _00
    u8 _01;                           // _01
    u8 _02;                           // _02
    u8 _03;                           // _03
    u8 _04;                           // _04
    u8 _05;                           // _05
    u8 _06;                           // _06
    u8 _07;                           // _07
    u16 mMatColorIndices[2];          // _08 - guessed length
    u16 mColorChanIndices[4];         // _0C - guessed length
    u16 mAmbColorIndices[10];         // _14 - guessed length
    u16 mTexCoordInfoIndices[16];     // _28 - guessed length
    u16 mTexMtxIndices[8];            // _48 - guessed length
    u8 _58[0x2C];                     // _58 - unknown
    u16 mTexNoIndices[8];             // _84 - guessed length
    u16 mTevKColorIndices[4];         // _94 - guessed length
    u8 mTevKColorSelIndices[16];      // _9C - unknown length
    u8 mTevKAlphaSelIndices[16];      // _AC - unknown
    u16 mTevOrderInfoIndices[16];     // _BC - guessed length
    u16 mTevColorIndices[4];          // _DC - guessed length
    u16 mTevStageIndices[16];         // _E4 - guessed length
    u16 _104[16];                     // _104 - guessed length
    u16 mTevSwapModeTableIndices[16]; // _124 - guessed length
    u16 mFogInfoIndex;                // _144
    u16 mAlphaCompInfoIndex;          // _146
    u16 mBlendInfoIndex;              // _148
    u16 mNbtScaleInfoIndex;           // _14A
};

struct J3DMaterialFactory {
    enum MaterialType { NORMAL = 0, LOCKED, PATCHED };

    J3DMaterialFactory(const J3DMaterialBlock& block);
    J3DMaterialFactory(const J3DMaterialDLBlock& block);

    size_t calcSize(J3DMaterial*, MaterialType, int, u32) const;
    size_t calcSizeNormalMaterial(J3DMaterial*, int, u32) const;
    size_t calcSizePatchedMaterial(J3DMaterial*, int, u32) const;
    size_t calcSizeLockedMaterial(J3DMaterial*, int, u32) const;
    u32 countUniqueMaterials();
    J3DMaterial* create(J3DMaterial*, MaterialType, int, u32) const;
    J3DMaterial* createNormalMaterial(J3DMaterial*, int, u32) const;
    J3DLockedMaterial* createLockedMaterial(J3DMaterial*, int, u32) const;
    J3DPatchedMaterial* createPatchedMaterial(J3DMaterial*, int, unsigned long) const;
    J3DAlphaComp newAlphaComp(int) const;
    J3DGXColor newAmbColor(int, int) const;
    J3DBlend newBlend(int) const;
    J3DColorChan newColorChan(int, int) const;
    u8 newColorChanNum(int) const;
    u8 newCullMode(int) const;
    u8 newDither(int) const;
    J3DFog newFog(int) const;
    J3DIndTexCoordScale newIndTexCoordScale(int, int) const;
    J3DIndTexMtx newIndTexMtx(int, int) const;
    J3DIndTexOrder newIndTexOrder(int, int) const;
    u8 newIndTexStageNum(int) const;
    J3DIndTevStage newIndTevStage(int, int) const;
    J3DGXColor newMatColor(int, int) const;
    J3DNBTScale newNBTScale(int) const;
    J3DGXColorS10 newTevColor(int, int) const;
    J3DGXColor newTevKColor(int, int) const;
    J3DTevOrder newTevOrder(int, int) const;
    J3DTevStage newTevStage(int, int) const;
    u8 newTevStageNum(int) const;
    J3DTevSwapModeTable newTevSwapModeTable(int, int) const;
    J3DTexCoord newTexCoord(int, int) const;
    u32 newTexGenNum(int) const;
    J3DTexMtx* newTexMtx(int, int) const;
    u16 newTexNo(int, int) const;
    u8 newZCompLoc(int) const;
    J3DZMode newZMode(int) const;
    void modifyPatchedCurrentMtx(J3DMaterial*, int) const;

    /** @fabricated */
    // inline s32 getMaterialInitDataIndex(s32 initDataIndexIndex) const { return _08[initDataIndexIndex]; }
    J3DMaterialInitData& getMaterialInitData(u16) const;
    u32 countTexGens(int) const;
    u32 countStages(int) const;
    inline u8 getMaterialMode(int matID) const { return _04[_08[matID]]._00; }


    /** @fabricated */
    inline J3DIndInitData& getIndInitData(s32 index) const { return _0C[index]; }

    // unused/inlined:
    // inline J3DMaterialInitData& getMaterialInitData(u16 initDataIndexIndex) const { return _04[_08[initDataIndexIndex]]; }
    // J3DMaterialInitData& getMaterialInitData(u16 initDataIndexIndex) const;

    u16 _00; // _00

    J3DMaterialInitData* _04;     // _04
    u16* _08;                     // _08
    J3DIndInitData* _0C;          // _0C
    GXColor* _10;                 // _10
    u8* _14;                      // _14
    J3DColorChanInfo* _18;        // _18
    GXColor* _1C;                 // _1C
    J3DLightInfo* _20;            // _20
    u8* _24;                      // _24
    J3DTexCoordInfo* _28;         // _28
    J3DTexCoord2Info* _2C;        // _2C
    J3DTexMtxInfo* _30;           // _30
    J3DTexMtxInfo* _34;           // _34
    u16* _38;                     // _38
    _GXCullMode* _3C;             // _3C
    J3DTevOrderInfo* _40;         // _40
    GXColorS10* _44;              // _44
    GXColor* _48;                 // _48
    u8* _4C;                      // _4C
    J3DTevStageInfo* _50;         // _50
    J3DTevSwapModeInfo* _54;      // _54
    J3DTevSwapModeTableInfo* _58; // _58
    J3DFogInfo* _5C;              // _5C
    J3DAlphaCompInfo* _60;        // _60
    J3DBlendInfo* _64;            // _64
    J3DZModeInfo* _68;            // _68
    u8* _6C;                      // _6C
    u8* _70;                      // _70
    J3DNBTScaleInfo* _74;         // _74
    J3DDisplayListInit* _78;      // _78
    J3DPatchingInfo* _7C;         // _7C
    J3DCurrentMtxInfo* _80;       // _80
    u8* _84;                      // _84
};

/**
 * @size{0x138}
 */
struct J3DMaterialInitData_v21 {
    u8 mMaterialMode;              // _00
    u8 mCullModeIdx;               // _01
    u8 mColorChanNumIdx;           // _02
    u8 mTexGenNumIdx;              // _03
    u8 mTevStageNumIdx;            // _04
    u8 mZCompLocIdx;               // _05
    u8 mZModeIdx;                  // _06
    u8 mDitherIdx;                 // _07
    u16 mMatColorIdx[2];           // _08
    u16 mColorChanIdx[4];          // _0C
    u16 mTexCoordIdx[8];           // _14
    u8 _24[0x10];                  // _24
    u16 mTexMtxIdx[8];             // _34
    u8 _44[0x2C];                  // _44
    u16 mTexNoIdx[8];              // _70
    u16 mTevKColorIdx[4];          // _80
    u8 mTevKColorSel[0x10];        // _88
    u8 mTevKAlphaSel[0x10];        // _98
    u16 mTevOrderIdx[0x10];        // _A8
    u16 mTevColorIdx[4];           // _C8
    u16 mTevStageIdx[0x10];        // _D0
    u16 mTevSwapModeIdx[0x10];     // _F0
    u16 mTevSwapModeTableIdx[4];   // _110
    u8 _118[0x18];                 // _118
    u16 mFogIdx;                   // _130
    u16 mAlphaCompIdx;             // _132
    u16 mBlendIdx;                 // _134
    u16 mNBTScaleIdx;              // _136
};

struct J3DMaterialFactory_v21 {
    J3DMaterialFactory_v21(const J3DMaterialBlock_v21&);

    u16 countUniqueMaterials();
    u32 countTexGens(int) const;
    u32 countStages(int) const;
    J3DMaterial* create(J3DMaterial*, int, u32) const;
    J3DGXColor newMatColor(int, int) const;
    u8 newColorChanNum(int) const;
    J3DColorChan newColorChan(int, int) const;
    u32 newTexGenNum(int) const;
    J3DTexCoord newTexCoord(int, int) const;
    J3DTexMtx* newTexMtx(int, int) const;
    u8 newCullMode(int) const;
    u16 newTexNo(int, int) const;
    J3DTevOrder newTevOrder(int, int) const;
    J3DGXColorS10 newTevColor(int, int) const;
    J3DGXColor newTevKColor(int, int) const;
    u8 newTevStageNum(int) const;
    J3DTevStage newTevStage(int, int) const;
    J3DTevSwapModeTable newTevSwapModeTable(int, int) const;
    J3DMaterialInitData_v21& getMaterialInitData_v21(u16) const;
    J3DFog newFog(int) const;
    J3DAlphaComp newAlphaComp(int) const;
    J3DBlend newBlend(int) const;
    J3DZMode newZMode(int) const;
    u8 newZCompLoc(int) const;
    u8 newDither(int) const;
    J3DNBTScale newNBTScale(int) const;

    inline u8 getMaterialMode(int matID) const { return mpMaterialInitData[mpMaterialID[matID]].mMaterialMode; }

    u16 mMaterialNum;                         // _00
    J3DMaterialInitData_v21* mpMaterialInitData; // _04
    u16* mpMaterialID;                        // _08
    GXColor* mpMatColor;                      // _0C
    u8* mpColorChanNum;                       // _10
    J3DColorChanInfo* mpColorChanInfo;        // _14
    u8* mpTexGenNum;                          // _18
    J3DTexCoordInfo* mpTexCoordInfo;          // _1C
    J3DTexCoord2Info* mpTexCoord2Info;        // _20
    J3DTexMtxInfo* mpTexMtxInfo;              // _24
    J3DTexMtxInfo* field_0x28;                // _28
    u16* mpTexNo;                             // _2C
    _GXCullMode* mpCullMode;                  // _30
    J3DTevOrderInfo* mpTevOrderInfo;          // _34
    GXColorS10* mpTevColor;                   // _38
    GXColor* mpTevKColor;                     // _3C
    u8* mpTevStageNum;                        // _40
    J3DTevStageInfo* mpTevStageInfo;          // _44
    J3DTevSwapModeInfo* mpTevSwapModeInfo;    // _48
    J3DTevSwapModeTableInfo* mpTevSwapModeTableInfo; // _4C
    J3DFogInfo* mpFogInfo;                    // _50
    J3DAlphaCompInfo* mpAlphaCompInfo;        // _54
    J3DBlendInfo* mpBlendInfo;                // _58
    J3DZModeInfo* mpZModeInfo;                // _5C
    u8* mpZCompLoc;                           // _60
    u8* mpDither;                             // _64
    J3DNBTScaleInfo* mpNBTScaleInfo;          // _68
};

#endif
