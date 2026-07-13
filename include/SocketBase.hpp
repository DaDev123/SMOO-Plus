#pragma once

#include <cstring>

#include "prim/seadSafeString.h"
#include "types.h"

class SocketBase {
public:
    SocketBase(const char* name);

    const char* getStateChar();
    SockState getLogState();
    s32 getFd();

    void set_sock_flags(int flags);

    const char* getIP() { return this->sock_ip.cstr(); }
    u16 getPort() { return this->port; }
    void setName(const char* name) {
        std::strncpy(sockName, name ? name : "", sizeof(sockName) - 1);
        sockName[sizeof(sockName) - 1] = '\0';
    };
    u32 socket_errno = 0;

protected:
    s32 socket_log(const char* str);
    s32 socket_read_char(char* out);

    char sockName[0x10] = {};
    sead::FixedSafeString<64> sock_ip;

    u16 port = 0;
    SockState socket_log_state = SockState::UNINITIALIZED;
    s32 socket_log_socket = -1;

    int sock_flags = 0;
};
