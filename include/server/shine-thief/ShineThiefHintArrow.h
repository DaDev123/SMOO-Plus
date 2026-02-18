#pragma once

#include "server/gamemode/GameModeHintArrow.h"
#include "server/shine-thief/ShineThiefInfo.h"

class ShineThiefHintArrow : public GameModeHintArrow {
public:
    ShineThiefHintArrow(const char* name);
    void initAfterPlacement(void) override;

protected:
    bool shouldBeVisible() override;
    void setupMaterials() override;

private:
    ShineThiefInfo* mInfo = nullptr;
};