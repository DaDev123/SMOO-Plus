#include "server/inf/InfectionConfigMenu.hpp"

#include "server/inf/InfectionInfo.hpp"

InfectionConfigMenu::InfectionConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Toggle Infection Gravity (OFF)");
    mItems->mBuffer[1].copy(u"Collision des Joueurs (ON)    ");
    mItems->mBuffer[2].copy(u"Saut sur Joueurs (ON)       ");
    mItems->mBuffer[3].copy(u"Collision de Cappy (OFF)   ");
    mItems->mBuffer[4].copy(u"Boost de Cappy (OFF)      ");
    mItems->mBuffer[5].copy(u"Cappy Damage (OFF)      ");
}

const sead::WFixedSafeString<0x200>* InfectionConfigMenu::getStringData() {
    // Gravity
    const char16_t* gravity = (
        InfectionInfo::mIsUseGravity
        ? u"Toggle Infection Gravity (ON) "
        : u"Toggle Infection Gravity (OFF)"
    );

    // Collision Toggles
    const char16_t* marioCollision = (
        InfectionInfo::mHasMarioCollision
        ? u"Collision des Joueurs (ON)    "
        : u"Collision des Joueurs (OFF)   "
    );
    const char16_t* marioBounce = (
        InfectionInfo::mHasMarioBounce
        ? u"Saut sur Joueurs (ON)       "
        : u"Saut sur Joueurs (OFF)      "
    );
    const char16_t* cappyCollision = (
        InfectionInfo::mHasCappyCollision
        ? u"Collision de Cappy (ON)    "
        : u"Collision de Cappy (OFF)   "
    );
    const char16_t* cappyBounce = (
        InfectionInfo::mHasCappyBounce
        ? u"Boost de Cappy (ON)       "
        : u"Boost de Cappy (OFF)      "
    );
    const char16_t* cappyDamage = (
        InfectionInfo::mHasCappyDamage
        ? u"Cappy Damage (ON)       "
        : u"Cappy Damage (OFF) (Needs Collision de Cappy)     "
    );

    mItems->mBuffer[0].copy(gravity);
    mItems->mBuffer[1].copy(marioCollision);
    mItems->mBuffer[2].copy(marioBounce);
    mItems->mBuffer[3].copy(cappyCollision);
    mItems->mBuffer[4].copy(cappyBounce);
    mItems->mBuffer[5].copy(cappyDamage);

    return mItems->mBuffer;
}

GameModeConfigMenu::UpdateAction InfectionConfigMenu::updateMenu(int selectIndex) {
    switch (selectIndex) {
        case 0: {
            InfectionInfo::mIsUseGravity = !InfectionInfo::mIsUseGravity;
            return UpdateAction::REFRESH;
        }
        case 1: {
            InfectionInfo::mHasMarioCollision = !InfectionInfo::mHasMarioCollision;
            return UpdateAction::REFRESH;
        }
        case 2: {
            InfectionInfo::mHasMarioBounce = !InfectionInfo::mHasMarioBounce;
            return UpdateAction::REFRESH;
        }
        case 3: {
            InfectionInfo::mHasCappyCollision = !InfectionInfo::mHasCappyCollision;
            return UpdateAction::REFRESH;
        }
        case 4: {
            InfectionInfo::mHasCappyBounce = !InfectionInfo::mHasCappyBounce;
            return UpdateAction::REFRESH;
        }
        case 5: {
            InfectionInfo::mHasCappyDamage = !InfectionInfo::mHasCappyDamage;
            return UpdateAction::REFRESH;
        }
        default: {
            return UpdateAction::NOOP;
        }
    }
}
