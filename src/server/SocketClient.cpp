#include "server/SocketClient.hpp"

#include "hk/diag/diag.h"

#include "nn/nifm.h"
#include "nn/os.h"
#include "nn/socket.h"
#include "vapours/results/results_common.hpp"

#include <cerrno>
#include <climits>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "heap/seadHeapMgr.h"
#include "Library/Thread/AsyncFunctorThread.h"
#include "Library/Thread/FunctorV0M.h"
#include "logger.hpp"
#include "main.hpp"
#include "server/LegacyProtocol.hpp"
#include "server/Client.hpp"
#include "types.h"

SocketClient::SocketClient() : SocketBase("SocketClient") {
    mRecvQueue.allocate(100, gHeap);
    mSendQueue.allocate(100, gHeap);

    mSocketThread = new al::AsyncFunctorThread(
        "SocketMainThread", al::FunctorV0M(this, &SocketClient::update), 0, 16_KB, sead::CoreId::cMain);
    mRecvThread = new al::AsyncFunctorThread(
        "SocketRecvThread", al::FunctorV0M(this, &SocketClient::recvFunc), 0, 16_KB, sead::CoreId::cMain);
    mSendThread = new al::AsyncFunctorThread(
        "SocketSendThread", al::FunctorV0M(this, &SocketClient::sendFunc), 0, 16_KB, sead::CoreId::cMain);
}

void SocketClient::update() {
    // didnt use al nerves to be safe but idk maybe that couldve worked too
    sead::ScopedCurrentHeapSetter setter(gHeap);

    while (true) {
        switch (mState) {
        case WAIT:
            break;
        case INIT:
            exeInit();
            break;
        case RESET:
            exeReset();
            break;
        case RECONNECT:
            if (Client::instance()->mIsAllowReconnect) {
                nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(mReconnectBackoffMs * 1000000LL));
                Client::instance()->startThread();
            } else
                mState = WAIT;
            break;
        }
        nn::os::YieldThread();
        nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(100000000));
    }
}

void SocketClient::init(const char* ip, u16 port) {
    this->sock_ip = ip;
    this->port = port;

    if (mSocketThread->isDone())
        mSocketThread->start();

    mState = INIT;
}

bool SocketClient::exeInit() {
    // TODO: add back the errors or an equivalent of them that doesnt pause the game
    hk::diag::logLine("socket client init");

// emulators (ryujinx) make this return false always, so skip it during init
#ifndef EMU
    for (s32 networkFails = 0; networkFails <= 20; networkFails++) {
        if (networkFails == 20) {
            this->socket_log_state = SockState::NONET;
            this->socket_errno = nn::socket::GetLastErrno();

            mState = WAIT;
            return false;
        }

        if (nn::nifm::IsNetworkAvailable())
            break;

        nn::os::YieldThread();
        nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(500_ms));
    }
#endif

    in_addr hostAddress = {0};
    sockaddr_in serverAddress = {0};

    hk::diag::logLine("SocketClient::exeInit: %s:%d sock %s", getIP(), this->port, getStateChar());

    if (!this->stringToIPAddress(this->sock_ip.cstr(), &hostAddress)) {
        hk::diag::logLine("IP address is invalid or hostname not resolveable.");
        this->socket_errno = nn::socket::GetLastErrno();
        this->socket_log_state = SockState::INVALIP;

        mState = WAIT;
        return false;
    }

    for (s32 socketFails = 0; socketFails <= 20; socketFails++) {
        if (socketFails == 20) {
            hk::diag::logLine("Socket Unavailable.");
            this->socket_errno = nn::socket::GetLastErrno();
            this->socket_log_state = SockState::UNAVAILABLE;

            mState = WAIT;
            return false;
        }

        if ((this->socket_log_socket = nn::socket::Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
            nn::os::YieldThread();
            nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(500_ms));
            continue;
        }

        serverAddress.sin_addr = hostAddress;
        serverAddress.sin_port = nn::socket::InetHtons(this->port);
        serverAddress.sin_family = nn::socket::InetHtons(AF_INET);

        s32 sockOptValue = 1;
        nn::socket::SetSockOpt(this->socket_log_socket, IPPROTO_TCP, TCP_NODELAY, &sockOptValue,
                               sizeof(sockOptValue));
        nn::socket::SetSockOpt(this->socket_log_socket, SOL_SOCKET, SO_REUSEADDR, &sockOptValue,
                               sizeof(sockOptValue));
        nn::socket::SetSockOpt(this->socket_log_socket, SOL_SOCKET, SO_REUSEPORT, &sockOptValue,
                               sizeof(sockOptValue));

        if (nn::socket::Connect(this->socket_log_socket, (sockaddr*)&serverAddress, sizeof(serverAddress))
                .IsSuccess())
            break;

        // shutdown and close so we dont create millions of fds
        nn::socket::Shutdown(this->socket_log_socket, SHUT_RDWR);
        nn::socket::Close(this->socket_log_socket);

        // different error than the one above
        if (socketFails == 19) {
            hk::diag::logLine("Socket Connection Failed!");
            this->socket_errno = nn::socket::GetLastErrno();
            this->socket_log_state = SockState::CONNFAIL;

            mState = WAIT;
            return false;
        }

        nn::os::YieldThread();
        nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(500_ms));
    }

    this->socket_log_state = SockState::CONNECTED;

    hk::diag::logLine("Socket fd: %d", socket_log_socket);

    if (mSendThread->isDone())
        mSendThread->start();
    if (mRecvThread->isDone())
        mRecvThread->start();

    // The legacy C# connect payload is exactly 38 bytes (int, ushort,
    // fixed 32-byte name).  Queue it so the send worker remains the only
    // TCP writer after the connection has been established.
    u8 initFrame[LegacyProtocol::HeaderSize + 38] = {};
    const nn::account::Uid clientId = Client::getClientId();
    LegacyProtocol::encodeHeader(initFrame, reinterpret_cast<const LegacyProtocol::Byte*>(clientId.data),
                                 LegacyProtocol::Connect, 38);
    const s32 connectionType = mIsFirstConnect ? 0 : 1;
    LegacyProtocol::write(initFrame + LegacyProtocol::HeaderSize, 38, 0, connectionType);
    const u16 unknownMaxPlayers = USHRT_MAX;
    LegacyProtocol::write(initFrame + LegacyProtocol::HeaderSize, 38, 4, unknownMaxPlayers);
    const char* username = Client::getUsername().cstr();
    std::strncpy(reinterpret_cast<char*>(initFrame + LegacyProtocol::HeaderSize + 6), username, 31);
    mIsFirstConnect = false;
    if (!queueFrame(initFrame, sizeof(initFrame))) {
        socket_log_state = SockState::DISCONNECTED;
        mState = RESET;
        return false;
    }

    mState = WAIT;
    return true;
}

void SocketClient::exeReset() {
    closeSocket();

    nn::os::YieldThread();
    nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(100000000));

    // Free up blocked workers (discard one owned frame first if a queue is
    // full, preserving the matching allocation/deallocation path).
    const s64 discardedSend = mSendQueue.pop(sead::MessageQueue::BlockType::NonBlocking);
    if (discardedSend && discardedSend != StateWakeMessage)
        delete[] reinterpret_cast<u8*>(discardedSend);
    const s64 discardedRecv = mRecvQueue.pop(sead::MessageQueue::BlockType::NonBlocking);
    if (discardedRecv)
        delete[] reinterpret_cast<u8*>(discardedRecv);

    mSendQueue.push(0, sead::MessageQueue::BlockType::NonBlocking);
    mRecvQueue.push(0, sead::MessageQueue::BlockType::NonBlocking);

    while (!(mRecvThread->isDone() && mSendThread->isDone() && Client::isThreadDone())) {
        nn::os::YieldThread();
        nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(100000000));
    }

    clearFrameQueue(mSendQueue);
    clearFrameQueue(mRecvQueue);
    {
        sead::ScopedLock<sead::CriticalSection> lock(&mStateFrameLock);
        delete[] mLatestPlayerFrame;
        delete[] mLatestCapFrame;
        mLatestPlayerFrame = nullptr;
        mLatestCapFrame = nullptr;
        mStateWakeQueued = false;
    }

    if (mReconnectBackoffMs < 4000)
        mReconnectBackoffMs *= 2;
    mState = RECONNECT;
}

bool SocketClient::recv() {
    if (this->socket_log_state != SockState::CONNECTED) {
        hk::diag::logLine("Unable To Receive! Socket Not Connected.");
        this->socket_errno = nn::socket::GetLastErrno();
        return false;
    }

    u8 headerBytes[LegacyProtocol::HeaderSize] = {};
    if (!readExact(headerBytes, sizeof(headerBytes)))
        return false;

    LegacyProtocol::Header header{};
    if (!LegacyProtocol::decodeHeader(headerBytes, sizeof(headerBytes), &header)) {
        hk::diag::logLine("Invalid legacy packet header; reconnecting.");
        return false;
    }

    const s32 frameSize = LegacyProtocol::HeaderSize + header.payloadSize;
    u8* frame = new (gHeap) u8[frameSize];
    if (!frame)
        return false;
    std::memcpy(frame, headerBytes, sizeof(headerBytes));

    // Read the entire bounded payload even for server extensions that this
    // client does not use, so the next TCP frame stays aligned.
    if (!readExact(frame + LegacyProtocol::HeaderSize, header.payloadSize)) {
        delete[] frame;
        return false;
    }

    if (!mRecvQueue.push(reinterpret_cast<s64>(frame), sead::MessageQueue::BlockType::NonBlocking)) {
        delete[] frame;
        hk::diag::logLine("Receive queue full; dropping legacy frame type %d.", header.type);
    }
    return true;
}

void SocketClient::closeSocket() {
    hk::diag::logLine("Closing Socket.");

    this->socket_log_state = SockState::DISCONNECTED;

    for (s32 shutdownFails = 0; shutdownFails <= 20; shutdownFails++) {
        if (shutdownFails == 20)
            hk::diag::logLine("Failed to shutdown socket!");

        // Shutdown to unblock read func
        if (nn::socket::Shutdown(this->socket_log_socket, SHUT_RDWR) == 0)
            break;

        nn::os::YieldThread();
        nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(100_ms));
    }

    for (s32 closeFails = 0; closeFails <= 20; closeFails++) {
        if (closeFails == 20)
            hk::diag::logLine("Failed to close socket!");

        if (nn::socket::Close(this->socket_log_socket).IsSuccess())
            break;

        nn::os::YieldThread();
        nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(100_ms));
    }
}

bool SocketClient::stringToIPAddress(const char* str, in_addr* out) {
    // string to IPv4
    if (nn::socket::InetAton(str, out)) {
        return true;
    }

    // get IPs via DNS
    hostent* he = nn::socket::GetHostByName(str);
    if (!he) {
        return false;
    }

    // might give us multiple IP addresses, so pick the first one
    in_addr** addr_list = (in_addr**)he->h_addr_list;
    if (addr_list[0]) {
        *out = *addr_list[0];
        return true;
    }

    return false;
}

void SocketClient::sendFunc() {
    sead::ScopedCurrentHeapSetter setter(gHeap);

    hk::diag::logLine("Starting Send Thread.");

    while (socket_log_state != SockState::DISCONNECTED && trySendQueue()) {
    }

    this->socket_log_state = SockState::DISCONNECTED;

    hk::diag::logLine("Sending packet failed!");
    hk::diag::logLine("Ending Send Thread.");

    if (mState != RESET && mState != RECONNECT)
        mState = RESET;
}

void SocketClient::recvFunc() {
    sead::ScopedCurrentHeapSetter setter(gHeap);

    // ???
    nn::socket::Recv(this->socket_log_socket, nullptr, 0, 0);

    hk::diag::logLine("Starting Recv Thread.");

    while (socket_log_state != SockState::DISCONNECTED && recv()) {
    }

    this->socket_log_state = SockState::DISCONNECTED;

    hk::diag::logLine("Receiving Packet Failed!");
    hk::diag::logLine("Ending Recv Thread.");

    if (mState != RESET && mState != RECONNECT)
        mState = RESET;
}

bool SocketClient::queueFrame(const u8* frame, s32 frameSize) {
    if (!frame || frameSize < LegacyProtocol::HeaderSize || frameSize > LegacyProtocol::MaxFrameSize ||
        socket_log_state != SockState::CONNECTED)
        return false;

    LegacyProtocol::Header header{};
    if (!LegacyProtocol::decodeHeader(frame, frameSize, &header) ||
        frameSize != LegacyProtocol::HeaderSize + header.payloadSize)
        return false;

    u8* copy = new (gHeap) u8[frameSize];
    if (!copy)
        return false;
    std::memcpy(copy, frame, frameSize);

    if (header.type == LegacyProtocol::Player || header.type == LegacyProtocol::Cap)
        return queueStateFrame(copy, header.type);

    if (mSendQueue.push(reinterpret_cast<s64>(copy), sead::MessageQueue::BlockType::NonBlocking))
        return true;

    delete[] copy;
    hk::diag::logLine("Send queue full; frame type %d dropped.", header.type);
    return false;
}

bool SocketClient::trySendQueue() {
    const s64 message = mSendQueue.pop(sead::MessageQueue::BlockType::Blocking);
    if (message == StateWakeMessage)
        return sendLatestStateFrames();

    u8* frame = reinterpret_cast<u8*>(message);
    if (!frame)
        return false;

    LegacyProtocol::Header header{};
    const bool valid = LegacyProtocol::decodeHeader(frame, LegacyProtocol::HeaderSize, &header);
    const s32 frameSize = valid ? LegacyProtocol::HeaderSize + header.payloadSize : 0;
    const bool successful = valid && sendAll(frame, frameSize);

    delete[] frame;

    return successful;
}

u8* SocketClient::tryGetFrame() {
    if (socket_log_state != SockState::CONNECTED)
        return nullptr;
    return reinterpret_cast<u8*>(mRecvQueue.pop(sead::MessageQueue::BlockType::NonBlocking));
}

bool SocketClient::sendAll(const u8* buffer, s32 size) {
    if (!buffer || size <= 0 || socket_log_state != SockState::CONNECTED)
        return false;

    s32 sent = 0;
    while (sent < size) {
        const s32 result = nn::socket::Send(socket_log_socket, buffer + sent, size - sent, 0);
        socket_errno = nn::socket::GetLastErrno();
        if (result > 0) {
            sent += result;
            continue;
        }
        if (result < 0 && socket_errno == EINTR)
            continue;
        hk::diag::logLine("Legacy TCP send failed after %d/%d bytes (errno %d).", sent, size, socket_errno);
        return false;
    }
    return true;
}

bool SocketClient::readExact(u8* buffer, s32 size) {
    s32 received = 0;
    while (received < size) {
        const s32 result = nn::socket::Recv(socket_log_socket, buffer + received, size - received, sock_flags);
        socket_errno = nn::socket::GetLastErrno();
        if (result > 0) {
            received += result;
            continue;
        }
        if (result < 0 && socket_errno == EINTR)
            continue;
        hk::diag::logLine("Legacy TCP read failed after %d/%d bytes (errno %d).", received, size, socket_errno);
        return false;
    }
    return true;
}

void SocketClient::clearFrameQueue(sead::MessageQueue& queue) {
    for (;;) {
        const s64 message = queue.pop(sead::MessageQueue::BlockType::NonBlocking);
        if (!message)
            return;
        if (message != StateWakeMessage)
            delete[] reinterpret_cast<u8*>(message);
    }
}

bool SocketClient::queueStateFrame(u8* frame, s16 type) {
    bool queueWake = false;
    {
        sead::ScopedLock<sead::CriticalSection> lock(&mStateFrameLock);
        u8** slot = type == LegacyProtocol::Player ? &mLatestPlayerFrame : &mLatestCapFrame;
        delete[] *slot;
        *slot = frame;
        if (!mStateWakeQueued) {
            mStateWakeQueued = true;
            queueWake = true;
        }
    }

    if (!queueWake)
        return true;
    if (mSendQueue.push(StateWakeMessage, sead::MessageQueue::BlockType::NonBlocking))
        return true;

    // A saturated reliable queue is allowed to drop stale state.  Clear the
    // wake flag so a later state update can request service again.
    sead::ScopedLock<sead::CriticalSection> lock(&mStateFrameLock);
    mStateWakeQueued = false;
    delete[] mLatestPlayerFrame;
    delete[] mLatestCapFrame;
    mLatestPlayerFrame = nullptr;
    mLatestCapFrame = nullptr;
    hk::diag::logLine("Reliable send queue full; dropping coalesced state.");
    return false;
}

bool SocketClient::sendLatestStateFrames() {
    u8* player = nullptr;
    u8* cap = nullptr;
    {
        sead::ScopedLock<sead::CriticalSection> lock(&mStateFrameLock);
        mStateWakeQueued = false;
        player = mLatestPlayerFrame;
        cap = mLatestCapFrame;
        mLatestPlayerFrame = nullptr;
        mLatestCapFrame = nullptr;
    }

    const auto sendStateFrame = [this](u8* frame) {
        if (!frame)
            return true;
        LegacyProtocol::Header header{};
        const bool valid = LegacyProtocol::decodeHeader(frame, LegacyProtocol::HeaderSize, &header);
        const bool sent = valid && sendAll(frame, LegacyProtocol::HeaderSize + header.payloadSize);
        delete[] frame;
        return sent;
    };

    if (!sendStateFrame(player)) {
        delete[] cap;
        return false;
    }
    return sendStateFrame(cap);
}
