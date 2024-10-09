#pragma once

#include "al/layout/LayoutActor.h"

class CounterLifeCtrl : public al::LayoutActor {
public:
    void appear();
    void end();
    void kill();
};