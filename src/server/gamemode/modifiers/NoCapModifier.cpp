#include "server/gamemode/modifiers/NoCapModifier.hpp"

NoCapModifier::NoCapModifier(GameModeBase* mode) : ModeModifierBase(mode) {}

void NoCapModifier::enable() {
    ModeModifierBase::enable();
}

void NoCapModifier::disable() {
    ModeModifierBase::disable();
}