// Test aid for headless runs: MKDD_AUTOPILOT=1 hands player 1's kart to the CPU
// driver (RivalKart) once a race is running, so scripted runs can finish races
// (results, standings, the next course) without steering input. Called from PADRead.
#include "types.h"
#include "Kaneshige/RaceMgr.h"
#include "Yamamoto/kartCtrl.h"

extern "C" int port_env(const char *name);

// MKDD_COURSE=<decimal ECourseID> swaps every race course ResMgr loads (menus, award and ending are left
// alone), so scripted runs reach a given course through the first Grand Prix race.
extern "C" int port_course_override(int courseID)
{
    static int course = -1;
    if (course < 0)
        course = port_env("MKDD_COURSE");
    return (course && courseID >= BABY_PARK && courseID <= SHERBET_LAND) ? course : courseID;
}

extern "C" void port_autopilot(void)
{
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
