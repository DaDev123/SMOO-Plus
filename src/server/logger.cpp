#include "logger.hpp"

#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>

#include "nn/nifm.h"
#include "nn/socket.h"
#include "vapours/results/results_common.hpp"

// If connection fails, try X ports above the specified one
// Useful for debugging multple clients on the same machine
constexpr u32 ADDITIONAL_LOG_PORT_COUNT = 2;

Logger* Logger::sInstance = nullptr;

void Logger::createInstance() {
#ifdef SERVERIP
    sInstance = new Logger(TOSTRING(SERVERIP), 3080, "MainLogger");
#else
    sInstance = new Logger(0, 3080, "MainLogger");
#endif
}

nn::Result Logger::init(const char* ip, u16 port) {
    sock_ip = ip;

    this->port = port;

    in_addr hostAddress = {0};
    sockaddr_in serverAddress = {0};

    if (this->socket_log_state != SOCKET_LOG_UNINITIALIZED)
        return nn::Result(-1);

    nn::nifm::Initialize();
    nn::nifm::SubmitNetworkRequest();

    while (nn::nifm::IsNetworkRequestOnHold()) {
    }

// emulators make this return false always, so skip it during init
#ifndef EMU

    if (!nn::nifm::IsNetworkAvailable()) {
        this->socket_log_state = SOCKET_LOG_UNAVAILABLE;
        return nn::Result(-1);
    }

#endif

    if ((this->socket_log_socket = nn::socket::Socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0) {
        this->socket_log_state = SOCKET_LOG_UNAVAILABLE;
        return nn::Result(nn::socket::GetLastErrno());
    }

    nn::socket::InetAton(this->sock_ip, &hostAddress);

    serverAddress.sin_addr = hostAddress;
    serverAddress.sin_port = nn::socket::InetHtons(this->port);
    serverAddress.sin_family = nn::socket::InetHtons(AF_INET);

    nn::Result result;
    bool connected = false;
    for (u32 i = 0; i < ADDITIONAL_LOG_PORT_COUNT + 1; ++i) {
        result = nn::socket::Connect(this->socket_log_socket, (sockaddr*)&serverAddress, sizeof(serverAddress));
        if (result.IsSuccess()) {
            connected = true;
            break;
        }
        this->port++;
        serverAddress.sin_port = nn::socket::InetHtons(this->port);
    }

    if (connected) {
        this->socket_log_state = SOCKET_LOG_CONNECTED;
        this->isDisableName = false;
        return nn::Result(0);
    } else {
        this->socket_log_state = SOCKET_LOG_UNAVAILABLE;
        return result;
    }
}

void Logger::log(const char* fmt, va_list args) {  // impl for replacing seads system::print
    if (!sInstance)
        return;
    char* buf = (char*)malloc(0x500);
    if (nn::util::VSNPrintf(buf, 0x500, fmt, args) > 0) {
        sInstance->socket_log(buf);
    }
}

s32 Logger::read(char* out) {
    return this->socket_read_char(out);
}

void Logger::log(const char* fmt, ...) {
    if (!sInstance || sInstance->socket_log_state != SOCKET_LOG_CONNECTED)
        return;
    va_list args;
    va_start(args, fmt);

    size_t buf_size = 0x500;
    size_t prefix_size = buf_size + 0x10;

    char* buf = (char*)malloc(buf_size);

    if (nn::util::VSNPrintf(buf, buf_size, fmt, args) > 0) {
        if (!sInstance->isDisableName) {
            char* prefix = (char*)malloc(prefix_size);
            nn::util::SNPrintf(prefix, prefix_size, "[%s] %s", sInstance->sockName, buf);
            sInstance->socket_log(prefix);
            free(prefix);
        } else {
            sInstance->socket_log(buf);
        }
    }

    va_end(args);

    free(buf);
}

bool Logger::pingSocket() {
    return socket_log("ping") > 0;  // if value is greater than zero, than the socket recieved our
                                    // message, otherwise the connection was lost.
}

void tryInitSocket() {
    __asm("STR X20, [X8,#0x18]");
#if DEBUGLOG
    Logger::createInstance();  // creates a static instance for debug logger
#endif
}