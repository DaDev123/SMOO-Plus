#pragma once

#include "sead/devenv/seadDebugFontMgrNvn.h"
#include "sead/gfx/seadTextWriter.h"
#include "sead/gfx/seadViewport.h"

#include "agl/DrawContext.h"

extern sead::TextWriter* gTextWriter;

bool setupDebugMenu(agl::DrawContext* context, sead::Viewport* viewport);

void drawBackground(agl::DrawContext* context);
