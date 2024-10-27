#include "server/coinrunners/CoinRunnerInfo.h"

CoinRunnerScore CoinRunnerInfo::mPlayerTagScore;
int            CoinRunnerInfo::mRoundLength = 10;
bool           CoinRunnerInfo::mIsHostMode  = false;
bool           CoinRunnerInfo::mIsDebugMode = false;

bool CoinRunnerInfo::mHasMarioCollision = true;
bool CoinRunnerInfo::mHasMarioBounce    = false;
bool CoinRunnerInfo::mHasCappyCollision = true;
bool CoinRunnerInfo::mHasCappyBounce    = false;
