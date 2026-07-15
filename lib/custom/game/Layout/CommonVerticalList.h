#pragma once

#include "sead/math/seadVector.h"
#include "sead/prim/seadSafeString.h"

#include "al/Library/Nerve/NerveExecutor.h"

namespace al {
class LayoutActor;
class LayoutInitInfo;
class RollParts;
}  // namespace al

namespace nn::ui2d {
class TextureInfo;
}

struct RollPartsData {
    int mRollMsgCount = 0;          // 0x0
    const char16_t** mRollMsgList;  // 0x8
    int mSelectedIdx = 0;           // 0x10
    bool mIsLoop = true;            // 0x14
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
    void* getParts(int) const;
    s32 getRollPartsSelected(int idx);
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

    al::LayoutActor* mRootActor;
    void* field_18;
    al::LayoutActor** mListPartsArr;
    struct CursorParts* mCursorParts;
    struct ScrollBarParts* mScrollBarParts;
    int mListPartsNum;
    int mCurSelected;
    int field_40;
    void* field_48;
    void* field_50;
    sead::Vector2f mCursorPos;
    void* field_60;
    int mTopSelectableIdx;
    sead::WFixedSafeString<0x200>** mStringDataArr;
    sead::FixedSafeString<0x90>** mPaneNameList;
    void* field_80;
    void* field_88;
    const bool* mIsEnableData;
    int mStringDataCount;
    int mDataCount;
    void* field_a0;
    void* field_a8;
    void* field_b0;
    void* field_b8;
    RollPartsData* RollPartsArr;
    int field_c8;
};
