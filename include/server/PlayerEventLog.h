#pragma once

#include <basis/seadTypes.h>

#include "Library/Message/IUseMessageSystem.h"
#include "Library/Message/MessageSystem.h"
#include "prim/seadSafeString.h"

constexpr s32 sNumEntries = 8;

class PlayerEventLog : public al::IUseMessageSystem {
public:
    enum Event { connect, disconnect, shine, purple, checkpoint };

    struct Entry {
        Entry();
        Entry(const char* player, Event event, sead::FixedSafeString<128> text);

        sead::FixedSafeString<16> mPlayer;
        Event mEvent = shine;
        sead::FixedSafeString<128> mText;
        s32 mNumPurples = 1;
        s32 mLife = 300;
    };

    PlayerEventLog(al::MessageSystem* messageSystem);

    void addEvent(const char* player, Event event, sead::FixedSafeString<128> text);
    void update();
    void kill();

    const al::MessageSystem* getMessageSystem() const override { return mMessageSystem; }

public:
    static PlayerEventLog* sInstance;

private:
    Entry mLog[8];
    al::MessageSystem* mMessageSystem = nullptr;
};
