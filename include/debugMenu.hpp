#pragma once

#include "sead/gfx/seadTextWriter.h"
#include "sead/gfx/seadViewport.h"

#include "agl/common/aglDrawContext.h"

extern sead::TextWriter* gTextWriter;

bool setupDebugMenu(agl::DrawContext* context, sead::Viewport* viewport);

void drawBackground(agl::DrawContext* context);

void drawChatBackground(agl::DrawContext* context, float rows);

void drawConnectionBackground(agl::DrawContext* context);

sead::Color4f loadColorFromFile(const char* file);
