#include "Keyboard.hpp"

#include "hk/prim/traits/Integer.h"

#include "nn/swkbd/swkbd.h"

Keyboard::Keyboard(ulong strSize) : mResultString(strSize) {
    mThread =
        new al::AsyncFunctorThread("Swkbd", al::FunctorV0M(this, &Keyboard::keyboardThread), 0, 16_KB, {0});

    mWorkBufSize = nn::swkbd::GetRequiredWorkBufferSize(false);
    mWorkBuf = (char*)aligned_alloc(0x1000, mWorkBufSize);

    mTextCheckSize = 0x1000;
    mTextCheckBuf = (char*)malloc(mTextCheckSize);

    mCustomizeDicSize = 0x1000;
    mCustomizeDicBuf = (char*)malloc(mCustomizeDicSize);

    mResultString.allocate();
}

void Keyboard::keyboardThread() {
    memset(&mKeyboardArg, 0, sizeof(mKeyboardArg));
    nn::swkbd::MakePreset(&mKeyboardArg.keyboardConfig, nn::swkbd::Preset::Default);

    mSetupFunc(mKeyboardArg.keyboardConfig);

    nn::swkbd::SetHeaderText(&mKeyboardArg.keyboardConfig, mHeaderText);
    nn::swkbd::SetSubText(&mKeyboardArg.keyboardConfig, mSubText);

    mKeyboardArg.workBufSize = mWorkBufSize;
    mKeyboardArg.textCheckWorkBufSize = mTextCheckSize;
    mKeyboardArg._customizeDicBufSize = mCustomizeDicSize;

    mKeyboardArg.workBuf = mWorkBuf;
    mKeyboardArg.textCheckWorkBuf = mTextCheckBuf;
    mKeyboardArg._customizeDicBuf = mCustomizeDicBuf;

    if (mInitialText.calcLength() > 0) {
        nn::swkbd::SetInitialTextUtf8(&mKeyboardArg, mInitialText.cstr());
    }

    mIsCancelled = nn::swkbd::ShowKeyboard(&mResultString, mKeyboardArg) ==
                   671;  // 671 = exit code for pressing x to cancel keyboard
}

void Keyboard::openKeyboard(const char* initialText, KeyboardSetup setupFunc) {
    mInitialText = initialText;
    mSetupFunc = setupFunc;

    mThread->start();
}