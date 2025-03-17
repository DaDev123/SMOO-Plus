#include "hk/os/Mutex.h"

struct Guard {
    bool initialized;
    hk::os::Mutex mutex;
};

extern "C" {

bool hk_guard_acquire(Guard* guard) {
    if (guard->initialized)
        return false;
    guard->mutex.lock();
    return true;
}

void hk_guard_release(Guard* guard) {
    guard->initialized = true;
    guard->mutex.unlock();
}

void hk_atexit(void (*)()) {
    // ...
}
}
