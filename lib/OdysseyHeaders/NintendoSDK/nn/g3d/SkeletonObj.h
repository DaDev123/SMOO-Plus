#pragma once

#include "nn/g3d/ResSkeleton.h"

namespace nn::g3d {

// TODO
class SkeletonObj {
public:
    const ResSkeleton* GetRes() const { return m_Res; }

public:
    const ResSkeleton* m_Res;
    // TODO: the rest of the members
};

}  // namespace nn::g3d
