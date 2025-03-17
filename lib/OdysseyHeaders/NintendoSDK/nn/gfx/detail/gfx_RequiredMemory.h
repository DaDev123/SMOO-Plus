#pragma once

#include <nn/types.h>

namespace nn::gfx::detail {

template <class TImpl>
class RequiredMemory {
public:
    static size_t GetRequiredMemorySize(...);
    static void SetMemory(void*, size_t);
    static void* GetMemory();
};

}  // namespace nn::gfx::detail
