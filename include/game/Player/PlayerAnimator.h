#pragma once

#include "PlayerAnimFrameCtrl.h"
#include "PlayerModelHolder.h"
#include "sead/prim/seadSafeString.hpp"

class PlayerAnimator {
    public:
        void startAnim(const sead::SafeString &animName);
        void startSubAnim(const sead::SafeString &animName);
        void startSubAnimOnlyAir(const sead::SafeString &animName);
        void startUpperBodyAnimAndHeadVisKeep(const sead::SafeString &animName);
        void startAnimDead(void);
        void endSubAnim(void);

        void updateAnimFrame(void);
        void clearUpperBodyAnim(void);
        
        bool isAnim(const sead::SafeString &animName) const;
        bool isSubAnim(sead::SafeString const &subAnimName) const;
        bool isSubAnimEnd(void) const;
        bool isUpperBodyAnimAttached(void) const;

        float getAnimFrame() const;
        float getAnimFrameMax() const;
        float getAnimFrameRate() const;
        float getSubAnimFrame() const;
        float getSubAnimFrameMax() const;
        float getBlendWeight(int index);

        void setAnimRate(float);
        void setAnimRateCommon(float);
        void setAnimFrame(float);
        void setAnimFrameCommon(float);
        void setSubAnimFrame(float);
        void setSubAnimRate(float);
        void setBlendWeight(float,float,float,float,float,float);
        void setModelAlpha(float);
        void setPartsAnimRate(float, char const*);
        void setPartsAnimFrame(float, char const*);

        PlayerModelHolder *mModelHolder; // 0x0
        al::LiveActor *mPlayerDeco; // 0x8
        void *unkPtr; // 0x10
        PlayerAnimFrameCtrl *mAnimFrameCtrl; // 0x18
        sead::FixedSafeString<64> curAnim;  // 0x20 (64 bytes = 0x40)
        sead::FixedSafeString<64> curSubAnim; // 0x60 (64 bytes = 0x40)
        sead::FixedSafeString<64> curUpperBodyAnim; // 0xA0 (64 bytes = 0x40)
        sead::FixedSafeString<64> _E0; // 0xE0 (unknown string)
        unsigned char padding[0x1A2 - 0x120]; // 0x120 to 0x1A2
        bool mIsSubAnimPlaying; // 0x1A2
};