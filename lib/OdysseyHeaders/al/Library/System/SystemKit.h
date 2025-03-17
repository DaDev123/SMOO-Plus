#pragma once

#include <heap/seadHeap.h>

namespace al {
class FileLoader;
class ResourceSystem;
class SaveDataDirector;
class MemorySystem;

class SystemKit {
public:
    SystemKit();

    void createFileLoader(s32 threadPriority);
    void createMemorySystem(sead::Heap* heap);
    void createResourceSystem(const char* archivePath, s32, s32, bool);
    void createSaveDataSystem();

    MemorySystem* getMemorySystem() { return mMemorySystem; }

    FileLoader* getFileLoader() { return mFileLoader; }

    ResourceSystem* getResourceSystem() { return mResourceSystem; }

    SaveDataDirector* getSaveDataDirector() { return mSaveDataDirector; }

public:
    friend class alProjectInterface;

public:
    MemorySystem* mMemorySystem;
    FileLoader* mFileLoader;
    ResourceSystem* mResourceSystem;
    SaveDataDirector* mSaveDataDirector;
};

static_assert(sizeof(SystemKit) == 0x20);
}  // namespace al
