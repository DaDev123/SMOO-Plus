#include "hk/hook/Trampoline.h"
#include "hk/hook/MapUtil.h"

namespace hk::hook {

    namespace detail {

        static ptr sRwAddr = 0;
        section(.text) static TrampolineBackup sTrampolinePoolData[HK_HOOK_TRAMPOLINE_POOL_SIZE];

        static void* mapRw() {
            HK_ABORT_UNLESS_R(mapRoToRw(ptr(sTrampolinePoolData), sizeof(sTrampolinePoolData), &sRwAddr));
            return cast<void*>(sRwAddr);
        }

        util::PoolAllocator<TrampolineBackup, HK_HOOK_TRAMPOLINE_POOL_SIZE> sTrampolinePool { mapRw() };

        ptr TrampolineBackup::getRx() const {
            ptr rw = ptr(this);

            return ptr(sTrampolinePoolData) + (rw - sRwAddr);
        }

    } // namespace detail

} // namespace hk::hook
