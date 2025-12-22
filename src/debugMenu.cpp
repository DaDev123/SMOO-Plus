#include <cstdlib>

#include "BloodMoon/BloodMoonUtils.hpp"
#include "agl/utility/aglDevTools.h"
#include "al/Library/File/FileUtil.h"
#include "al/Library/System/GameSystemInfo.h"
#include "al/Project/Memory/Util.h"
#include "debugMenu.hpp"
#include "game/System/GameSystem.h"
#include "sead/gfx/nvn/seadDebugFontMgrNvn.h"
#include "sead/gfx/seadPrimitiveRenderer.h"

static const char* DBG_FONT_PATH = "DebugData/Font/nvn_font_jis1.ntx";
static const char* DBG_SHADER_PATH = "DebugData/Font/nvn_font_shader_jis1.bin";
static const char* DBG_TBL_PATH = "DebugData/Font/nvn_font_jis1_tbl.bin";

sead::TextWriter* gTextWriter;
static sead::Color4f DMBgColor(0.1f, 0.1f, 0.9f, 0.9f);    // Debug menu
static sead::Color4f ChatBgColor(0.1f, 0.1f, 0.9f, 0.9f);  // Chat

void setupDebugMenu(GameSystem* gSys) {
    sead::Heap* curHeap = al::getCurrentHeap();

    agl::DrawContext* context = gSys->mSystemInfo->drawSystemInfo->drawContext;

    if (curHeap) {
        if (context) {
            sead::DebugFontMgrJis1Nvn::sInstance =
                sead::DebugFontMgrJis1Nvn::createInstance(curHeap);

            if (al::isExistFile(DBG_FONT_PATH) && al::isExistFile(DBG_SHADER_PATH) &&
                al::isExistFile(DBG_TBL_PATH)) {
                sead::DebugFontMgrJis1Nvn::sInstance->initialize(
                    curHeap, DBG_SHADER_PATH, DBG_FONT_PATH, DBG_TBL_PATH, 0x100000);
                sead::TextWriter::setDefaultFont(sead::DebugFontMgrJis1Nvn::sInstance);
                gTextWriter = new sead::TextWriter(context);

                if (gTextWriter) {
                    gTextWriter->setupGraphics(context);
                }
            }

            sead::PrimitiveDrawer drawer(context);
        }
    }

    __asm("MOV W23, #0x3F800000");
    __asm("MOV W8, #0xFFFFFFFF");

    if (al::isExistFile("OnlineData/DebugMenuColor.txt")) {
        DMBgColor = loadColorFromFile("OnlineData/DebugMenuColor.txt");
    }

    if (al::isExistFile("OnlineData/ChatBGColor.txt")) {
        ChatBgColor = loadColorFromFile("OnlineData/ChatBGColor.txt");
    }
}
void drawBackground(agl::DrawContext* context) {
    sead::Vector3<float> p1(-1, .3, 0);
    sead::Vector3<float> p2(-.2, .3, 0);
    sead::Vector3<float> p3(-1, -1, 0);
    sead::Vector3<float> p4(-.2, -1, 0);

    agl::utl::DevTools::beginDrawImm(context, sead::Matrix34<float>::ident,
                                     sead::Matrix44<float>::ident);
    agl::utl::DevTools::drawTriangleImm(context, p1, p2, p3, DMBgColor);
    agl::utl::DevTools::drawTriangleImm(context, p3, p4, p2, DMBgColor);
}

void drawChatBackground(agl::DrawContext* context, float rows) {
    sead::Vector3<float> p1(-1, -.50 - 0.05 * rows, 0);   // top left
    sead::Vector3<float> p2(-.2, -.50 - 0.05 * rows, 0);  // top right
    sead::Vector3<float> p3(-1, -.7, 0);                  // bottom left
    sead::Vector3<float> p4(-.2, -.7, 0);                 // bottom right

    agl::utl::DevTools::beginDrawImm(context, sead::Matrix34<float>::ident,
                                     sead::Matrix44<float>::ident);
    agl::utl::DevTools::drawTriangleImm(context, p1, p2, p3, ChatBgColor);
    agl::utl::DevTools::drawTriangleImm(context, p3, p4, p2, ChatBgColor);
}

// ===== BACKGROUND DRAW FUNCTION =====
void drawConnectionBackground(agl::DrawContext* context) {
    sead::Vector3<float> p1(.65, 1, 0);    // top left (at very top)
    sead::Vector3<float> p2(1, 1, 0);      // top right (at very top)
    sead::Vector3<float> p3(.65, .76, 0);  // bottom left (smaller box)
    sead::Vector3<float> p4(1, .76, 0);    // bottom right (smaller box)
    sead::Color4f c(.1, .1, .1, .9);

    agl::utl::DevTools::beginDrawImm(context, sead::Matrix34<float>::ident,
                                     sead::Matrix44<float>::ident);
    agl::utl::DevTools::drawTriangleImm(context, p1, p2, p3, c);
    agl::utl::DevTools::drawTriangleImm(context, p3, p4, p2, c);
}

sead::Color4f loadColorFromFile(const char* file) {
    size_t fileSize = 0;
    u8* fileData = BloodMoon::loadFile(file, &fileSize);
    sead::Color4f color(0.1f, 0.1f, 0.1f, 0.9f);  // Default

    if (!fileData || fileSize == 0)
        return color;

    char* buffer = reinterpret_cast<char*>(fileData);
    char* savePtr = nullptr;
    char* line = strtok_r(buffer, "\n\r", &savePtr);

    while (line) {
        while (*line == ' ' || *line == '\t')
            line++;
        if (*line == '\0' || *line == '#') {
            line = strtok_r(nullptr, "\n\r", &savePtr);
            continue;
        }

        char* savePtr2 = nullptr;
        char* token = strtok_r(line, ",", &savePtr2);
        float values[4] = {0.1f, 0.1f, 0.1f, 0.9f};
        int index = 0;

        while (token && index < 4) {
            values[index++] = std::atof(token);  // std::atof statt al::parseFloatFromString
            token = strtok_r(nullptr, ",", &savePtr2);
        }

        color = sead::Color4f(values[0], values[1], values[2], values[3]);
        break;  // nur erste g�ltige Zeile
    }

    delete[] fileData;
    return color;
}