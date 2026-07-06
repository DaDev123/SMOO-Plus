#include "game/Scene/StageSceneStatePauseMenu.h"

#include "hk/diag/diag.h"

#include "al/Library/Message/MessageHolder.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "al/Library/Scene/SceneUtil.h"

#include "game/Layout/FooterParts.h"
#include "game/Scene/StageScene.h"

#include "logger.hpp"

void StageSceneStatePauseMenu::exeModConfig(void) {
    if (al::isFirstStep(this)) {
        hk::diag::logLine("Start Server Config Nerve.");
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
            mFooterParts->tryChangeTextFade(
                al::getSystemMessageString(mMenuGuide, "Footer", "MenuMessage_Footer"));

            al::setNerve(this, &NrvStageSceneStatePauseMenu.Wait);
        }
    }
}