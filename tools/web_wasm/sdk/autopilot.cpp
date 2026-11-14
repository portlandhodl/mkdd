// Test aid for headless runs: MKDD_AUTOPILOT=1 hands player 1's kart to the CPU
// driver (RivalKart) once a race is running, so scripted runs can finish races
// (results, standings, the next course) without steering input. Called from PADRead.
// MKDD_GIVEITEM=<kind>[,<kind>...] stocks those items (ItemObj kinds, cycled) for player 1
// every 5 s of race time whenever it holds none, so scripted runs can exercise each item.
#include "types.h"
#include "Kaneshige/RaceMgr.h"
#include "Yamamoto/kartCtrl.h"
#include "Sato/ItemObjMgr.h"
#include <stdlib.h>

extern "C" int port_env(const char *name);
extern "C" char *gpu_js_env_str(const char *name);

// MKDD_COURSE=<decimal ECourseID> swaps every race course ResMgr loads (menus, award and ending are left
// alone), so scripted runs reach a given course through the first Grand Prix race.
extern "C" int port_course_override(int courseID)
{
    static int course = -1;
    if (course < 0)
        course = port_env("MKDD_COURSE");
    return (course && courseID >= BABY_PARK && courseID <= SHERBET_LAND) ? course : courseID;
}

static void port_give_item(void)
{
    static int kinds[32], nkinds = -1, next;
    if (nkinds < 0) {
        nkinds = 0;
        char *env = gpu_js_env_str("MKDD_GIVEITEM");
        for (char *p = env; p && *p && nkinds < 32;) {
            char *end;
            kinds[nkinds] = (int)strtol(p, &end, 10);
            if (end == p)
                break;
            nkinds++;
            p = *end == ',' ? end + 1 : end;
        }
        free(env);
    }
    RaceMgr *rcm = RaceMgr::getManager();
    ItemObjMgr *mgr = ItemObjMgr::getItemObjMgr();
    if (!nkinds || !rcm || !mgr || rcm->getRaceFrame() < 400 || rcm->getRaceFrame() % 300)
        return;
    static u32 last;
    if (rcm->getRaceFrame() == last)
        return;
    last = rcm->getRaceFrame();
    u8 driver = ItemObjMgr::getNowTandemDriverNum(0);
    if (mgr->getStockItem(0, driver) != 0x10) // 0x10: no item
        return;
    int kind = kinds[next++ % nkinds];
    mgr->abortItemShuffle(0);
    if (mgr->stockItemToKart(kind, 0, driver, false, 0))
        OSReport("[giveitem] frame %u kind %d\n", last, kind);
}

extern "C" void port_autopilot(void)
{
    port_give_item();
    static int enabled = -1;
    if (enabled < 0)
        enabled = port_env("MKDD_AUTOPILOT");
    if (!enabled)
        return;
    RaceMgr *rcm = RaceMgr::getManager();
    KartCtrl *ctrl = KartCtrl::getKartCtrl();
    if (!rcm || !ctrl || rcm->getRaceFrame() < 2 || ctrl->GetKartNumber() <= 0)
        return;
    KartBody *body = ctrl->getKartBody(0);
    if (body && (body->mGameStatus & 2) && !(body->mGameStatus & 8))
        body->mGameStatus |= 8;
    static u32 last;
    if (body && rcm->getRaceFrame() % 600 == 0 && rcm->getRaceFrame() != last) {
        last = rcm->getRaceFrame();
        KartChecker *kc = rcm->getKartChecker(0);
        OSReport("[autopilot] frame %u lap %d rank %d goal %d pos %.0f %.0f %.0f\n", rcm->getRaceFrame(), kc->getLap(),
                 kc->getRank(), kc->isGoal(), body->mPos.x, body->mPos.y, body->mPos.z);
    }
}
