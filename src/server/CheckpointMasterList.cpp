#include "server/CheckpointMasterList.h"

#include "Library/Base/StringUtil.h"

CheckpointMasterList::CheckpointData CheckpointMasterList::getCheckpointDataFromMasterList(const char* objId) {
    CheckpointData result;
    for (s32 i = 0; i < sNumCheckpoints; i++) {
        if (al::isEqualString(objId, list[i].objId)) {
            CheckpointData result;
            result.objId = objId;
            result.stageName = list[i].stageName;
            return result;
        }
    }
    return result;
}
