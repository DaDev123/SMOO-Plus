#pragma once

#include <basis/seadTypes.h>
#include <prim/seadSafeString.h>

constexpr s32 sNumEntries = 8;

class PlayerEventLog {
public:
    enum Event { connect, disconnect, start, shine, purple, checkpoint };

    struct Entry {
        Entry();
        Entry(sead::FixedSafeString<16> player, Event event, sead::FixedSafeString<128> text);

        sead::FixedSafeString<16> mPlayer;
        Event mEvent = shine;
        sead::FixedSafeString<128> mText;
        s32 mNumPurples = 1;
        s32 mLife = -1;
    };

    PlayerEventLog();

    void addEvent(sead::SafeString player, Event event, sead::SafeString text);
    void update();

    static s32 calculateLife();

    static const char* getShineMessage(sead::SafeString stage, sead::SafeString objId);
    static const char* getCheckpointMessage(sead::SafeString objId);
    static const char* getAchievementMessage(sead::SafeString label);

    static void toggleShow() { mIsShow = !mIsShow; }
    static bool isShow() { return mIsShow; }
    static void setShow(bool isShow) { mIsShow = isShow; }

public:
    static PlayerEventLog* sInstance;

private:
    Entry mLog[8];
    static bool mIsShow;
};
