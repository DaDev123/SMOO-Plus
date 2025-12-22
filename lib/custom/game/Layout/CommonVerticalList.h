#pragma once

#include "Library/Layout/LayoutActor.h"
#include "Library/Layout/LayoutInitInfo.h"
#include "Library/Nerve/NerveExecutor.h"
#include "math/seadVector.h"
#include "prim/seadSafeString.h"

namespace nn::ui2d {
class TextureInfo;
}

struct RollPartsData {
    int mRollMsgCount = 0;         // 0x0
    const char16_t** mRollMsgList; // 0x8
    int unkInt1 = 0;               // 0x10
    bool mUnkBool = false;         // 0x14
};

class CommonVerticalList : public al::NerveExecutor {
public:
    CommonVerticalList(al::LayoutActor*, const al::LayoutInitInfo&, bool);
    ~CommonVerticalList();

    void activate(void);
    void addGroupAnimData(const sead::FixedSafeString<64>*, const char*);
    void addStringData(const sead::WFixedSafeString<512>*, const char*);
    void appearCursor(void);
    void calcAnimRate(void);
    void calcCursorPos(sead::Vector2f*);
    void deactivate(void);
    void decide(void);
    void down(void);
    void endCursor(void);
    void getListPartsNum(void);
    void getParts(int);
    void getRollPartsSelected(int);
    void getSelectedParts(void);
    void hideAll(void);
    void hideCursor(void);
    void initData(int);
    void initDataNoResetSelected(int);
    void initDataWithIdx(int, int, int);
    void jumpBottom(void);
    void jumpTop(void);
    void pageDown(void);
    void pageUp(void);
    void reject(void);
    void rollLeft(void);
    void rollRight(void);
    void setEnableData(const bool*);
    void setImageData(nn::ui2d::TextureInfo**, const char*);
    void setRollPartsData(RollPartsData*);
    void setRollPartsSelected(int, int);
    void setSelectedIdx(int, int);
    void startLoopActionAll(const char*, const char*);
    void up(void);
    void update(void);
    void updateCursorPos(void);
    void updateParts(void);

    bool isActive(void) const;
    bool isDeactive(void) const;
    bool isDecideEnd(void) const;
    bool isRejectEnd(void) const;

    void exeActive(void);
    void exeDeactive(void);
    void exeDecide(void);
    void exeDecideEnd(void);
    void exeReject(void);
    void exeRejectEnd(void);

    al::LayoutActor* mRootActor;                    // 0x10
    void* unkPtr1;                                  // 0x18
    void* mListPartsArr;                            // 0x20
    struct CursorParts* mCursorParts;               // 0x28
    struct ScrollBarParts* mScrollBarParts;         // 0x30
    int mListPartsNum;                              // 0x38
    int mCurSelected;                               // 0x3C
    int mIdx;                                       // 0x40
    void* unkPtr2;                                  // 0x48
    void* unkPtr3;                                  // 0x50
    sead::Vector2f mCursorPos;                      // 0x58
    void* unkPtr4;                                  // 0x60
    int unkInt1;                                    // 0x68
    sead::WFixedSafeString<0x200>** mStringDataArr; // 0x70
    sead::FixedSafeString<0x90>** mPaneNameList;    // 0x78
    void* unkPtr8;                                  // 0x80
    void* unkPtr9;                                  // 0x88
    const bool* mIsEnableData;                      // 0x90
    int mStringDataCount;                           // 0x98
    int mDataCount;                                 // 0x9C
    void* unkPtr12;                                 // 0xA0
    void* unkPtr13;                                 // 0xA8
    void* unkPtr14;                                 // 0xB0
    void* unkPtr15;                                 // 0xB8
    void* RollPartsArr;                             // 0xC0
    void* unkPtrX;                                  // 0xC8
};

static_assert(sizeof(CommonVerticalList) == 0xD0, "CommonVerticalList size");
