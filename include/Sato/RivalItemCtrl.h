#ifndef RIVALITEMCTRL_H
#define RIVALITEMCTRL_H

#include <JSystem/JKernel/JKRDisposer.h>

#include "Sato/RivalCtrlBase.h"
#include "Sato/ItemObj.h"
#include "types.h"

class RivalKart;

class RivalItemCtrl : RivalCtrlBase, JKRDisposer
{ 
public:
    typedef void (RivalItemCtrl::*StateFunc)();

    RivalItemCtrl(RivalKart *);          // 0x8027ea50
    void init();                         // 0x8027eb5c
    void reset();                        // 0x8027eb7c
    void doItemCtrl();                   // 0x8027ebb8
    void startItem(ItemObj *);           // 0x8027ed9c
    void resetSucItem(ItemObj *);
    void endItem();
    void execute_endItemParam();         // 0x8027ee0c
    void setRndCount(u32, u32);
    void set_endItemParam();             // 0x8027ee44
    void setRndFlgItemFront(float);
    void setThrowFlg(bool);              // 0x8027ee74
    bool chkEnableThrowBack();
    void initItem_GTurtle();             // 0x8027efb0
    void useItem_GTurtle_Normal();       // 0x8027f00c
    void initItem_GTurtleSuc();          // 0x8027f0a0
    void useItem_GTurtleSuc_Normal();    // 0x8027f150
    void initItem_RTurtleSuc();
    void useItem_RTurtleSuc_Normal();
    void doSpReleaseGTurtle();
    void initItem_RTurtle();             // 0x8027f3a8
    void useItem_RTurtle_Normal();       // 0x8027f404
    void initItem_Bomb();                // 0x8027f498
    void releaseBombFront(const float &, const float &);
    void initItem_YoshiEgg();            // 0x8027f4f4
    void useItem_YoshiEgg_Normal();      // 0x8027f550
    void useItem_Bomb_Normal();          // 0x8027f630
    void initItem_Banana();              // 0x8027f710
    void useItem_Banana_Normal();        // 0x8027f764
    void initItem_Kinoko();              // 0x8027f840
    void doUseKinoko();
    void useItem_Kinoko_Normal();        // 0x8027f89c
    void initItem_KinokoSuc();           // 0x8027f9bc
    void useItem_KinokoSuc_Normal();     // 0x8027fa6c
    void initItem_GoldenKinoko();        // 0x8027fb24
    void useItem_GoldenKinoko();         // 0x8027fb80
    void initItem_Star();                // 0x8027fcd4
    void useItem_Star_Normal();          // 0x8027fd30
    void initItem_Thunder();             // 0x8027fde8
    void useItem_Thunder_Normal();       // 0x8027fe44
    void initItem_WanWan();              // 0x8027fe74
    void useItem_WanWan_Normal();        // 0x8027fed0
    void pressButton();
    void releaseButton();
    void releaseButtonStickFront(float);
    void releaseButtonStickBack(float);
    void triggerButton();                // 0x8027ff00
    void triggerButtonStickFront(float); // 0x8027fff4
    void triggerButtonStickBack(float);  // 0x802800f8
    // Inline/Unused
    // Inline
    virtual ~RivalItemCtrl(); // 0x8027eae4

    StateFunc mStateFunc;  // 24
    u32 mUseCnt;           // 30
    u32 mFlags;            // 34
    u32 mUseWait;          // 38
    ItemObj *mItem;        // 3c
    bool mSuc;             // 40
    u8 _41[3];             // 41
}; // Size: 0x44

#endif // RIVALITEMCTRL_H
