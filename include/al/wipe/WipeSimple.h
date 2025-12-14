#pragma once

#include "al/layout/LayoutActor.h"

namespace al {
class WipeSimple : public LayoutActor {
public:
    WipeSimple(const char* name, const char* layoutName, const LayoutInitInfo& info,
               const char* actorName);

    void startClose(s32 frames = -1);
    void tryStartClose(s32 frames = -1);
    void startCloseEnd();
    void startOpen(s32 frames = -1);
    void tryStartOpen(s32 frames = -1);
    bool isCloseEnd() const;
    bool isOpenEnd() const;
    void exeClose();
    void exeCloseEnd();
    void exeOpen();

    void appear() override;

public:
    s32 mFrames = -1;
};
}  // namespace al