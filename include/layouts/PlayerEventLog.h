#pragma once

#include <basis/seadTypes.h>
#include <prim/seadSafeString.h>

namespace al {
class AsyncFunctorThread;
}

constexpr s32 sNumEntries = 8;

class PlayerEventLog {
public:
    enum Event { connect, disconnect, shine, purple, checkpoint };

    struct Entry {
        Entry();
        Entry(sead::FixedSafeString<16> player, Event event, sead::FixedSafeString<128> text);

        sead::FixedSafeString<16> mPlayer;
        Event mEvent = shine;
        sead::FixedSafeString<128> mText;
        s32 mNumPurples = 1;
        s32 mLife = 300;
    };

    PlayerEventLog();

    void addEvent(sead::SafeStringBase<char> player, Event event, sead::SafeStringBase<char> text);
    void update();

    static const char* getShineMessage(sead::SafeStringBase<char> stage, sead::SafeStringBase<char> objId);
    static const char* getCheckpointMessage(sead::SafeStringBase<char> objId);
    static const char* getAchievementMessage(sead::SafeStringBase<char> label);

    void toggleHidden() { mIsHidden = !mIsHidden; }

public:
    static PlayerEventLog* sInstance;

private:
    Entry mLog[8];
    bool mIsHidden = false;
    al::AsyncFunctorThread* mThread = nullptr;
};

typedef void (PlayerEventLog::*LogThreadFunc)();
