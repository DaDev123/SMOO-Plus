#include "BloodMoon/BloodMoonUtils.hpp"

#include "sead/filedevice/seadFileDevice.h"
#include "sead/filedevice/seadFileDeviceMgr.h"

#include "al/Library/File/FileUtil.h"
#include "al/Project/Memory/Util.h"

#include <cstring>

namespace BloodMoon {

u8* loadFile(const char* filePath, size_t* outSize) {
    // Check if file exists first
    if (!al::isExistFile(filePath)) {
        if (outSize)
            *outSize = 0;
        return nullptr;
    }

    // Get default file device
    sead::FileDevice* device = sead::FileDeviceMgr::instance()->getDefaultFileDevice();
    if (!device) {
        if (outSize)
            *outSize = 0;
        return nullptr;
    }

    // Try to load the file
    sead::FileDevice::LoadArg loadArg;
    loadArg.path = filePath;
    loadArg.buffer = nullptr;
    loadArg.buffer_size = 0;
    loadArg.heap = al::getCurrentHeap();
    loadArg.alignment = 0x20;
    loadArg.div_size = 0;
    loadArg.assert_on_alloc_fail = false;

    u8* fileData = device->tryLoad(loadArg);

    if (!fileData || loadArg.read_size == 0) {
        if (outSize)
            *outSize = 0;
        return nullptr;
    }

    // Copy to null-terminated buffer
    u8* buffer = new u8[loadArg.read_size + 1];
    memcpy(buffer, fileData, loadArg.read_size);
    buffer[loadArg.read_size] = '\0';

    // Free the loaded data if needed
    if (loadArg.need_unload && fileData) {
        delete[] fileData;
    }

    if (outSize) {
        *outSize = loadArg.read_size;
    }

    return buffer;
}

}  // namespace BloodMoon