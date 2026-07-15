#pragma once

#include "al/Library/Layout/LayoutActor.h"
#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/Player/PlayerHolder.h"

class MapMini : public al::LayoutActor {
public:
    MapMini(const al::LayoutInitInfo&, const al::PlayerHolder*);
    void appearSlideIn(void);
    void end(void);
    void calcNearHintTrans(void);

    bool isEnd(void) const;

    void exeAppear(void);
    void exeWait(void);
    void exeEnd(void);
};
