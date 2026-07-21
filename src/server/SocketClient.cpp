#include "server/SocketClient.hpp"

#include "hk/diag/diag.h"

#include "nn/os.h"
#include "nn/socket.h"
#include "vapours/results/results_common.hpp"

#include "sead/heap/seadHeapMgr.h"

#include "al/Library/Thread/AsyncFunctorThread.h"
#include "al/Library/Thread/FunctorV0M.h"

#include <cerrno>
#include <experimental/memory>
#include <experimental/utility>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "main.hpp"
#include "packets/Packet.h"
#include "packets/PacketFactory.h"
#include "packets/PlayerConnect.h"
#include "server/Client.hpp"
#include "SocketBase.hpp"

using namespace nn;

SocketClient::SocketClient() : SocketBase("SocketClient") {
    sead::ScopedCurrentHeapSetter setter(gHeap);

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
            Client::instance()->startThread();
            break;
        }
        os::YieldThread();
        os::SleepThread(TimeSpan::FromMilliSeconds(100));
    }
}

void SocketClient::init(const char* ip, u16 port) {
    mSockIp = ip;
    mPort = port;

    if (mSocketThread->isDone())
        mSocketThread->start();

    mState = INIT;
}

bool SocketClient::exeInit() {
    hk::diag::logLine("socket client init");

    in_addr hostAddress = {0};
    sockaddr_in serverAddress = {0};

    hk::diag::logLine("SocketClient::exeInit: %s:%d sock %s", getIP(), mPort, getStateChar());

    if (!stringToIPAddress(getIP(), &serverAddress.sin_addr)) {
        hk::diag::logLine("IP address is invalid or hostname not resolveable.");
        mSockErrno = socket::GetLastErrno();
        mSockState = SockState::INVALIP;

        mState = WAIT;
        return false;
    }

    serverAddress.sin_port = socket::InetHtons(mPort);
    serverAddress.sin_family = socket::InetHtons(AF_INET);

    for (s32 fails = 0; fails <= 20; fails++) {
        if (fails == 20) {
            hk::diag::logLine("Socket Unavailable.");
            mSockErrno = socket::GetLastErrno();
            mSockState = SockState::UNAVAILABLE;

            mState = WAIT;
            return false;
        }

        if ((mSockFd = socket::Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
            hk::diag::logLine("Failed to create Socket");
            os::YieldThread();
            os::SleepThread(TimeSpan::FromMilliSeconds(500));
            continue;
        }

        s32 optValue = 1;

        socket::SetSockOpt(mSockFd, SOL_SOCKET, TCP_NODELAY, &optValue, sizeof(optValue));
        socket::SetSockOpt(mSockFd, SOL_SOCKET, SO_REUSEADDR, &optValue, sizeof(optValue));
        socket::SetSockOpt(mSockFd, SOL_SOCKET, SO_REUSEPORT, &optValue, sizeof(optValue));

        if (socket::Connect(mSockFd, (sockaddr*)&serverAddress, sizeof(serverAddress)).IsSuccess())
            break;

        hk::diag::logLine("Failed to connect to server");

        // shutdown and close so we dont create millions of fds
        socket::Shutdown(mSockFd, SHUT_RDWR);
        socket::Close(mSockFd);

        // different error than the one above
        if (fails == 19) {
            hk::diag::logLine("Socket Connection Failed!");
            mSockErrno = socket::GetLastErrno();
            mSockState = SockState::CONNFAIL;

            mState = WAIT;
            return false;
        }

        os::YieldThread();
        os::SleepThread(TimeSpan::FromMilliSeconds(500));
    }

    mSockState = SockState::CONNECTED;

    hk::diag::logLine("Socket fd: %d", mSockFd);

    auto initPacket = std::unique_ptr<PlayerConnect>{new (gHeap) PlayerConnect};

    initPacket->mUserID = Client::getClientId();
    initPacket->clientName = Client::getUsername();

    initPacket->conType = mIsFirstConnect ? ConnectionTypes::INIT : ConnectionTypes::RECONNECT;
    mIsFirstConnect = false;

    if (send(std::move(initPacket))) {
        mState = WAIT;
        startThreads();
        return true;
    }

    return false;
}

void SocketClient::exeReset() {
    closeSocket();

    os::YieldThread();
    os::SleepThread(TimeSpan::FromMilliSeconds(100));

    // Free up all blocked threads
    mSendQueue.push(0, sead::MessageQueue::BlockType::NonBlocking);
    mRecvQueue.push(0, sead::MessageQueue::BlockType::NonBlocking);

    while (!(mRecvThread->isDone() && mSendThread->isDone() && Client::isThreadDone())) {
        os::YieldThread();
        os::SleepThread(TimeSpan::FromMilliSeconds(100));
    }

    // clear send and recv queue (idk man)
    for (s32 i = 0; i < 100; i++) {
        delete reinterpret_cast<Packet*>(mSendQueue.pop(sead::MessageQueue::BlockType::NonBlocking));
        delete reinterpret_cast<Packet*>(mRecvQueue.pop(sead::MessageQueue::BlockType::NonBlocking));
    }

    mState = RECONNECT;
}

void SocketClient::startThreads() {
    if (mRecvThread->isDone())
        mRecvThread->start();
    if (mSendThread->isDone())
        mSendThread->start();
}

bool SocketClient::send(std::unique_ptr<Packet> packet) {
    if (mSockState != SockState::CONNECTED || packet == nullptr) {
        hk::diag::logLine("Unable To Send! Socket Not Connected.");
        mSockErrno = socket::GetLastErrno();
        return false;
    }

    PacketVector packetData = packet->serialize();
    s32 valsent = 0;

    if (packet->mType != PLAYERINF && packet->mType != HACKCAPINF)
        hk::diag::logLine("Sending packet: %s", packetNames[packet->mType]);

    while (valsent < packetData.size()) {
        s32 result = socket::Send(mSockFd, packetData.data(), packetData.size() - valsent, 0);

        mSockErrno = socket::GetLastErrno();

        if (result <= 0) {
            hk::diag::logLine(
                "Packet send failed! Packet type is %hd. Sent %d this iteration, %d so far, out of %hd.",
                packet->mType, result, valsent, packet->mPacketSize);
            return false;
        }

        valsent += result;
    }

    return true;
}

bool SocketClient::recv() {
    if (mSockState != SockState::CONNECTED) {
        hk::diag::logLine("Unable To Receive! Socket Not Connected.");
        mSockErrno = socket::GetLastErrno();
        return false;
    }

    u8 headerBuf[sHeaderSize];
    s32 valread = 0;

    // read only the size of a header
    while (valread < sHeaderSize) {
        s32 result = socket::Recv(mSockFd, headerBuf + valread, sHeaderSize - valread, mSockFlags);

        mSockErrno = socket::GetLastErrno();

        if (result <= 0) {
            if (mSockErrno == EAGAIN) {
                return true;
            } else {
                hk::diag::logLine("Header Read Failed! Value: %d Total Read: %d", result, valread);
                return false;
            }
        }

        valread += result;
    }

    PacketVector packetData;
    packetData.insert(packetData.end(), headerBuf, headerBuf + sHeaderSize);

    PacketHeader header;
    header.deserialize(packetData);

    if (header.mIsFail) {
        hk::diag::logLine("The header failed to deserialize properly.");
        return false;
    }

    if (header.mType != PLAYERINF && header.mType != HACKCAPINF) {
        hk::diag::logLine("Received packet (from %lu%lu): Size: %hd Type: %hd%s%s",
                          header.mUserID.m_Storage[0], header.mUserID.m_Storage[1], header.mPacketSize,
                          header.mType, packetNames[header.mType] ? " Type String: " : "",
                          packetNames[header.mType] ? packetNames[header.mType] : "");
    }

    u8 packetBuf[header.mPacketSize];

    valread = 0;

    while (valread < header.mPacketSize) {
        s32 result = socket::Recv(mSockFd, packetBuf + valread, header.mPacketSize - valread, mSockFlags);

        mSockErrno = socket::GetLastErrno();

        if (result <= 0) {
            hk::diag::logLine("Packet Read Failed! Value: %d Packet Size: %d Packet Type: %s", result,
                              header.mPacketSize, packetNames[header.mType]);
            return false;
        }

        valread += result;
    }

    // taginf is unused in SR so just ignore
    if (header.mType == TAGINF)
        return true;

    packetData.insert(packetData.end(), packetBuf, packetBuf + header.mPacketSize);

    auto packet = PacketFactory::create(header.mType);

    if (packet)
        packet->deserialize(packetData);
    else {
        hk::diag::logLine("Factory couldn't produce packet of type %d", header.mType);
        return false;
    }

    if (packet->mIsFail) {
        hk::diag::logLine("The packet failed to deserialize properly.");
        return false;
    }

    s64 ptr = reinterpret_cast<s64>(packet.release());
    mRecvQueue.push(ptr, sead::MessageQueue::BlockType::NonBlocking);

    return true;
}

void SocketClient::closeSocket() {
    hk::diag::logLine("Closing Socket.");

    mSockState = SockState::DISCONNECTED;
    s32 fails = 0;
    for (fails = 0; fails <= 20; fails++) {
        if (fails == 20)
            hk::diag::logLine("Failed to shutdown socket!");

        // Shutdown to unblock read func
        if (socket::Shutdown(mSockFd, SHUT_RDWR) == 0)
            break;

        os::YieldThread();
        os::SleepThread(TimeSpan::FromMilliSeconds(100));
    }

    for (fails = 0; fails <= 20; fails++) {
        if (fails == 20)
            hk::diag::logLine("Failed to close socket!");

        if (socket::Close(mSockFd).IsSuccess())
            break;

        os::YieldThread();
        os::SleepThread(TimeSpan::FromMilliSeconds(100));
    }
}

bool SocketClient::stringToIPAddress(const char* str, in_addr* out) {
    // string to IPv4
    if (socket::InetAton(str, out)) {
        return true;
    }

    // get IPs via DNS
    hostent* he = socket::GetHostByName(str);
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

    while (mSockState != SockState::DISCONNECTED && trySendQueue()) {
    }

    mSockState = SockState::DISCONNECTED;

    hk::diag::logLine("Sending packet failed!");
    hk::diag::logLine("Ending Send Thread.");

    signalReset();
}

void SocketClient::recvFunc() {
    sead::ScopedCurrentHeapSetter setter(gHeap);

    // ???
    socket::Recv(mSockFd, nullptr, 0, 0);

    hk::diag::logLine("Starting Recv Thread.");

    while (mSockState != SockState::DISCONNECTED && recv()) {
    }

    mSockState = SockState::DISCONNECTED;

    hk::diag::logLine("Receiving Packet Failed!");
    hk::diag::logLine("Ending Recv Thread.");

    signalReset();
}

bool SocketClient::queuePacket(std::unique_ptr<Packet> packet) {
    if (mSockState == SockState::CONNECTED) {
        s64 ptr = reinterpret_cast<s64>(packet.release());
        mSendQueue.push(ptr, sead::MessageQueue::BlockType::NonBlocking);

        return true;
    }

    return false;
}

bool SocketClient::trySendQueue() {
    Packet* ptr = reinterpret_cast<Packet*>(mSendQueue.pop(sead::MessageQueue::BlockType::Blocking));
    if (!ptr) {
        hk::diag::logLine("SocketClient::trySendQueue: packet was nullptr");
        return false;
    }

    std::unique_ptr<Packet> packet(ptr);
    return send(std::move(packet));
}

std::unique_ptr<Packet> SocketClient::tryGetPacket() {
    if (mSockState != SockState::CONNECTED)
        return nullptr;

    return std::unique_ptr<Packet>{
        reinterpret_cast<Packet*>(mRecvQueue.pop(sead::MessageQueue::BlockType::Blocking))};
}
