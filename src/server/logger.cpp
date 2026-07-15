#include "logger.hpp"

#include "hk/diag/diag.h"

#include "nn/nifm.h"
#include "nn/socket.h"
#include "vapours/results/results_common.hpp"

#include <cstdio>
#include <cstdlib>
#include <netinet/in.h>
#include <std/musl/arch/generic/bits/socket.h>

#include "main.hpp"

// If connection fails, try X ports above the specified one
// Useful for debugging multple clients on the same machine
constexpr u32 ADDITIONAL_LOG_PORT_COUNT = 2;

Logger* Logger::sInstance = nullptr;

extern "C" void hk::diag::hkLogSink(const char* msg, size len) {
    Logger::log(msg);
}

void Logger::createInstance() {
#ifdef SERVERIP
    sInstance = new (gHeap) Logger(SERVERIP, 3080, "MainLogger");
#else
    sInstance = new (gHeap) Logger(0, 3080, "MainLogger");
#endif
}

bool Logger::init(const char* ip, u16 port) {
    mSockIp = ip;

    mPort = port;

    in_addr hostAddress = {0};
    sockaddr_in serverAddress = {0};

    if (mSockState != SockState::UNINITIALIZED)
        return false;

// emulators make this return false always, so skip it during init
#ifndef EMU

    if (!nn::nifm::IsNetworkAvailable()) {
        mSockState = SockState::UNAVAILABLE;
        return false;
    }

#endif

    if ((mSockFd = nn::socket::Socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0) {
        mSockState = SockState::UNAVAILABLE;
        return false;
    }

    nn::socket::InetAton(mSockIp.cstr(), &hostAddress);

    serverAddress.sin_addr = hostAddress;
    serverAddress.sin_port = nn::socket::InetHtons(mPort);
    serverAddress.sin_family = nn::socket::InetHtons(AF_INET);

    nn::Result result;
    bool connected = false;
    for (u32 i = 0; i < ADDITIONAL_LOG_PORT_COUNT + 1; ++i) {
        result = nn::socket::Connect(mSockFd, (sockaddr*)&serverAddress, sizeof(serverAddress));
        if (result.IsSuccess()) {
            connected = true;
            break;
        }
        mPort++;
        serverAddress.sin_port = nn::socket::InetHtons(mPort);
    }

    if (connected) {
        mSockState = SockState::CONNECTED;
        isDisableName = false;
        return true;
    } else {
        mSockState = SockState::UNAVAILABLE;
        return false;
    }
}

void Logger::log(const char* fmt, ...) {
    if (!sInstance || sInstance->mSockState != SockState::CONNECTED)
        return;
    va_list args;
    va_start(args, fmt);

    size_t buf_size = vsnprintf(nullptr, 0, fmt, args) + 1;
    size_t prefix_size = buf_size + 0x10;

    char buf[buf_size];

    if (vsnprintf(buf, buf_size, fmt, args) > 0) {
        if (!sInstance->isDisableName) {
            char prefix[prefix_size];
            snprintf(prefix, prefix_size, "[%s] %s", sInstance->mSockName, buf);
            sInstance->socket_log(prefix);
        } else {
            sInstance->socket_log(buf);
        }
    }

    va_end(args);
}
