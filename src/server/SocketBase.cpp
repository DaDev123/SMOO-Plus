#include "SocketBase.hpp"

#include "nn/socket.h"

#include <cstring>

#include "types.h"

SocketBase::SocketBase(const char* name) {
    setName(name);
}

const char* SocketBase::getStateChar() {
    switch (this->socket_log_state) {
    case SockState::CONNECTED:
        return "Socket Connected";
    case SockState::UNAVAILABLE:
        return "Socket Unavailable";
    case SockState::UNINITIALIZED:
        return "Socket Uninitialized";
    case SockState::DISCONNECTED:
        return "Socket Disconnected";
    case SockState::CONNFAIL:
        return "Connection Failed";
    case SockState::INVALIP:
        return "Invalid IP or hostname";
    case SockState::NONET:
        return "Network not available";
    default:
        return "Unknown State";
    }
}

SockState SocketBase::getLogState() {
    return this->socket_log_state;
}

void SocketBase::set_sock_flags(int flags) {
    this->sock_flags = flags;
}

s32 SocketBase::socket_log(const char* str) {
    if (this->socket_log_state != SockState::CONNECTED)
        return -1;

    return nn::socket::Send(this->socket_log_socket, str, strlen(str), 0);
}

s32 SocketBase::socket_read_char(char* out) {
    if (this->socket_log_state != SockState::CONNECTED)
        return -2;

    char buf[0x1000];

    int valread = nn::socket::Recv(this->socket_log_socket, buf, sizeof(buf), this->sock_flags);

    if (valread <= 0)
        return valread;
    const s32 safeLength = valread < static_cast<s32>(sizeof(buf)) ? valread : sizeof(buf) - 1;
    buf[safeLength] = '\0';
    strncat(out, buf, safeLength);
    return valread;
}

s32 SocketBase::getFd() {
    if (this->socket_log_state == SockState::CONNECTED) {
        return this->socket_log_socket;
    } else {
        return -1;
    }
}
