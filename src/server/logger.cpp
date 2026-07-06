#include "logger.hpp"

#include "hk/diag/diag.h"

#include "nn/nifm.h"
#include "nn/socket.h"
#include "vapours/results/results_common.hpp"

#include <cstdio>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>

// If connection fails, try X ports above the specified one
// Useful for debugging multple clients on the same machine
constexpr u32 ADDITIONAL_LOG_PORT_COUNT = 2;

Logger* Logger::sInstance = nullptr;

extern "C" void hk::diag::hkLogSink(const char* msg, size len) {
    Logger::log(msg);
}

void Logger::createInstance() {
#ifdef SERVERIP
    sInstance = new Logger(TOSTRING(SERVERIP), 3080, "MainLogger");
#else
    sInstance = new Logger(0, 3080, "MainLogger");
#endif
}

bool Logger::init(const char* ip, u16 port) {
    sock_ip = ip;

    this->port = port;

    in_addr hostAddress = {0};
    sockaddr_in serverAddress = {0};

    if (this->socket_log_state != SockState::UNINITIALIZED)
        return false;

// emulators make this return false always, so skip it during init
#ifndef EMU

    if (!nn::nifm::IsNetworkAvailable()) {
        this->socket_log_state = SockState::UNAVAILABLE;
        return false;
    }

#endif

    if ((this->socket_log_socket = nn::socket::Socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0) {
        this->socket_log_state = SockState::UNAVAILABLE;
        return false;
    }

    nn::socket::InetAton(this->sock_ip.cstr(), &hostAddress);

    serverAddress.sin_addr = hostAddress;
    serverAddress.sin_port = nn::socket::InetHtons(this->port);
    serverAddress.sin_family = nn::socket::InetHtons(AF_INET);

    nn::Result result;
    bool connected = false;
    for (u32 i = 0; i < ADDITIONAL_LOG_PORT_COUNT + 1; ++i) {
        result =
            nn::socket::Connect(this->socket_log_socket, (sockaddr*)&serverAddress, sizeof(serverAddress));
        if (result.IsSuccess()) {
            connected = true;
            break;
        }
        this->port++;
        serverAddress.sin_port = nn::socket::InetHtons(this->port);
    }

    if (connected) {
        this->socket_log_state = SockState::CONNECTED;
        this->isDisableName = false;
        return true;
    } else {
        this->socket_log_state = SockState::UNAVAILABLE;
        return false;
    }
}

void Logger::log(const char* fmt, ...) {
    if (!sInstance || sInstance->socket_log_state != SockState::CONNECTED)
        return;
    va_list args;
    va_start(args, fmt);

    size_t buf_size = vsnprintf(nullptr, 0, fmt, args) + 1;
    size_t prefix_size = buf_size + 0x10;

    char buf[buf_size];

    if (vsnprintf(buf, buf_size, fmt, args) > 0) {
        if (!sInstance->isDisableName) {
            char prefix[prefix_size];
            snprintf(prefix, prefix_size, "[%s] %s", sInstance->sockName, buf);
            sInstance->socket_log(prefix);
        } else {
            sInstance->socket_log(buf);
        }
    }

    va_end(args);
}
