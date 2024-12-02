#include "server/sardines/SardineConfigMenu.hpp"

SardineConfigMenu::SardineConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Sardine Gravity (OFF)");
    mItems->mBuffer[1].copy(u"Sardine Tether (OFF) ");
    mItems->mBuffer[2].copy(u"Tether Snapping (OFF)");
    mItems->mBuffer[3].copy(u"Mario Collision (ON) ");
    mItems->mBuffer[4].copy(u"Mario Bounce (ON)    ");
    mItems->mBuffer[5].copy(u"Cappy Collision (OFF)");
    mItems->mBuffer[6].copy(u"Cappy Bounce (OFF)   ");
}

const sead::WFixedSafeString<0x200>* SardineConfigMenu::getStringData() {
    // Gravity
    const char16_t* gravity = (
        SardineInfo::mIsUseGravity
        ? u"Sardine Gravity (ON) "
        : u"Sardine Gravity (OFF)"
    );

    // Tether
    const char16_t* tether = (
        SardineInfo::mIsTether
        ? u"Sardine Tether (ON)  "
        : u"Sardine Tether (OFF) "
    );
    const char16_t* tetherSnapping = (
        SardineInfo::mIsTether && SardineInfo::mIsTetherSnap
        ? u"Tether Snapping (ON) "
        : u"Tether Snapping (OFF)"
    );

    // Collision Toggles
    const char16_t* marioCollision = (
        SardineInfo::mHasMarioCollision
        ? u"Mario Collision (ON) "
        : u"Mario Collision (OFF)"
    );
    const char16_t* marioBounce = (
        SardineInfo::mHasMarioBounce
        ? u"Mario Bounce (ON)    "
        : u"Mario Bounce (OFF)   "
    );
    const char16_t* cappyCollision = (
        SardineInfo::mHasCappyCollision
        ? u"Cappy Collision (ON) "
        : u"Cappy Collision (OFF)"
    );
    const char16_t* cappyBounce = (
        SardineInfo::mHasCappyBounce
        ? u"Cappy Bounce (ON)    "
        : u"Cappy Bounce (OFF)   "
    );

    mItems->mBuffer[0].copy(gravity);
    mItems->mBuffer[1].copy(tether);
    mItems->mBuffer[2].copy(SardineInfo::mIsTether ? tetherSnapping : marioCollision);
    mItems->mBuffer[3].copy(SardineInfo::mIsTether ? marioCollision : marioBounce);
    mItems->mBuffer[4].copy(SardineInfo::mIsTether ? marioBounce    : cappyCollision);
    mItems->mBuffer[5].copy(SardineInfo::mIsTether ? cappyCollision : cappyBounce);
    mItems->mBuffer[6].copy(cappyBounce);

    return mItems->mBuffer;
}

GameModeConfigMenu::UpdateAction SardineConfigMenu::updateMenu(int selectIndex) {
    int adjustedIndex = selectIndex + (!SardineInfo::mIsTether && selectIndex >= 2);
    switch (adjustedIndex) {
        case 0: {
            SardineInfo::mIsUseGravity = !SardineInfo::mIsUseGravity;
            return UpdateAction::REFRESH;
        }
        case 1: {
            SardineInfo::mIsTether = !SardineInfo::mIsTether;
            return UpdateAction::REFRESH;
        }
        case 2: {
            SardineInfo::mIsTetherSnap = SardineInfo::mIsTether && !SardineInfo::mIsTetherSnap;
            return UpdateAction::REFRESH;
        }
        case 3: {
            SardineInfo::mHasMarioCollision = !SardineInfo::mHasMarioCollision;
            return UpdateAction::REFRESH;
        }
        case 4: {
            SardineInfo::mHasMarioBounce = !SardineInfo::mHasMarioBounce;
            return UpdateAction::REFRESH;
        }
        case 5: {
            SardineInfo::mHasCappyCollision = !SardineInfo::mHasCappyCollision;
            return UpdateAction::REFRESH;
        }
        case 6: {
            SardineInfo::mHasCappyBounce = !SardineInfo::mHasCappyBounce;
            return UpdateAction::REFRESH;
        }
        default: {
            return UpdateAction::NOOP;
        }
    }
}
