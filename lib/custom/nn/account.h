/**
 * @file account.h
 * @brief Account service implementation.
 */

#pragma once

#include "nn/os.h"

#include "logger.hpp"
#include "types.h"

namespace nn {
namespace account {
// typedef char Nickname[0x21];
// typedef u64 Uid[0x2];

struct Nickname {
    char name[0x21] = {};
};

struct Uid {
    char data[0x10] = {};

    bool operator==(const Uid& rhs) const { return memcmp(data, rhs.data, 0x10) == 0; }

    Uid& operator=(const Uid& other) {
        memcpy(this->data, other.data, 0x10);
        return *this;
    }

    inline bool isEmpty() const { return *this == EmptyId; }

    inline void print() const {
        Logger::log("Player ID: 0x");
        Logger::disableName();
        for (size_t i = 0; i < 0x10; i++) {
            Logger::log("%02X", data[i]);
        }
        Logger::log("\n");
        Logger::enableName();
    }

    inline void print(const char* prefix) const {
        Logger::log("%s: 0x", prefix);
        Logger::disableName();
        for (size_t i = 0; i < 0x10; i++) {
            Logger::log("%02X", data[i]);
        }
        Logger::log("\n");
        Logger::enableName();
    }

    static const Uid EmptyId;
};

typedef u64 NetworkServiceAccountId;

class AsyncContext;
struct UserHandle;

void Initialize();
Result ListAllUsers(s32*, nn::account::Uid*, s32 numUsers);
Result OpenUser(nn::account::UserHandle*, const nn::account::Uid&);
Result IsNetworkServiceAccountAvailable(bool* out, const nn::account::UserHandle&);
void CloseUser(const nn::account::UserHandle&);

Result EnsureNetworkServiceAccountAvailable(const nn::account::UserHandle& userHandle);
Result EnsureNetworkServiceAccountIdTokenCacheAsync(nn::account::AsyncContext*,
                                                    const nn::account::UserHandle&);
Result LoadNetworkServiceAccountIdTokenCache(u64*, char*, u64, const nn::account::UserHandle&);

Result GetLastOpenedUser(nn::account::Uid*);
Result GetNickname(nn::account::Nickname* nickname, const nn::account::Uid& userID);

class AsyncContext {
public:
    AsyncContext();

    Result HasDone(bool*);
    Result GetResult();
    Result Cancel();
    Result GetSystemEvent(nn::os::SystemEvent*);
};
};  // namespace account
};  // namespace nn
