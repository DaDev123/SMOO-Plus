#include "Keyboard.hpp"

#include "hk/prim/traits/Integer.h"

#include "nn/swkbd/swkbd.h"

#include "main.hpp"

Keyboard::Keyboard()
    : al::AsyncFunctorThread("Swkbd", al::FunctorV0M(this, &Keyboard::threadFunction), 0, 16_KB,
                             sead::CoreId::cSub1) {
    mWorkBufSize = nn::swkbd::GetRequiredWorkBufferSize(false);
    mWorkBuf = (char*)gHeap->alloc(mWorkBufSize, 0x1000);
}

void Keyboard::threadFunction() {
    nn::swkbd::ShowKeyboardArg arg;

    nn::swkbd::MakePreset(&arg.keyboardConfig, nn::swkbd::Preset::Default);

    mSetupFunc(arg.keyboardConfig);
    arg.keyboardConfig._isUseTextCheck = false;

    nn::swkbd::SetHeaderTextUtf8(&arg.keyboardConfig, mHeaderText);
    nn::swkbd::SetSubTextUtf8(&arg.keyboardConfig, mSubText);

    arg.workBufSize = mWorkBufSize;
    arg.workBuf = mWorkBuf;

    if (mInitialText.calcLength() > 0) {
        nn::swkbd::SetInitialTextUtf8(&arg, mInitialText.cstr());
    }

    mIsCancelled = nn::swkbd::ShowKeyboard(&mResString, arg) ==
                   671;  // 671 = exit code for pressing x to cancel keyboard
}
