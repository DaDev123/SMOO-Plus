#include "CheckpointMasterList.h"

#include "Library/Base/StringUtil.h"

CheckpointMasterList::CheckpointData CheckpointMasterList::getCheckpointDataFromMasterList(const char* objId) {
    CheckpointData result;
    for (s32 i = 0; i < sNumCheckpoints; i++) {
        if (al::isEqualString(objId, list[i].objId)) {
            result.objId = objId;
            result.stageName = list[i].stageName;
            result.zoneName = list[i].zoneName;
            result.zoneObjId = list[i].zoneObjId;
            return result;
        }
    }
    return result = {nullptr, nullptr, nullptr, nullptr};
}
