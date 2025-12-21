#pragma once

#include <vector>
#include "sead/basis/seadTypes.h"

namespace BloodMoon {

/**
 * Load a file from the filesystem
 * @param filePath Path to the file (e.g., "OnlineData/config.txt")
 * @param outSize Pointer to store the size of the loaded file
 * @return Pointer to null-terminated buffer containing file contents, or nullptr if failed
 * @note Caller is responsible for freeing the returned buffer with delete[]
 */
u8* loadFile(const char* filePath, size_t* outSize);

} // namespace BloodMoon