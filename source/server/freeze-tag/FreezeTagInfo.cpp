#include "server/freeze-tag/FreezeTagInfo.h"

FreezeTagScore FreezeTagInfo::mPlayerTagScore;
int            FreezeTagInfo::mRoundLength = 10;
bool           FreezeTagInfo::mIsHostMode  = false;
bool           FreezeTagInfo::mIsDebugMode = false;

bool FreezeTagInfo::mHasMarioCollision = true;
bool FreezeTagInfo::mHasMarioBounce    = false;
bool FreezeTagInfo::mHasCappyCollision = true;
bool FreezeTagInfo::mHasCappyBounce    = false;
