#pragma once

#include "nn/swkbd/swkbd.h"

#include "sead/prim/seadSafeString.h"

#include "al/Library/Thread/AsyncFunctorThread.h"

#include <cstddef>

typedef void (*KeyboardSetup)(nn::swkbd::KeyboardConfig&);
const u8 MAX_HOSTNAME_LENGTH = 50;
typedef sead::FixedSafeString<MAX_HOSTNAME_LENGTH + 1> hostname;

inline char* mResBuf = (char*)malloc(nn::swkbd::GetRequiredStringBufferSize());
inline nn::swkbd::String mResString = nn::swkbd::String(nn::swkbd::GetRequiredStringBufferSize(), mResBuf);

class Keyboard {
public:
    Keyboard();
    void keyboardThread();

    void openKeyboard(const char* initialText, KeyboardSetup setup);

    const char* getResult() {
        if (mThread->isDone()) {
            return mResString.cstr();
        }
        return nullptr;
    };

    bool isKeyboardCancelled() const { return mIsCancelled; }

    bool isThreadDone() { return mThread->isDone(); }

    void setHeaderText(const char16_t* text) { mHeaderText = text; }
    void setSubText(const char16_t* text) { mSubText = text; }

private:
    al::AsyncFunctorThread* mThread = nullptr;
    nn::swkbd::ShowKeyboardArg mKeyboardArg = nn::swkbd::ShowKeyboardArg();

    hostname mInitialText = sead::FixedSafeString<MAX_HOSTNAME_LENGTH + 1>();
    KeyboardSetup mSetupFunc = KeyboardSetup();

    const char16_t* mHeaderText = u"Enter Server IP Here!";
    const char16_t* mSubText = u"Must be a Valid Address.";

    bool mIsCancelled = false;

    char* mWorkBuf = nullptr;
    int mWorkBufSize = 0;
    char* mTextCheckBuf = nullptr;
    int mTextCheckSize = 0;
};
