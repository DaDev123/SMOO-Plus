#pragma once

#include "nn/account.h"

#include "sead/basis/seadTypes.h"
#include "sead/heap/seadDisposer.h"
#include "sead/prim/seadSafeString.h"

constexpr s32 sNumEntries = 8;

class PlayerEventLog {
    SEAD_SINGLETON_DISPOSER(PlayerEventLog)

public:
    enum Event { CONNECT, DISCONNECT, START, SHINE, PURPLE, CHECKPOINT, MOONROCK };

    struct Entry {
        Entry();
        Entry(nn::account::Uid player, Event event, sead::FixedSafeString<128> text);
        Entry(Event event, sead::FixedSafeString<128> text);

        nn::account::Uid mPlayerId;
        sead::FixedSafeString<16> mPlayerName;
        bool mIsPlayerName = false;
        Event mEvent = SHINE;
        sead::FixedSafeString<128> mText;
        s32 mNumPurples = 1;
        s32 mLife = -1;
    };

    PlayerEventLog();

    static void addEvent(nn::account::Uid player, Event event, sead::SafeString text);
    static void addSelfEvent(Event event, sead::SafeString text);
    void update();
    void tryUpdateNames();

    static s32 calculateLife();

    static const char* getShineMessage(sead::SafeString stage, sead::SafeString objId);
    static const char* getCheckpointMessage(sead::SafeString objId);
    static const char* getAchievementMessage(sead::SafeString label);

    static void toggleShow() { mIsShow = !mIsShow; }
    static bool isShow() { return mIsShow; }
    static void setShow(bool isShow) { mIsShow = isShow; }

private:
    Entry mLog[8];
    static bool mIsShow;
};
