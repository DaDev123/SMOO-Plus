#include "fsHelper.h"
// #include "logger/Logger.hpp"
#include "hk/diag/diag.h"
#include "nn/init.h"

namespace FsHelper {
    nn::Result writeFileToPath(void *buf, size_t size, const char *path) {
        nn::fs::FileHandle handle;

        if (isFileExist(path)) {
            // Logger::log("Removing Previous File.\n");
            nn::fs::DeleteFile(path); // remove previous file
        }

        nn::Result result = nn::fs::CreateFile(path, size);

        if (result.IsFailure()) {
            // Logger::log("Failed to Create File.\n");
            return result;
        }
        result = nn::fs::OpenFile(&handle, path, nn::fs::OpenMode_Write);
        if (result.IsFailure()) {
            // Logger::log("Failed to Open File.\n");
            return result;
        }
        result = nn::fs::WriteFile(handle, 0, buf, size, nn::fs::WriteOption::CreateOption(nn::fs::WriteOptionFlag_Flush));
        if (result.IsFailure()) {
            // Logger::log("Failed to Write to File.\n");
            return result;
        }

        // Logger::log("Successfully wrote file to: %s!\n", path);

        nn::fs::CloseFile(handle);

        return nn::result::detail::ConstructResult(1);
    }

    // make sure to free buffer after usage is done
    void loadFileFromPath(LoadData &loadData) {

        nn::fs::FileHandle handle;

        HK_ASSERT(FsHelper::isFileExist(loadData.path));//, "Failed to Find File!\nPath: %s", loadData.path

        HK_ABORT_UNLESS_R(nn::fs::OpenFile(&handle, loadData.path, nn::fs::OpenMode_Read).IsFailure());

        long size = 0;
        nn::fs::GetFileSize(&size, handle);
        loadData.buffer = nn::init::GetAllocator()->Allocate(size);
        loadData.bufSize = size;

        HK_ASSERT(loadData.buffer);//, "Failed to Allocate Buffer! File Size: %ld", size

        HK_ABORT_UNLESS_R(nn::fs::ReadFile(handle, 0, loadData.buffer, size).IsFailure());

        nn::fs::CloseFile(handle);
    }


    long getFileSize(const char *path) {
        nn::fs::FileHandle handle;
        long result = -1;

        nn::Result openResult = nn::fs::OpenFile(&handle, path, nn::fs::OpenMode::OpenMode_Read);

        if (openResult.IsSuccess()) {
            nn::fs::GetFileSize(&result, handle);
            nn::fs::CloseFile(handle);
        }

        return result;
    }

    bool isFileExist(const char *path) {
        nn::fs::DirectoryEntryType type;
        nn::Result result = nn::fs::GetEntryType(&type, path);

        return type == nn::fs::DirectoryEntryType_File;
    }

    bool isDirExist(const char *path) {
        nn::fs::DirectoryEntryType type;
        nn::Result result = nn::fs::GetEntryType(&type, path);

        return type == nn::fs::DirectoryEntryType_Directory;
    }
}