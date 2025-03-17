#include <nn/oe.h>
#include "imgui_nvn.h"
#include "Menu.h"
#include "hk/hook/Trampoline.h"

#include "imgui_backend/imgui_impl_nvn.hpp"
#include "nn/init.h"
#include "helpers/InputHelper.h"
#include "nvn/nvn_CppFuncPtrImpl.h"

nvn::Device *nvnDevice;
nvn::Queue *nvnQueue;
nvn::CommandBuffer *nvnCmdBuf;

nvn::DeviceGetProcAddressFunc tempGetProcAddressFuncPtr;

nvn::CommandBufferInitializeFunc tempBufferInitFuncPtr;
nvn::DeviceInitializeFunc tempDeviceInitFuncPtr;
nvn::QueueInitializeFunc tempQueueInitFuncPtr;
nvn::QueuePresentTextureFunc tempPresentTexFunc;

nvn::WindowSetCropFunc tempSetCropFunc;

bool hasInitImGui = false;

namespace nvnImGui {
    ImVector<ProcDrawFunc> drawQueue;
}

#define IMGUI_USEEXAMPLE_DRAW false

void setCrop(nvn::Window *window, int x, int y, int w, int h) {
    tempSetCropFunc(window, x, y, w, h);

    if (hasInitImGui) {

        ImVec2 &dispSize = ImGui::GetIO().DisplaySize;
        ImVec2 windowSize = ImVec2(w - x, h - y);

        if (dispSize.x != windowSize.x && dispSize.y != windowSize.y) {
            bool isDockedMode = nn::oe::GetOperationMode() == nn::oe::OperationMode_Docked;

            // Logger::log("Updating Projection and Scale to: X %f Y %f\n", windowSize.x, windowSize.y);
            // Logger::log("Previous Size: X %f Y %f\n", dispSize.x, dispSize.y);

            dispSize = windowSize;
            ImguiNvnBackend::updateProjection(windowSize);
            ImguiNvnBackend::updateScale(isDockedMode);

        }
    }
}

void presentTexture(nvn::Queue *queue, nvn::Window *window, int texIndex) {

    if (hasInitImGui)
        nvnImGui::procDraw();

    tempPresentTexFunc(queue, window, texIndex);
}

NVNboolean deviceInit(nvn::Device *device, const nvn::DeviceBuilder *builder) {
    NVNboolean result = tempDeviceInitFuncPtr(device, builder);
    nvnDevice = device;
    nvn::nvnLoadCPPProcs(nvnDevice, tempGetProcAddressFuncPtr);
    return result;
}

NVNboolean queueInit(nvn::Queue *queue, const nvn::QueueBuilder *builder) {
    NVNboolean result = tempQueueInitFuncPtr(queue, builder);
    nvnQueue = queue;
    return result;
}

NVNboolean cmdBufInit(nvn::CommandBuffer *buffer, nvn::Device *device) {
    NVNboolean result = tempBufferInitFuncPtr(buffer, device);
    nvnCmdBuf = buffer;

    if (!hasInitImGui) {
        hasInitImGui = nvnImGui::InitImGui();
    }

    return result;
}

nvn::GenericFuncPtrFunc getProc(nvn::Device *device, const char *procName) {

    nvn::GenericFuncPtrFunc ptr = tempGetProcAddressFuncPtr(nvnDevice, procName);

    if (strcmp(procName, "nvnQueueInitialize") == 0) {
        tempQueueInitFuncPtr = (nvn::QueueInitializeFunc) ptr;
        return (nvn::GenericFuncPtrFunc) &queueInit;
    } 
    if (strcmp(procName, "nvnCommandBufferInitialize") == 0) {
        tempBufferInitFuncPtr = (nvn::CommandBufferInitializeFunc) ptr;
        return (nvn::GenericFuncPtrFunc) &cmdBufInit;
    } 
    if (strcmp(procName, "nvnWindowSetCrop") == 0) {
        tempSetCropFunc = (nvn::WindowSetCropFunc) ptr;
        return (nvn::GenericFuncPtrFunc) &setCrop;
    }
    if (strcmp(procName, "nvnQueuePresentTexture") == 0) {
        tempPresentTexFunc = (nvn::QueuePresentTextureFunc) ptr;
        return (nvn::GenericFuncPtrFunc) &presentTexture;
    }
    if (strcmp(procName, "nvnDeviceGetProcAddress") == 0) {
        tempGetProcAddressFuncPtr = (nvn::DeviceGetProcAddressFunc) ptr;
        return (nvn::GenericFuncPtrFunc) &getProc;
    }
    if (strcmp(procName, "nvnDeviceInitialize") == 0) {
        tempDeviceInitFuncPtr = (nvn::DeviceInitializeFunc) ptr;
        return (nvn::GenericFuncPtrFunc) &deviceInit;
    
    }

    return ptr;
}

void disableButtons(nn::hid::NpadBaseState *state) {
    if (!InputHelper::isReadInputs() && InputHelper::isInputToggled() && btt::Menu::instance()->mIsEnabledMenu) {
        // clear out the data within the state (except for the sampling number and attributes)
        state->mButtons = nn::hid::NpadButtonSet();
        state->mAnalogStickL = nn::hid::AnalogStickState();
        state->mAnalogStickR = nn::hid::AnalogStickState();
    }
}

HkTrampoline<int, int*, nn::hid::NpadFullKeyState*, int, unsigned int const&> DisableFullKeyState = hk::hook::trampoline([](int *unkInt, nn::hid::NpadFullKeyState *state, int count, unsigned int const &port) -> int {
    int result = DisableFullKeyState.orig(unkInt, state, count, port);
    disableButtons(state);
    return result;
});

HkTrampoline<int, int*, nn::hid::NpadHandheldState*, int, unsigned int const&> DisableHandheldState = hk::hook::trampoline([](int *unkInt, nn::hid::NpadHandheldState *state, int count, unsigned int const &port) -> int {
    int result = DisableHandheldState.orig(unkInt, state, count, port);
    disableButtons(state);
    return result;
});

HkTrampoline<int, int*, nn::hid::NpadJoyDualState*, int, unsigned int const&> DisableJoyDualState = hk::hook::trampoline([](int *unkInt, nn::hid::NpadJoyDualState *state, int count, unsigned int const &port) -> int {
    int result = DisableJoyDualState.orig(unkInt, state, count, port);
    disableButtons(state);
    return result;
});

HkTrampoline<int, int*, nn::hid::NpadJoyLeftState*, int, unsigned int const&> DisableJoyLeftState = hk::hook::trampoline([](int *unkInt, nn::hid::NpadJoyLeftState *state, int count, unsigned int const &port) -> int {
    int result = DisableJoyLeftState.orig(unkInt, state, count, port);
    disableButtons(state);
    return result;
});

HkTrampoline<int, int*, nn::hid::NpadJoyRightState*, int, unsigned int const&> DisableJoyRightState = hk::hook::trampoline([](int *unkInt, nn::hid::NpadJoyRightState *state, int count, unsigned int const &port) -> int {
    int result = DisableJoyRightState.orig(unkInt, state, count, port);
    disableButtons(state);
    return result;
});

HkTrampoline<void*, const char*> NvnBootstrapHook = hk::hook::trampoline([](const char *funcName) -> void* {
    void *result = NvnBootstrapHook.orig(funcName);

    if (strcmp(funcName, "nvnQueueInitialize") == 0) {
        tempQueueInitFuncPtr = (nvn::QueueInitializeFunc) result;
        return (void*) &queueInit;
    } 
    if (strcmp(funcName, "nvnCommandBufferInitialize") == 0) {
        tempBufferInitFuncPtr = (nvn::CommandBufferInitializeFunc) result;
        return (void*) &cmdBufInit;
    } 
    if (strcmp(funcName, "nvnWindowSetCrop") == 0) {
        tempSetCropFunc = (nvn::WindowSetCropFunc) result;
        return (void*) &setCrop;
    }
    if (strcmp(funcName, "nvnQueuePresentTexture") == 0) {
        tempPresentTexFunc = (nvn::QueuePresentTextureFunc) result;
        return (void*) &presentTexture;
    }
    if (strcmp(funcName, "nvnDeviceGetProcAddress") == 0) {
        tempGetProcAddressFuncPtr = (nvn::DeviceGetProcAddressFunc) result;
        return (void*) &getProc;
    }
    if (strcmp(funcName, "nvnDeviceInitialize") == 0) {
        tempDeviceInitFuncPtr = (nvn::DeviceInitializeFunc) result;
        return (void*) &deviceInit;
    
    }

    return result;
});

void nvnImGui::addDrawFunc(ProcDrawFunc func) {

    HK_ASSERT(!drawQueue.contains(func));//, "Function has already been added to queue!"

    drawQueue.push_back(func);
}

void nvnImGui::procDraw() {

    ImguiNvnBackend::newFrame();
    ImGui::NewFrame();

    for (auto drawFunc: drawQueue) {
        drawFunc();
    }

    ImGui::Render();
    ImguiNvnBackend::renderDrawData(ImGui::GetDrawData());
}

void nvnImGui::InstallHooks() {
    NvnBootstrapHook.installAtSym<"nvnBootstrapLoader">();
    DisableFullKeyState.installAtSym<"_ZN2nn3hid6detail13GetNpadStatesEPiPNS0_16NpadFullKeyStateEiRKj">();
    DisableHandheldState.installAtSym<"_ZN2nn3hid6detail13GetNpadStatesEPiPNS0_17NpadHandheldStateEiRKj">();
    DisableJoyDualState.installAtSym<"_ZN2nn3hid6detail13GetNpadStatesEPiPNS0_16NpadJoyDualStateEiRKj">();
    DisableJoyLeftState.installAtSym<"_ZN2nn3hid6detail13GetNpadStatesEPiPNS0_16NpadJoyLeftStateEiRKj">();
    DisableJoyRightState.installAtSym<"_ZN2nn3hid6detail13GetNpadStatesEPiPNS0_17NpadJoyRightStateEiRKj">();
}

bool nvnImGui::InitImGui() {
    if (nvnDevice && nvnQueue && nvnCmdBuf) {

        // Logger::log("Creating ImGui.\n");

        IMGUI_CHECKVERSION();

        ImGuiMemAllocFunc allocFunc = [](size_t size, void *user_data) {
            return nn::init::GetAllocator()->Allocate(size);
        };
    
        ImGuiMemFreeFunc freeFunc = [](void *ptr, void *user_data) {
            nn::init::GetAllocator()->Free(ptr);
        };
    
        ImGui::SetAllocatorFunctions(allocFunc, freeFunc, nullptr);

        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;

        ImGui::StyleColorsDark();

        ImguiNvnBackend::NvnBackendInitInfo initInfo = {
                .device = nvnDevice,
                .queue = nvnQueue,
                .cmdBuf = nvnCmdBuf
        };

        ImguiNvnBackend::InitBackend(initInfo);

        InputHelper::initKBM();

        InputHelper::setPort(0); // set input helpers default port to zero


#if IMGUI_USEEXAMPLE_DRAW
        IMGUINVN_DRAWFUNC(
                ImGui::ShowDemoWindow();
            //    ImGui::ShowStyleSelector("Style Selector");
            //        ImGui::ShowMetricsWindow();
            //        ImGui::ShowDebugLogWindow();
            //        ImGui::ShowStackToolWindow();
            //        ImGui::ShowAboutWindow();
            //        ImGui::ShowFontSelector("Font Selector");
            //        ImGui::ShowUserGuide();
        )
#endif
        hasInitImGui = true;
        return true;

    } else {
        // Logger::log("Unable to create ImGui Renderer!\n");
        hasInitImGui = false;
        return false;
    }
}