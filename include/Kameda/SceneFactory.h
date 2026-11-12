#ifndef SCENEFACTORY_H
#define SCENEFACTORY_H

#include "Kameda/Scene.h"
#include "Kameda/BattleName2D.h"

// TODO
class CourseName2D;
class Cup2D;

class SceneFactory
{
public:
    enum SceneArc
    {
        ARC_TITLE,
        ARC_MENU,
        ARC_OPTION,
        ARC_COURSE_SELECT,
        ARC_MAP_SELECT,
        ARC_RECORD,
        ARC_GHOST_DATA,
        ARC_LAN_ENTRY,
        ARC_TITLE_LINE,
        ARC_BACK,
        ARC_COURSE_NAME,
        ARC_BATTLE_NAME,
        ARC_CUP,
        ARC_SELECT_ANM,
        ARC_SECRET,
        ARC_MAX
    };

    SceneFactory();                                  // 0x801509c4
    ~SceneFactory();                                 // 0x801509f4
    void loadData(Scene::SceneType, JKRHeap *);      // 0x80150a7c
    Scene *createScene(Scene::SceneType, JKRHeap *); // 0x801510f0
    
    // Inline/Unused
    void mountSceneArc(SceneArc, JKRHeap *);

    // Inline
    static SceneFactory *create()
    {
        if (mspSceneFactory == nullptr)
        {
            mspSceneFactory = new SceneFactory();
        }
        return mspSceneFactory;
    }

    JKRArchive *getArchive(int idx) { return mArchives[idx]; }

    static SceneFactory *ptr() { return mspSceneFactory; }
    
private:
    static SceneFactory *mspSceneFactory; // 0x804162f8

    JKRArchive *mArchives[15];
    CourseName2D *mCourseName;
    BattleName2D *mBattleName;
    Cup2D *mCup;
}; // size: 0x48;

#endif // SCENEFACTORY_H
