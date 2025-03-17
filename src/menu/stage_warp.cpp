#include <stage_warp.h>

#include <cstdio>

#include "al/Library/Base/StringUtil.h"
#include "al/Library/Scene/Scene.h"
#include "game/Sequence/ChangeStageInfo.h"
#include "game/Sequence/HakoniwaSequence.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameSystem.h"
#include "getHelper.h"
#include "helpers/InputHelper.h"
#include "imgui.h"
#include "imgui_internal.h"

struct KingdomEnglishName {
    const char* mInternal;
    const char* mEnglish;
};

bool isShowMenu = false;
s32 curScenario = 0;

KingdomEnglishName names[] = {

    { "CapWorldHomeStage", "Cap Kingdom" },
    { "FrogSearchExStage", "Frog Pond" },
    { "CapWorldTowerStage", "Inside Cap Tower" },
    { "PoisonWaveExStage", "Poison Tides" },
    { "RollingExStage", "Precision Rolling" },
    { "PushBlockExStage", "Push Block Peril" },

    { "WaterfallWorldHomeStage", "Cascade Kingdom" },
    { "Lift2DExStage", "8-Bit Chasm Lifts" },
    { "TrexPoppunExStage", "Dinosaur Nest" },
    { "WindBlowExStage", "Gusty Bridges" },
    { "CapAppearExStage", "Mysterious Clouds" },
    { "WanwanClashExStage", "Nice Shots with Chain Chomps" },

    { "SandWorldHomeStage", "Sand Kingdom" },
    { "SandWorldKillerExStage", "Bullet Bill Maze" },
    { "RocketFlowerExStage", "Colossal Ruins" },
    { "SandWorldCostumeStage", "Costume Room" },
    { "SandWorldShopStage", "Crazy Cap Store" },
    { "SandWorldUnderground001Stage", "Deepest Underground" },
    { "WaterTubeExStage", "Freezing Waterway" },
    { "SandWorldPressExStage", "Ice Cave" },
    { "SandWorldPyramid000Stage", "Inverted Pyramid: Lower Interior" },
    { "SandWorldPyramid001Stage", "Inverted Pyramid: Upper Interior" },
    { "SandWorldSphinxExStage", "Jaxi Driving" },
    { "SandWorldVibrationStage", "Rumbling Floor House" },
    { "SandWorldSecretStage", "Sphynx Treasure Vault" },
    { "SandWorldRotateExStage", "Strange Neighborhood" },
    { "MeganeLiftExStage", "The Invisible Maze" },
    { "SandWorldSlotStage", "Tostarena Slots" },
    { "SandWorldMeganeExStage", "Transparent Lifts" },
    { "SandWorldUnderground000Stage", "Underground Ruins" },

    { "ForestWorldHomeStage", "Wooded Kingdom" },
    { "KillerRoadExStage", "Breakdown Road" },
    { "ForestWorldWoodsCostumeStage", "Costume Room" },
    { "ShootingElevatorExStage", "Crowded Elevator" },
    { "ForestWorldWoodsStage", "Deep Woods" },
    { "ForestWorldWoodsTreasureStage", "Deep Woods Treasure Vault" },
    { "ForestWorldWaterExStage", "Flooding Pipeway" },
    { "RailCollisionExStage", "Flower Road" },
    { "AnimalChaseExStage", "Herding Sheep" },
    { "PackunPoisonExStage", "Invisible Road" },
    { "ForestWorldBossStage", "Secret Flower Field" },
    { "FogMountainExStage", "Shards in the Fog" },
    { "ForestWorldTowerStage", "Sky Garden Tower" },
    { "ForestWorldBonusStage", "Spinning-Platforms Treasure Vault" },
    { "ForestWorldCloudBonusExStage", "Walking on Clouds" },

    { "LakeWorldHomeStage", "Lake Kingdom" },
    { "LakeWorldShopStage", "Crazy Cap Store" },
    { "TrampolineWallCatchExStage", "Jump, Grab, Cling, Climb" },
    { "GotogotonExStage", "Puzzle Room" },
    { "FastenerExStage", "Unzipping the Chasm" },
    { "FrogPoisonExStage", "Waves of Poison" },

    { "CloudWorldHomeStage", "Cloud Kingdom" },
    { "Cube2DExStage", "2D Cube" },
    { "FukuwaraiKuriboStage", "Picture Match" },

    { "ClashWorldHomeStage", "Lost Kingdom" },
    { "ClashWorldShopStage", "Crazy Cap Store" },
    { "ImomuPoisonExStage", "Stretch and Traverse the Jungle" },
    { "JangoExStage", "Klepto 2 - Lava Boogaloo" },

    { "CityWorldHomeStage", "Metro Kingdom" },
    { "PoleKillerExStage", "Bullet Billding" },
    { "CityPeopleRoadStage", "Crowded Alleyway" },
    { "CityWorldShop01Stage", "Crazy Cap Store" },
    { "CityWorldMainTowerStage", "Inside New Donk City Tower" },
    { "CityWorldSandSlotStage", "Metro Kingdom Slots" },
    { "Theater2DExStage", "On the Big Screen" },
    { "DonsukeExStage", "Pitchblack Mountain" },
    { "Note2D3DRoomExStage", "Rainbow Notes" },
    { "RadioControlExStage", "RC Race" },
    { "ElectricWireExStage", "Rewiring the Neighborhood" },
    { "CapRotatePackunExStage", "Rotating Maze Shards" },
    { "BikeSteelExStage", "Scooter Daredevil" },
    { "TrexBikeExStage", "Scooter Escape!" },
    { "CityWorldFactoryStage", "Sewers" },
    { "ShootingCityExStage", "Shards Under Siege" },
    { "PoleGrabCeilExStage", "Swinging Along the High-Rises" },
    { "SwingSteelExStage", "Swinging Scaffolding" },

    { "SnowWorldHomeStage", "Snow Kingdom" },
    { "SnowWorldLobby000Stage", "Class A Lobby" },
    { "SnowWorldLobbyExStage", "Class S Lobby" },
    { "SnowWorldCostumeStage", "Cold Room" },
    { "SnowWorldShopStage", "Crazy Cap Store" },
    { "IceWaterDashExStage", "Dashing Over Cold Water" },
    { "IceWaterBlockExStage", "Freezing Water" },
    { "IceWalkerExStage", "Shards in the Cold Room" },
    { "SnowWorldTownStage", "Shiveria Town" },
    { "SnowWorldCloudBonusExStage", "Spinning Above the Clouds" },
    { "ByugoPuzzleExStage", "Ty-Foo Sliding Puzzle" },
    { "KillerRailCollisionExStage", "Wintery Flower Road" },

    { "SeaWorldHomeStage", "Seaside Kingdom" },
    { "CloudExStage", "A Sea of Clouds" },
    { "SeaWorldCostumeStage", "Costume Room" },
    { "WaterValleyExStage", "Flying Through the Narrow Valley" },
    { "SeaWorldVibrationStage", "Rumbling Floor Cave" },
    { "SeaWorldSecretStage", "Sphynx Treasure Vault" },
    { "TogezoRotateExStage", "Spinning Maze" },
    { "ReflectBombExStage", "Stop, Poke, and Roll" },
    { "SenobiTowerExStage", "Stretching Up the Sinking Island" },
    { "SeaWorldUtsuboCaveStage", "Underwater Tunnel" },
    { "SeaWorldSneakingManStage", "Wriggling Power Moon" },

    { "LavaWorldHomeStage", "Luncheon Kingdom" },
    { "GabuzouClockExStage", "Blazing Above the Gears" },
    { "LavaWorldShopStage", "Crazy Cap Store(?)" },
    { "ForkExStage", "Fork-Flickin to the Summit" },
    { "LavaWorldFenceLiftExStage", "Lava Island Fly-By" },
    { "LavaBonus1Zone", "Luncheon Kingdom Slots(?)" },
    { "LavaWorldTreasureStage", "Luncheon Treasure Vault(?)" },
    { "LavaWorldUpDownExStage", "Magma Swamp" },
    { "LavaWorldBubbleLaneExStage", "Narrow Magma Path" },
    { "LavaWorldExcavationExStage", "Shards in the Cheese Rocks" },
    { "LavaWorldCostumeStage", "Simmering in the Kitchen" },
    { "LavaWorldClockExStage", "Spinning Athletics" },
    { "CapAppearLavaLiftExStage", "Volcano Cave Cruising" },

    { "BossRaidWorldHomeStage", "Ruined Kingdom" },
    { "AttackWorldHomeStage", "Ruined Kingdom" },
    { "BullRunExStage", "Mummy Army" },
    { "DotTowerExStage", "Roulette Tower" },

    { "SkyWorldHomeStage", "Bowser's Kingdom" },
    { "SkyWorldShopStage", "Crazy Cap Store" },
    { "SkyWorldTreasureStage", "Bowser's Kingdom Treasure Vault" },
    { "SkyWorldCloudBonusExStage", "Dashing Above the Clouds" },
    { "KaronWingTowerStage", "Hexagon Tower" },
    { "JizoSwitchExStage", "Jizo's Great Adventure" },
    { "SkyWorldCostumeStage", "Scene in the Folding Screen" },
    { "TsukkunRotateExStage", "Spinning Tower" },
    { "TsukkunClimbExStage", "Wooden Tower" },

    { "MoonWorldHomeStage", "Moon Kingdom" },
    { "Galaxy2DExStage", "8-Bit Galaxy" },
    { "MoonWorldShopRoom", "Crazy Cap Store" },
    { "MoonAthleticExStage", "Giant Swings" },
    { "MoonWorldBasementStage", "Moon Cave Escape" },
    { "MoonWorldSphinxRoom", "Moon Kingdom Treasure Vault" },
    { "MoonWorldCaptureParadeStage", "Underground Caverns" },
    { "MoonWorldWeddingRoomStage", "Wedding Room" },

    { "PeachWorldHomeStage", "Mushroom Kingdom" },
    { "DotHardExStage", "8-Bit Bullet Bills" },
    { "PeachWorldCostumeStage", "Castle Courtyard 64" },
    { "RevengeBossMagmaStage", "Cookatiel Boss Re-fight" },
    { "PeachWorldShopStage", "Crazy Cap Store" },
    { "RevengeBossKnuckleStage", "Knucklotec Boss Re-fight" },
    { "RevengeBossRaidStage", "Lord of Lightning Boss Re-fight" },
    { "RevengeMofumofuStage", "Mechawiggler Boss Re-fight" },
    { "RevengeGiantWanderBossStage", "Mollusque-Lanceur Boss Re-fight" },
    { "PeachWorldPictureBossMagmaStage", "Painting Room: Cookatiel" },
    { "PeachWorldPictureBossKnuckleStage", "Painting Room: Knucklotec" },
    { "PeachWorldPictureBossRaidStage", "Painting Room: Lord of Lightning" },
    { "PeachWorldPictureMofumofuStage", "Painting Room: Mechawiggler" },
    { "PeachWorldPictureGiantWanderBossStage", "Painting Room: Mollusque-Lanceur" },
    { "PeachWorldPictureBossForestStage", "Painting Room: Torkdrift" },
    { "RevengeForestBossStage", "Torkdrift Boss Re-fight" },
    { "PeachWorldCastleStage", "Peach's Castle" },
    { "FukuwaraiMarioStage", "Picture Match" },
    { "YoshiCloudExStage", "Yoshi in the Sea of Clouds" },

    { "Special1WorldHomeStage", "Dark Side" },
    { "KillerRoadNoCapExStage", "Breakdown Road Capless" },
    { "Special1WorldTowerBombTailStage", "Rabbit Ridge Tower: Hariet Battle" },
    { "Special1WorldTowerStackerStage", "Rabbit Ridge Tower: Topper Battle" },
    { "Special1WorldTowerCapThrowerStage", "Rabbit Ridge Tower: Rango Battle" },
    { "Special1WorldTowerFireBlowerStage", "Rabbit Ridge Tower: Spewart Battle" },
    { "PackunPoisonNoCapExStage", "Invisible Road Capless" },
    { "BikeSteelNoCapExStage", "Vanishing Road Capless" },
    { "SenobiTowerYoshiExStage", "Yoshi on the Sinking Island" },
    { "ShootingCityYoshiExStage", "Yoshi Under Siege" },
    { "LavaWorldUpDownYoshiExStage", "Yoshi's Magma Swamp" },

    { "Special2WorldHomeStage", "Darker Side" },
    { "Special2WorldKoopaStage", "Culmina Crater: Bowser Escape" },
    { "Special2WorldCloudStage", "Culmina Crater: Pokio Section" },
    { "Special2WorldLavaStage", "Inside Culmina Crater" },
};

const char* getEnglishName(const char* internalName) {
    for (const auto& entry : names) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    return internalName;
}

inline const char* getScenarioType(WorldListEntry& entry, int scenario) {
    if (scenario == 0) return " (NC)";
    if (scenario == 1) return " ()";
    if (scenario == entry.clearMainScenario) return " (Peace)";
    if (scenario == entry.endingScenario) return " (PG)";
    if (scenario == entry.moonRockScenario) return " (MR)";

    return "";
}

void drawStageWarpWindow() {
    HakoniwaSequence* gameSeq = (HakoniwaSequence*)GameSystemFunction::getGameSystem()->mSequence;

    if (ImGui::CollapsingHeader("Stage Warp")) {
        ImGui::Indent();
        auto curScene = helpers::tryGetStageScene(gameSeq);

        bool isInGame = curScene && curScene->mIsAlive;

        ImGuiContext* ctx = ImGui::GetCurrentContext();

        ImGui::PushItemWidth(200);
        ImGui::InputInt("Scenario", &curScenario);
        if (curScenario < 0) curScenario = 15;
        if (curScenario > 15) curScenario = 0;
        ImGui::PopItemWidth();
        if (ctx->NavId == ImGui::GetID("Scenario")) ImGui::SetTooltip("(NC) = No Change, () = First Arrival,\n(PG) = Post-Game, (MR) = Moon Rock");

        for (auto& entry : gameSeq->mGameDataHolderAccessor.mData->mWorldList->mWorldList) {
            char popupStr[0x60] = {};
            snprintf(popupStr, sizeof(popupStr), "SubAreaList_%s", entry.mainStageName);
            char warpButtonId[0x60] = {};
            sprintf(warpButtonId, "Warp##%s", entry.mainStageName);
            char subAreaButtonId[0x60] = {};
            sprintf(subAreaButtonId, "Sub-Areas##%s", entry.mainStageName);

            ImGui::AlignTextToFramePadding();
            ImGui::BulletText("%s%s", getEnglishName(entry.mainStageName), getScenarioType(entry, curScenario));
            ImGui::SameLine();
            if (ImGui::Button(warpButtonId)) {
                if (!isInGame) continue;
                if (curScenario == 0) curScenario = -1;
                ChangeStageInfo stageInfo(
                    gameSeq->mGameDataHolderAccessor, "start", entry.mainStageName, false, curScenario, ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO
                );
                GameDataFunction::tryChangeNextStage(GameDataHolderWriter(curScene), &stageInfo);
                if (curScenario == -1) curScenario = 0;
            }

            ImGui::SameLine();
            if (ImGui::Button(subAreaButtonId)) ImGui::OpenPopup(popupStr);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Warp to Sub-Area");
            if (ImGui::BeginPopup(popupStr)) {
                for (auto& dbEntry : entry.stageNames) {
                    const char* stageName = dbEntry.stageName.cstr();

                    if (al::isEqualString(stageName, entry.mainStageName) || al::isStartWithString(stageName, "Demo")) continue;

                    if (isInGame) {
                        if (ImGui::MenuItem(getEnglishName(stageName))) {
                            if (curScenario == 0) curScenario = -1;
                            ChangeStageInfo stageInfo(
                                gameSeq->mGameDataHolderAccessor, "start", stageName, false, curScenario, ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO
                            );
                            GameDataFunction::tryChangeNextStage(GameDataHolderWriter(curScene), &stageInfo);
                            if (curScenario == -1) curScenario = 0;
                        }
                    } else {
                        ImGui::Text("%s", getEnglishName(stageName));
                    }
                }

                ImGui::EndPopup();
            }
        }
        ImGui::Unindent();
    }
}
