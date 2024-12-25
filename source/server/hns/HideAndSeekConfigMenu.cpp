#include "server/hns/HideAndSeekConfigMenu.hpp"

#include "server/hns/HideAndSeekInfo.hpp"

HideAndSeekConfigMenu::HideAndSeekConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Gravité Mario Galaxy (OFF)");
    mItems->mBuffer[1].copy(u"Collision des Joueurs (ON)    ");
    mItems->mBuffer[2].copy(u"Saut sur Joueurs (ON)       ");
    mItems->mBuffer[3].copy(u"Collision de Cappy (OFF)   ");
    mItems->mBuffer[4].copy(u"Boost de Cappy (OFF)      ");
}

const sead::WFixedSafeString<0x200>* HideAndSeekConfigMenu::getStringData() {
    // Gravity
    const char16_t* gravity = (
        HideAndSeekInfo::mIsUseGravity
        ? u"Gravité Mario Galaxy (ON) "
        : u"Gravité Mario Galaxy (OFF)"
    );

    // Collision Toggles
    const char16_t* marioCollision = (
        HideAndSeekInfo::mHasMarioCollision
        ? u"Collision des Joueurs (ON)    "
        : u"Collision des Joueurs (OFF)   "
    );
    const char16_t* marioBounce = (
        HideAndSeekInfo::mHasMarioBounce
        ? u"Saut sur Joueurs (ON)       "
        : u"Saut sur Joueurs (OFF)      "
    );
    const char16_t* cappyCollision = (
        HideAndSeekInfo::mHasCappyCollision
        ? u"Collision de Cappy (ON)    "
        : u"Collision de Cappy (OFF)   "
    );
    const char16_t* cappyBounce = (
        HideAndSeekInfo::mHasCappyBounce
        ? u"Boost de Cappy (ON)       "
        : u"Boost de Cappy (OFF)      "
    );

    mItems->mBuffer[0].copy(gravity);
    mItems->mBuffer[1].copy(marioCollision);
    mItems->mBuffer[2].copy(marioBounce);
    mItems->mBuffer[3].copy(cappyCollision);
    mItems->mBuffer[4].copy(cappyBounce);

    return mItems->mBuffer;
}

GameModeConfigMenu::UpdateAction HideAndSeekConfigMenu::updateMenu(int selectIndex) {
    switch (selectIndex) {
        case 0: {
            HideAndSeekInfo::mIsUseGravity = !HideAndSeekInfo::mIsUseGravity;
            return UpdateAction::REFRESH;
        }
        case 1: {
            HideAndSeekInfo::mHasMarioCollision = !HideAndSeekInfo::mHasMarioCollision;
            return UpdateAction::REFRESH;
        }
        case 2: {
            HideAndSeekInfo::mHasMarioBounce = !HideAndSeekInfo::mHasMarioBounce;
            return UpdateAction::REFRESH;
        }
        case 3: {
            HideAndSeekInfo::mHasCappyCollision = !HideAndSeekInfo::mHasCappyCollision;
            return UpdateAction::REFRESH;
        }
        case 4: {
            HideAndSeekInfo::mHasCappyBounce = !HideAndSeekInfo::mHasCappyBounce;
            return UpdateAction::REFRESH;
        }
        default: {
            return UpdateAction::NOOP;
        }
    }
}
