#pragma once

#include "thread/seadAtomic.h"
#include "thread/seadSemaphore.h"

namespace sead
{
class Thread;

class ReadWriteLock
{
public:
    ReadWriteLock();
    ~ReadWriteLock();
    void readLock();
    void readUnlock();
    void writeLock();
    void writeUnlock();

public:
    class SemaphoreLock
    {
    public:
        SemaphoreLock();
        void lock();
        void unlock();

    public:
        Semaphore mSemaphore{0, 0x7FFF};
        Atomic<u32> mLockCount{0};
    };

    Atomic<u32> mNumReaders{0};
    Atomic<u32> mNumWriters{0};
    SemaphoreLock mReadLock;
    SemaphoreLock mWriteLock;
    Thread* mWriterThread = nullptr;
    u32 mWritingThreadCount{0};
};
}  // namespace sead
