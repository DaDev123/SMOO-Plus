#pragma once

#include <nn/font/font_Util.h>

namespace eui {

class ControlBase {
public:
    // is this a macro?
    virtual const char* getClassName() { return "ControlBase"; }

    NN_RUNTIME_TYPEINFO_BASE();

    ControlBase();
    virtual ~ControlBase() = default;

    virtual void Update(float);

    char filler[0x20];
};

}  // namespace eui
