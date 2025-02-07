#pragma once

#include "al/scene/ISceneObj.h"
#include "al/scene/SceneObjHolder.h"

namespace rs {
    al::ISceneObj *createCoinCollectHolder(al::IUseSceneObjHolder const*);
    al::ISceneObj *createCoinCollectWatcher(al::IUseSceneObjHolder const*);
    al::ISceneObj *createRandomItemSelector(al::IUseSceneObjHolder const*);
    al::ISceneObj *createRouteGuideDirector(al::IUseSceneObjHolder const*);
    al::ISceneObj *createStageTimeDirector(al::IUseSceneObjHolder const*);
    al::ISceneObj *createRankingNameHolder(al::IUseSceneObjHolder const*);
}