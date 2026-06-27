#pragma once

#include "hk/mem/BssHeap.h"

#include "heap/seadHeap.h"
#include "prim/seadRuntimeTypeInfo.h"

// sead::Heap wrapper for the hakkun bss heap; used for functions that want a sead::Heap
namespace sead {
class HakkunHeap : public Heap {
    SEAD_RTTI_OVERRIDE(HakkunHeap, Heap)

public:
    HakkunHeap()
        : Heap("HakkunHeap", nullptr, nullptr, hk::mem::sMainHeap.getTotalSize(), cHeapDirection_Forward,
               false) {}

    void destroy() override { hk::mem::sMainHeap.destroy(); }
    size_t adjust() override {
        hk::mem::sMainHeap.adjust();
        return 0;
    }
    void* tryAlloc(size_t size, s32 alignment) override {
        return hk::mem::sMainHeap.allocate(size, alignment);
    }
    void free(void* ptr) override { hk::mem::sMainHeap.free(ptr); }
    void* resizeFront(void* p_void, size_t size) override { return nullptr; }
    void* resizeBack(void* p_void, size_t size) override { return nullptr; }
    void* tryRealloc(void* ptr, size_t size, s32 alignment) override {
        return hk::mem::sMainHeap.reallocate(ptr, size);
    }
    void freeAll() override { return; }
    uintptr_t getStartAddress() const override { return 0; }
    uintptr_t getEndAddress() const override { return 0; }
    size_t getSize() const override { return hk::mem::sMainHeap.getTotalSize(); }
    size_t getFreeSize() const override { return hk::mem::sMainHeap.getFreeSize(); }
    size_t getMaxAllocatableSize(int alignment) const override {
        return hk::mem::sMainHeap.getAllocatableSize();
    }
    bool isInclude(const void* p_void) const override { return false; }
    bool isEmpty() const override { return false; }
    bool isFreeable() const override { return true; }
    bool isResizable() const override { return false; }
    bool isAdjustable() const override { return true; }
    void dump() const override { return; }
    void dumpYAML(WriteStream& stream, int i) const override { return; }

public:
    static HakkunHeap* sInstance;
};
}  // namespace sead