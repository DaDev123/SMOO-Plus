#include "Scene/Twists/SmallMario/SmallMario.hpp"

#include "game/Sequence/ChangeStageInfo.h"
#include "game/System/GameDataFunction.h"

#include "server/Client.hpp"

bool SmallMario::sSmallMarioEnabled = false;

void SmallMario::toggleSmallMario() {
    sSmallMarioEnabled = !sSmallMarioEnabled;

    ChangeStageInfo info =
        ChangeStageInfo(Client::get()->getHolder(), Client::get()->getHolder()->getGameDataFile()->getPlayerStartId().cstr(),
                        GameDataFunction::getCurrentStageName(Client::get()->getHolder()), false, -1, ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO);
    Client::get()->getHolder()->changeNextStage(&info, 0);
}