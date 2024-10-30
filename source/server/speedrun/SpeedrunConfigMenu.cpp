#include "server/speedrun/SpeedrunConfigMenu.hpp"

#include "server/speedrun/SpeedrunInfo.hpp"

SpeedrunConfigMenu::SpeedrunConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Toggle H&S Gravity (OFF)");
    mItems->mBuffer[1].copy(u"Mario Collision (ON)    ");
    mItems->mBuffer[2].copy(u"Mario Bounce (ON)       ");
    mItems->mBuffer[3].copy(u"Cappy Collision (OFF)   ");
    mItems->mBuffer[4].copy(u"Cappy Bounce (OFF)      ");
}

const sead::WFixedSafeString<0x200>* SpeedrunConfigMenu::getStringData() {
    // Gravity
    const char16_t* gravity = (
        SpeedrunInfo::mIsUseGravity
        ? u"Toggle H&S Gravity (ON) "
        : u"Toggle H&S Gravity (OFF)"
    );

    // Collision Toggles
    const char16_t* marioCollision = (
        SpeedrunInfo::mHasMarioCollision
        ? u"Mario Collision (ON)    "
        : u"Mario Collision (OFF)   "
    );
    const char16_t* marioBounce = (
        SpeedrunInfo::mHasMarioBounce
        ? u"Mario Bounce (ON)       "
        : u"Mario Bounce (OFF)      "
    );
    const char16_t* cappyCollision = (
        SpeedrunInfo::mHasCappyCollision
        ? u"Cappy Collision (ON)    "
        : u"Cappy Collision (OFF)   "
    );
    const char16_t* cappyBounce = (
        SpeedrunInfo::mHasCappyBounce
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

GameModeConfigMenu::UpdateAction SpeedrunConfigMenu::updateMenu(int selectIndex) {
    switch (selectIndex) {
        case 0: {
            SpeedrunInfo::mIsUseGravity = !SpeedrunInfo::mIsUseGravity;
            return UpdateAction::REFRESH;
        }
        case 1: {
            SpeedrunInfo::mHasMarioCollision = !SpeedrunInfo::mHasMarioCollision;
            return UpdateAction::REFRESH;
        }
        case 2: {
            SpeedrunInfo::mHasMarioBounce = !SpeedrunInfo::mHasMarioBounce;
            return UpdateAction::REFRESH;
        }
        case 3: {
            SpeedrunInfo::mHasCappyCollision = !SpeedrunInfo::mHasCappyCollision;
            return UpdateAction::REFRESH;
        }
        case 4: {
            SpeedrunInfo::mHasCappyBounce = !SpeedrunInfo::mHasCappyBounce;
            return UpdateAction::REFRESH;
        }
        default: {
            return UpdateAction::NOOP;
        }
    }
}
