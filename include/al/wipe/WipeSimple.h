#pragma once

#include "al/layout/LayoutActor.h"

namespace al {
class WipeSimple : al::LayoutActor {
    WipeSimple(const char*, char*, const al::LayoutInitInfo*, al::LayoutActor*, const char*);
    void appear() override;

    void startClose();
    void tryStartClose();
    void startCloseEnd();
    void StartOpen();
    void tryStartOpen();
    void isCloseEnd() const;
    void isOpenEnd() const;

    void exeClose();
    void exeCloseEnd();
    void exeOpen();
};
}  // namespace al