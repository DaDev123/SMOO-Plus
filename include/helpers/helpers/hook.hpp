#pragma once

#include "types.h"
#include <functional>

namespace starlight {
    namespace hook {
        template<typename T>
        using function_ptr = T*;

        template<typename T>
        struct HookTrampoline {
            T original;
            T callback;
            
            HookTrampoline(T orig, T cb) : original(orig), callback(cb) {}
            
            template<typename... Args>
            auto operator()(Args&&... args) -> decltype(original(std::forward<Args>(args)...)) {
                return callback(std::forward<Args>(args)...);
            }
        };

        template<typename T>
        HookTrampoline<T> CreateTrampoline(T original, T callback) {
            return HookTrampoline<T>(original, callback);
        }
    }
}

// Simple hook macro for Starlight
#define HOOK_DEFINE_TRAMPOLINE(name) \
    struct name { \
        static void Callback(); \
        static void InstallAtSymbol(const char* symbol); \
        static void InstallAtOffset(uintptr_t offset); \
        static void InstallAtPtr(void* ptr); \
        static void InstallAtAddress(uintptr_t address); \
    private: \
        static void* Orig; \
    }; \
    void* name::Orig = nullptr; 