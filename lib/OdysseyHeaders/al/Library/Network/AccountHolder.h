#pragma once

#include <nn/account.h>

#include "Library/HostIO/HioNode.h"

namespace al {
class AccountHolder : public HioNode {
public:
    AccountHolder();
    virtual ~AccountHolder();

    bool isValid() const;

public:
    nn::account::UserHandle* mUserHandle = nullptr;
    nn::account::Uid mUid{};
    bool mIsValid = false;
};

static_assert(sizeof(AccountHolder) == 0x28);
}  // namespace al
