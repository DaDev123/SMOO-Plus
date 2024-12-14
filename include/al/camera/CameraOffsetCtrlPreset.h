#pragma once

#include "al/byaml/ByamlIter.h"
#include "CameraOffsetPreset.h"

namespace al {

class CameraOffsetCtrlPreset {
public:
    float getOffset() const;
    void load(al::ByamlIter const&);

    const char* mUnkVTable;
    CameraOffsetPreset* mOffsetPreset;

};
}