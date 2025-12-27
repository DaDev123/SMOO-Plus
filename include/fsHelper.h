#pragma once

#include "nn/types.h"

namespace FsHelper {

struct LoadData {
    const char* path = nullptr;
    void* buffer = nullptr;
    long bufSize = 0;
};

nn::Result writeFileToPath(void* buf, size_t size, const char* path);

void loadFileFromPath(LoadData& loadData);

long getFileSize(const char* path);

bool isFileExist(const char* path);
}  // namespace FsHelper
