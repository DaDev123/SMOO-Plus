#pragma once

#include "types.h"
#include "nn/result.h"

class SocketBase {

    public:
        SocketBase(const char* name);

        virtual nn::Result init(const char* ip, u16 port) = 0;
        virtual bool closeSocket();
        virtual bool tryReconnect();
        virtual struct Packet* tryGetPacket();

        const char* getStateChar();
        u8 getLogState();
        s32 getFd();

        void set_sock_flags(int flags);

        const char* getIP() { return this->sock_ip; }
        u16 getPort() { return this->port; }
        void setName(const char* name) { strcpy(sockName, name); };
        u32 socket_errno;

    protected:
        s32 socket_log(const char* str);
        s32 socket_read_char(char* out);

        char sockName[0x10] = {};
        char sock_ip[MAX_HOSTNAME_LENGTH] = {};

        u16 port;
        u8 socket_log_state = SOCKET_LOG_UNINITIALIZED;
        s32 socket_log_socket;

        int sock_flags;
};
