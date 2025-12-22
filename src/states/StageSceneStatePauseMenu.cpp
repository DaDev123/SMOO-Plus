#include "game/Scene/StageSceneStatePauseMenu.h"

#include "al/Library/Message/MessageHolder.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "al/Library/Scene/SceneUtil.h"

#include "game/Layout/FooterParts.h"
#include "game/Scene/StageScene.h"

#include "logger.hpp"

void StageSceneStatePauseMenu::exeServerConfig(void) {
    if (al::isFirstStep(this)) {
        Logger::log("Start Server Config Nerve.\n");
    }

    al::updateKitListPrev(mHost);
    rs::requestGraphicsPresetAndCubeMapPause(mHost);
    al::updateKitList(mHost, "２Ｄ（ポーズ無視）");
    al::updateKitListPost(mHost);

    if (al::updateNerveState(this)) {
        if (mStateOption->isChangeLanguage() || mStateOption->mIsLoadData) {
            kill();
        } else {
            mSelectParts->appearWait();
            mFooterParts->tryChangeTextFade(al::getSystemMessageString(mMenuGuide, "Footer", "MenuMessage_Footer"));

            al::setNerve(this, &NrvStageSceneStatePauseMenu.Wait);
        }
    }
}