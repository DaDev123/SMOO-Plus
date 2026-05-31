#pragma once

#include <basis/seadTypes.h>
#include <container/seadSafeArray.h>

class GameDataFile;

namespace CheckpointMasterList {
constexpr s32 sNumCheckpoints = 85;

const struct CheckpointData {
    const char* stageName;
    const char* objId;
    const char* zoneName;
    const char* zoneObjId;
} list[] = {{"CapWorldHomeStage", "obj381"},
            {"CapWorldHomeStage", "obj2497"},
            {"WaterfallWorldHomeStage", "obj598"},
            {"WaterfallWorldHomeStage", "obj601"},
            {"WaterfallWorldHomeStage", "obj1724"},
            {"WaterfallWorldHomeStage", "obj2832"},
            {"WaterfallWorldHomeStage", "obj3139"},
            {"SandWorldHomeStage", "obj98"},
            {"SandWorldHomeStage", "obj99"},
            {"SandWorldHomeStage", "obj525"},
            {"SandWorldHomeStage", "obj530"},
            {"SandWorldHomeStage", "obj655"},
            {"SandWorldHomeStage", "obj1511"},
            {"SandWorldHomeStage", "obj1595"},
            {"SandWorldHomeStage", "obj1597"},
            {"SandWorldHomeStage", "obj5575"},
            {"LakeWorldHomeStage", "obj220(LakeWorldTownZone[obj324])", "LakeWorldTownZone", "obj220"},
            {"LakeWorldHomeStage", "obj583(LakeWorldTownZone[obj324])", "LakeWorldTownZone", "obj583"},
            {"LakeWorldHomeStage", "obj693(LakeWorldTownZone[obj324])", "LakeWorldTownZone", "obj693"},
            {"LakeWorldHomeStage", "obj839(LakeWorldTownZone[obj324])", "LakeWorldTownZone", "obj839"},
            {"LakeWorldHomeStage", "obj1323(LakeWorldTownZone[obj324])", "LakeWorldTownZone", "obj1323"},
            {"LakeWorldHomeStage", "obj1389(LakeWorldTownZone[obj324])", "LakeWorldTownZone", "obj1389"},
            {"ForestWorldHomeStage", "obj447"},
            {"ForestWorldHomeStage", "obj1841"},
            {"ForestWorldHomeStage", "obj2821"},
            {"ForestWorldHomeStage", "obj3708"},
            {"ForestWorldHomeStage", "obj3734"},
            {"ForestWorldHomeStage", "obj3835"},
            {"ForestWorldHomeStage", "obj5216"},
            {"ForestWorldHomeStage", "obj6865"},
            {"ForestWorldHomeStage", "obj7333"},
            {"ClashWorldHomeStage", "obj543"},
            {"ClashWorldHomeStage", "obj545"},
            {"ClashWorldHomeStage", "obj2899"},
            {"CityWorldHomeStage", "obj3994"},
            {"CityWorldHomeStage", "obj3996"},
            {"CityWorldHomeStage", "obj3998"},
            {"CityWorldHomeStage", "obj4000"},
            {"CityWorldHomeStage", "obj4904"},
            {"CityWorldHomeStage", "obj5012"},
            {"CityWorldHomeStage", "obj5016"},
            {"CityWorldHomeStage", "obj8918"},
            {"CityWorldHomeStage", "obj9017"},
            {"CityWorldHomeStage", "obj9086"},
            {"CityWorldHomeStage", "obj14061"},
            {"SnowWorldHomeStage", "obj1066"},
            {"SnowWorldHomeStage", "obj1387"},
            {"SnowWorldHomeStage", "obj1440"},
            {"SeaWorldHomeStage", "obj1524"},
            {"SeaWorldHomeStage", "obj1866"},
            {"SeaWorldHomeStage", "obj2264"},
            {"SeaWorldHomeStage", "obj2266"},
            {"SeaWorldHomeStage", "obj2621"},
            {"SeaWorldHomeStage", "obj59(SeaWorldDamageBallZone[obj1070])", "SeaWorldDamageBallZone", "obj59"},
            {"SeaWorldHomeStage", "obj182(SeaWorldLavaZone[obj1399])", "SeaWorldLavaZone", "obj182"},
            {"SeaWorldHomeStage", "obj212(SeaWorldLighthouseZone[obj1402])", "SeaWorldLighthouseZone", "obj212"},
            {"SeaWorldHomeStage", "obj153(SeaWorldUnderGlassZone[obj1898])", "SeaWorldUnderGlassZone", "obj153"},
            {"LavaWorldHomeStage", "obj642"},
            {"LavaWorldHomeStage", "obj1061"},
            {"LavaWorldHomeStage", "obj2292"},
            {"LavaWorldHomeStage", "obj2549"},
            {"LavaWorldHomeStage", "obj4074"},
            {"LavaWorldHomeStage", "obj4117"},
            {"LavaWorldHomeStage", "obj4120"},
            {"LavaWorldHomeStage", "obj4619"},
            {"LavaWorldHomeStage", "obj6042"},
            {"SkyWorldHomeStage", "obj1626"},
            {"SkyWorldHomeStage", "obj1726"},
            {"SkyWorldHomeStage", "obj2028"},
            {"SkyWorldHomeStage", "obj2697"},
            {"SkyWorldHomeStage", "obj1392(SkyWorldCastleZone[obj2160])", "SkyWorldCastleZone", "obj1392"},
            {"SkyWorldHomeStage", "obj1888(SkyWorldCastleZone[obj2160])", "SkyWorldCastleZone", "obj1888"},
            {"SkyWorldHomeStage", "obj1890(SkyWorldCastleZone[obj2160])", "SkyWorldCastleZone", "obj1890"},
            {"SkyWorldHomeStage", "obj1891(SkyWorldCastleZone[obj2160])", "SkyWorldCastleZone", "obj1891"},
            {"SkyWorldHomeStage", "obj2134(SkyWorldCastleZone[obj2160])", "SkyWorldCastleZone", "obj2134"},
            {"SkyWorldHomeStage", "obj6423(SkyWorldCastleZone[obj2160])", "SkyWorldCastleZone", "obj6423"},
            {"SkyWorldHomeStage", "obj545(SkyWorldWallZone[obj2161])", "SkyWorldWallZone", "obj545"},
            {"MoonWorldHomeStage", "obj127"},
            {"MoonWorldHomeStage", "obj345"},
            {"MoonWorldHomeStage", "obj1006"},
            {"MoonWorldHomeStage", "obj1348"},
            {"PeachWorldHomeStage", "obj162"},
            {"PeachWorldHomeStage", "obj1143"},
            {"PeachWorldHomeStage", "obj1145"},
            {"PeachWorldHomeStage", "obj1455"}};

inline sead::SafeArray<bool, 85> isGot;

CheckpointData getCheckpointDataFromMasterList(const char* objId);
}  // namespace CheckpointMasterList
