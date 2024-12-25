#include "server/inf/InfectionConfigMenu.hpp"

#include "server/inf/InfectionInfo.hpp"

InfectionConfigMenu::InfectionConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Toggle Infection Gravity (OFF)");
    mItems->mBuffer[1].copy(u"Mario Collision (ON)    ");
    mItems->mBuffer[2].copy(u"Mario Bounce (ON)       ");
    mItems->mBuffer[3].copy(u"Cappy Collision (OFF)   ");
    mItems->mBuffer[4].copy(u"Cappy Bounce (OFF)      ");
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
        ? u"Mario Collision (ON)    "
        : u"Mario Collision (OFF)   "
    );
    const char16_t* marioBounce = (
        InfectionInfo::mHasMarioBounce
        ? u"Mario Bounce (ON)       "
        : u"Mario Bounce (OFF)      "
    );
    const char16_t* cappyCollision = (
        InfectionInfo::mHasCappyCollision
        ? u"Cappy Collision (ON)    "
        : u"Cappy Collision (OFF)   "
    );
    const char16_t* cappyBounce = (
        InfectionInfo::mHasCappyBounce
        ? u"Cappy Bounce (ON)       "
        : u"Cappy Bounce (OFF)      "
    );

    mItems->mBuffer[0].copy(gravity);
    mItems->mBuffer[1].copy(marioCollision);
    mItems->mBuffer[2].copy(marioBounce);
    mItems->mBuffer[3].copy(cappyCollision);
    mItems->mBuffer[4].copy(cappyBounce);

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
        default: {
            return UpdateAction::NOOP;
        }
    }
}
