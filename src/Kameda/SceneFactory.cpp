#include <stdio.h>

#include "JSystem/JUtility/JUTAssert.h"
#include "Kameda/SceneFactory.h"
#include "Kameda/CourseName2D.h"
#include "Kameda/Cup2D.h"
#include "Kameda/SceneTitle.h"
#include "Kameda/SceneMenu.h"
#include "Kameda/SceneOption.h"
#include "Kameda/SceneCourseSelect.h"
#include "Kameda/SceneMapSelect.h"
#include "Kameda/SceneRecord.h"
#include "Kameda/SceneMap.h"
#include "Kameda/SceneGhostLoadSave.h"
#include "Kameda/SceneLanEntry.h"
#include "Shiraiwa/SecretScene.h"
#include "Kaneshige/SysDebug.h"
#include "Osako/SystemRecord.h"
#include "kartLocale.h"

static const char *scpaArcName[15] = {
    "title", "menu", "option", "courseselect",
    "mapselect", "record", "ghostdata", "lanentry",
    "titleline", "back", "coursename", "battlename",
    "cup", "selectAnm", "secret"
};

SceneFactory *SceneFactory::mspSceneFactory;

SceneFactory::SceneFactory()
{
    for (int i = 0; i < 15; i++)
        mArchives[i] = nullptr;
    mCourseName = nullptr;
    mBattleName = nullptr;
    mCup = nullptr;
}

SceneFactory::~SceneFactory()
{
    mspSceneFactory = nullptr;
    delete mCourseName;
    mCourseName = nullptr;
    delete mBattleName;
    mBattleName = nullptr;
    delete mCup;
    mCup = nullptr;
}

void SceneFactory::mountSceneArc(SceneArc arc, JKRHeap *heap)
{
    const char *format = "/SceneData/%s/%s.arc";
    if (mArchives[arc] == nullptr)
    {
        char path[64];
        if (arc == ARC_TITLE && gSystemRecord.isGameUnlocked(SystemRecord::SpecialEnding))
            snprintf(path, sizeof(path), format, KartLocale::getLanguageName(), "title2");
        else
            snprintf(path, sizeof(path), format, KartLocale::getLanguageName(), scpaArcName[arc]);
        mArchives[arc] = JKRArchive::mount(path, JKRArchive::MOUNT_MEM, heap, JKRArchive::MOUNT_DIRECTION_HEAD);
    }
}

void SceneFactory::loadData(Scene::SceneType id, JKRHeap *heap)
{
#line 120
    JUT_MINMAX_ASSERT(0, id, 10);
    if (id == Scene::SCENE_GP_NEXT)
        return;

    SysDebug::getManager()->setHeapGroup("Arc", heap);

    switch (id)
    {
    case Scene::SCENE_MENU:
    case Scene::SCENE_OPTION:
    case Scene::SCENE_COURSE:
    case Scene::SCENE_MAP_SELECT:
    case Scene::SCENE_RECORD:
    case Scene::SCENE_GHOST_LOAD_SAVE:
        mountSceneArc(ARC_TITLE_LINE, heap);
        break;
    }

    switch (id)
    {
    case Scene::SCENE_MENU:
    case Scene::SCENE_COURSE:
    case Scene::SCENE_MAP_SELECT:
    case Scene::SCENE_GHOST_LOAD_SAVE:
        if (mArchives[ARC_BACK] == nullptr)
            mArchives[ARC_BACK] = JKRArchive::mount("/SceneData/back.arc", JKRArchive::MOUNT_MEM, heap, JKRArchive::MOUNT_DIRECTION_HEAD);
        break;
    }

    switch (id)
    {
    case Scene::SCENE_RECORD:
    case Scene::SCENE_GHOST_LOAD_SAVE:
        mountSceneArc(ARC_COURSE_NAME, heap);
        break;
    }

    switch (id)
    {
    case Scene::SCENE_MENU:
    case Scene::SCENE_MAP_SELECT:
        mountSceneArc(ARC_BATTLE_NAME, heap);
        break;
    }

    switch (id)
    {
    case Scene::SCENE_COURSE:
    case Scene::SCENE_RECORD:
        if (mArchives[ARC_CUP] == nullptr)
            mArchives[ARC_CUP] = JKRArchive::mount("/SceneData/cup2d.arc", JKRArchive::MOUNT_MEM, heap, JKRArchive::MOUNT_DIRECTION_HEAD);
        break;
    }

    switch (id)
    {
    case Scene::SCENE_MENU:
    case Scene::SCENE_SECRET:
        if (mArchives[ARC_SELECT_ANM] == nullptr)
            mArchives[ARC_SELECT_ANM] = JKRArchive::mount("/SceneData/selectAnm.arc", JKRArchive::MOUNT_MEM, heap, JKRArchive::MOUNT_DIRECTION_HEAD);
        break;
    }

    switch (id)
    {
    case Scene::SCENE_TITLE:
        mountSceneArc(ARC_TITLE, heap);
        break;
    case Scene::SCENE_MENU:
        mountSceneArc(ARC_MENU, heap);
        break;
    case Scene::SCENE_OPTION:
        mountSceneArc(ARC_OPTION, heap);
        break;
    case Scene::SCENE_COURSE:
        mountSceneArc(ARC_COURSE_SELECT, heap);
        break;
    case Scene::SCENE_MAP_SELECT:
        mountSceneArc(ARC_MAP_SELECT, heap);
        break;
    case Scene::SCENE_RECORD:
        mountSceneArc(ARC_RECORD, heap);
        break;
    case Scene::SCENE_GHOST_LOAD_SAVE:
        mountSceneArc(ARC_GHOST_DATA, heap);
        break;
    case Scene::SCENE_LAN_ENTRY:
        mountSceneArc(ARC_LAN_ENTRY, heap);
        break;
    case Scene::SCENE_SECRET:
        mountSceneArc(ARC_SECRET, heap);
        break;
    }

    SysDebug::getManager()->setDefaultHeapGroup(heap);
}

Scene *SceneFactory::createScene(Scene::SceneType id, JKRHeap *heap)
{
#line 214
    JUT_MINMAX_ASSERT(0, id, 10);
    Scene *scene = nullptr;

    switch (id)
    {
    case Scene::SCENE_TITLE:
        SysDebug::getManager()->setHeapGroup("Title", heap);
        scene = new (heap, 0) SceneTitle(mArchives[ARC_TITLE], heap);
        break;
    case Scene::SCENE_MENU:
        SysDebug::getManager()->setHeapGroup("Menu", heap);
        mBattleName = BattleName2D::create(mArchives[ARC_BATTLE_NAME], heap);
        scene = new (heap, 0) SceneMenu(mArchives[ARC_MENU], heap);
        break;
    case Scene::SCENE_OPTION:
        SysDebug::getManager()->setHeapGroup("Option", heap);
        scene = new (heap, 0) SceneOption(mArchives[ARC_OPTION], heap);
        break;
    case Scene::SCENE_COURSE:
        SysDebug::getManager()->setHeapGroup("Course", heap);
        mCup = Cup2D::create(mArchives[ARC_CUP], heap);
        scene = new (heap, 0) SceneCourseSelect(mArchives[ARC_COURSE_SELECT], heap);
        break;
    case Scene::SCENE_MAP_SELECT:
        SysDebug::getManager()->setHeapGroup("MapSelect", heap);
        mBattleName = BattleName2D::create(mArchives[ARC_BATTLE_NAME], heap);
        scene = new (heap, 0) SceneMapSelect(mArchives[ARC_MAP_SELECT], heap);
        break;
    case Scene::SCENE_RECORD:
        SysDebug::getManager()->setHeapGroup("Record", heap);
        mCourseName = CourseName2D::create(mArchives[ARC_COURSE_NAME], heap);
        mCup = Cup2D::create(mArchives[ARC_CUP], heap);
        scene = new (heap, 0) SceneRecord(mArchives[ARC_RECORD], heap);
        break;
    case Scene::SCENE_GP_NEXT:
        SysDebug::getManager()->setHeapGroup("GPNext", heap);
        scene = new (heap, 0) SceneMap(nullptr, heap);
        break;
    case Scene::SCENE_GHOST_LOAD_SAVE:
        SysDebug::getManager()->setHeapGroup("Ghost", heap);
        mCourseName = CourseName2D::create(mArchives[ARC_COURSE_NAME], heap);
        scene = new (heap, 0) SceneGhostLoadSave(mArchives[ARC_GHOST_DATA], heap);
        break;
    case Scene::SCENE_LAN_ENTRY:
        SysDebug::getManager()->setHeapGroup("LanEntry", heap);
        scene = new (heap, 0) SceneLanEntry(mArchives[ARC_LAN_ENTRY], heap);
        break;
    case Scene::SCENE_SECRET:
        SysDebug::getManager()->setHeapGroup("Secret", heap);
        scene = new (heap, 0) SecretScene(mArchives[ARC_SECRET], heap);
        break;
    default:
#line 265
        JUT_ASSERT_F(0, "%s", "0");
        break;
    }

    SysDebug::getManager()->setDefaultHeapGroup(heap);
    return scene;
}
