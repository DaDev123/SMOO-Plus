#pragma once

#include "Library/Scene/SceneMsg.h"

namespace al {
class SceneMsgCtrl {
public:
    SceneMsgCtrl();

public:
    SceneMsg mMessages[32];
};
}  // namespace al
