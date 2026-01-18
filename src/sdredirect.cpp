#include "sdredirect.hpp"

#include "hk/hook/Trampoline.h"

#include "nn/fs.h"

#include "sead/filedevice/seadFileDeviceMgr.h"
#include "sead/filedevice/seadPath.h"
#include "sead/filedevice/seadArchiveFileDevice.h"
#include "sead/filedevice/nin/seadNinFileDeviceBaseNin.h"

#include "al/Project/File/FileLoader.h"

HkTrampoline<void, sead::FileDeviceMgr*> fileDeviceMgrHook = hk::hook::trampoline([](sead::FileDeviceMgr* fileDeviceMgr) -> void {
    fileDeviceMgrHook.orig(fileDeviceMgr);

    fileDeviceMgr->mMountedSd = nn::fs::MountSdCardForDebug("sd") == 0;

    sead::NinFileDeviceBase* sdFileDevice = new sead::NinFileDeviceBase("sd","sd");

    fileDeviceMgr->mount(sdFileDevice);
});

HkTrampoline<sead::FileDevice*, sead::FileDeviceMgr*, sead::SafeString&, sead::BufferedSafeString*> redirectFileDevicehook =
    hk::hook::trampoline([](sead::FileDeviceMgr* thisPtr, sead::SafeString& path, sead::BufferedSafeString* pathNoDrive) -> sead::FileDevice* {
        sead::FixedSafeString<32> driveName;
        sead::FileDevice* device;

        if (!sead::Path::getDriveName(&driveName, path)) {
            device = thisPtr->findDevice("sd");

            if (!(device && device->isExistFile(path))) {
                device = thisPtr->getDefaultFileDevice();

                if (!device) return nullptr;

            }
        } else device = thisPtr->findDevice(driveName);

        if (!device) return nullptr;

        if (pathNoDrive != nullptr) sead::Path::getPathExceptDrive(pathNoDrive, path);

        char* newpath = "romfs/";

        strcat(newpath, path.cstr());

        return device;
});

HkTrampoline<sead::ArchiveRes*, al::FileLoader*, sead::SafeString&, const char*, sead::FileDevice*> fileLoaderLoadArchiveHook =
    hk::hook::trampoline([](al::FileLoader* thisPtr, sead::SafeString& path, const char* ext, sead::FileDevice* device) -> sead::ArchiveRes* {
        sead::FileDevice* sdFileDevice = sead::FileDeviceMgr::instance()->findDevice("sd");

        if (sdFileDevice && sdFileDevice->isExistFile(path)) {
            device = sdFileDevice;
            char* newpath = "romfs/";

            strcat(newpath, path.cstr());
        }

        return fileLoaderLoadArchiveHook.orig(thisPtr, path, ext, device);
});

sead::FileDevice* tryFindNewFileDevice(sead::SafeString& path, sead::FileDevice* orig) {
    sead::FileDevice* sdFileDevice = sead::FileDeviceMgr::instance()->findDevice("sd");

    if (sdFileDevice && sdFileDevice->isExistFile(path)) return sdFileDevice;

    return orig;
}

HkTrampoline<bool, al::FileLoader*, sead::SafeString&, sead::FileDevice*> fileLoaderIsExistHook =
    hk::hook::trampoline([](al::FileLoader* thisPtr, sead::SafeString& path, sead::FileDevice* device) -> bool {
        sead::FileDevice* sdFileDevice = sead::FileDeviceMgr::instance()->findDevice("sd");
        char* newpath = "romfs/";
        strcat(newpath, path.cstr());

        if (sdFileDevice && sdFileDevice->isExistFile(newpath)) {device = sdFileDevice, path = newpath;};

        return fileLoaderIsExistHook.orig(thisPtr, path, device);
});

namespace SDRedirect {
    void installHooks() {
        fileDeviceMgrHook.installAtSym<"_ZN4sead13FileDeviceMgrC1Ev">();
        redirectFileDevicehook.installAtSym<"_ZNK4sead13FileDeviceMgr18findDeviceFromPathERKNS_14SafeStringBaseIcEEPNS_22BufferedSafeStringBaseIcEE">();
        fileLoaderLoadArchiveHook.installAtSym<"_ZN2al10FileLoader16loadArchiveLocalERKN4sead14SafeStringBaseIcEEPKcPNS1_10FileDeviceE">();
        fileLoaderIsExistHook.installAtSym<"_ZNK2al10FileLoader11isExistFileERKN4sead14SafeStringBaseIcEEPNS1_10FileDeviceE">();
        fileLoaderIsExistHook.installAtSym<"_ZNK2al10FileLoader14isExistArchiveERKN4sead14SafeStringBaseIcEEPNS1_10FileDeviceE">();
    }
}