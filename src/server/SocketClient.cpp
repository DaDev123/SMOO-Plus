#include "server/SocketClient.hpp"

#include "hk/diag/diag.h"

#include "nn/nifm.h"
#include "nn/os.h"
#include "nn/socket.h"
#include "vapours/results/results_common.hpp"

#include "sead/heap/seadHeapMgr.h"

#include "al/Library/Thread/AsyncFunctorThread.h"
#include "al/Library/Thread/FunctorV0M.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "main.hpp"
#include "packets/Packet.h"
#include "server/Client.hpp"

using namespace nn;

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
            Client::instance()->startThread();
            break;
        }
        os::YieldThread();
        os::SleepThread(TimeSpan::FromNanoSeconds(100_ms));
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

    // emulators (ryujinx) make this return false always, so skip it during init
    s32 fails;
#ifndef EMU
    // TODO: this has been causing issues so maybe reomove
    for (fails = 0; fails <= 20; fails++) {
        if (fails == 20) {
            mSockState = SockState::NONET;
            mSockErrno = socket::GetLastErrno();

            mState = WAIT;
            return false;
        }

        if (nifm::IsNetworkAvailable())
            break;

        os::YieldThread();
        os::SleepThread(TimeSpan::FromNanoSeconds(500_ms));
    }
#endif
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

    for (fails = 0; fails <= 20; fails++) {
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
            os::SleepThread(TimeSpan::FromNanoSeconds(500_ms));
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
        os::SleepThread(TimeSpan::FromNanoSeconds(500_ms));
    }

    mSockState = SockState::CONNECTED;

    hk::diag::logLine("Socket fd: %d", mSockFd);

    if (mRecvThread->isDone())
        mRecvThread->start();
    if (mSendThread->isDone())
        mSendThread->start();

    PlayerConnect initPacket;

    initPacket.mUserID = Client::getClientId();
    strcpy(initPacket.clientName, Client::getUsername().cstr());

    initPacket.conType = mIsFirstConnect ? ConnectionTypes::INIT : ConnectionTypes::RECONNECT;
    mIsFirstConnect = false;

    send(&initPacket);

    mState = WAIT;
    return true;
}

void SocketClient::exeReset() {
    closeSocket();

    os::YieldThread();
    os::SleepThread(TimeSpan::FromNanoSeconds(100_ms));

    // Free up all blocked threads (pop first in case its full somehow)
    mSendQueue.pop(sead::MessageQueue::BlockType::NonBlocking);
    mRecvQueue.pop(sead::MessageQueue::BlockType::NonBlocking);

    mSendQueue.push(0, sead::MessageQueue::BlockType::NonBlocking);
    mRecvQueue.push(0, sead::MessageQueue::BlockType::NonBlocking);

    while (!(mRecvThread->isDone() && mSendThread->isDone() && Client::isThreadDone())) {
        os::YieldThread();
        os::SleepThread(TimeSpan::FromNanoSeconds(100_ms));
    }

    // clear send and recv queue (idk man)
    for (s32 i = 0; i < 100; i++) {
        mSendQueue.pop(sead::MessageQueue::BlockType::NonBlocking);
        mRecvQueue.pop(sead::MessageQueue::BlockType::NonBlocking);
    }

    mState = RECONNECT;
}

bool SocketClient::send(Packet* packet) {
    if (mSockState != SockState::CONNECTED || packet == nullptr)
        return false;

    if (!(packet->mType > PacketType::UNKNOWN && packet->mType < PacketType::End))
        return false;

    u8* buffer = reinterpret_cast<u8*>(packet);

    int valread = 0;

    if (packet->mType != PLAYERINF && packet->mType != HACKCAPINF)
        hk::diag::logLine("Sending packet: %s", packetNames[packet->mType]);

    valread = socket::Send(mSockFd, buffer, packet->mPacketSize + sizeof(Packet), 0);

    if (valread <= 0) {
        hk::diag::logLine("Failed to Fully Send Packet! Result: %d Type: %s Packet Size: %d", valread,
                          packetNames[packet->mType], packet->mPacketSize);
        mSockErrno = socket::GetLastErrno();
        return false;
    }

    return true;
}

bool SocketClient::recv() {
    if (mSockState != SockState::CONNECTED) {
        hk::diag::logLine("Unable To Receive! Socket Not Connected.");
        mSockErrno = socket::GetLastErrno();
        return false;
    }

    int headerSize = sizeof(Packet);
    Packet header;
    u8* headerBuf = reinterpret_cast<u8*>(&header);
    int valread = 0;

    // just for sanity
    memset(headerBuf, 0, sizeof(Packet));

    // read only the size of a header
    while (valread < headerSize) {
        int result = socket::Recv(mSockFd, headerBuf + valread, headerSize - valread, mSockFlags);

        mSockErrno = socket::GetLastErrno();

        if (result > 0) {
            valread += result;
        } else {
            if (mSockErrno == EAGAIN) {
                return true;
            } else {
                hk::diag::logLine("Header Read Failed! Value: %d Total Read: %d", result, valread);
                return false;
            }
        }
    }

    if (valread > 0) {
        int fullSize = header.mPacketSize + sizeof(Packet);

        if (header.mType > PacketType::UNKNOWN && header.mType < PacketType::End && fullSize <= MAXPACKSIZE &&
            fullSize > 0 && valread == sizeof(Packet)) {
            if (header.mType != PLAYERINF && header.mType != HACKCAPINF) {
                char msg[0x50] = "";
                int len = sprintf(msg, "Received packet (from %02X%02X):", header.mUserID.data[0],
                                  header.mUserID.data[1]);
                len += sprintf(msg + len, " Size: %d", header.mPacketSize);
                len += sprintf(msg + len, " Type: %d", header.mType);

                if (packetNames[header.mType])
                    len += sprintf(msg + len, " Type String: %s", packetNames[header.mType]);
                msg[len] = '\0';
                hk::diag::logLine("%s", msg);
            }

            // char* packetBuf = (char*)gHeap->alloc(fullSize);
            u8* packetBuf = new (gHeap) u8[fullSize];
            if (packetBuf) {
                memcpy(packetBuf, headerBuf, sizeof(Packet));
                while (valread < fullSize) {
                    int result =
                        nn::socket::Recv(mSockFd, packetBuf + valread, fullSize - valread, mSockFlags);

                    mSockErrno = socket::GetLastErrno();

                    if (result > 0) {
                        valread += result;
                    } else {
                        // gHeap->free(packetBuf);
                        delete[] packetBuf;
                        hk::diag::logLine("Packet Read Failed! Value: %d\nPacket Size: %d\nPacket Type: %s",
                                          result, header.mPacketSize, packetNames[header.mType]);
                        return false;
                    }
                }

                Packet* packet = reinterpret_cast<Packet*>(packetBuf);

                if (!mRecvQueue.push((uintptr_t)packet, sead::MessageQueue::BlockType::NonBlocking))
                    // gHeap->free(packetBuf);
                    delete[] packetBuf;
            }
        } else {
            hk::diag::logLine(
                "Failed to aquire valid data! Packet Type: %d Full Packet Size %d valread size: %d",
                header.mType, fullSize, valread);
        }

        return true;
    } else {  // if we error'd, close the socket
        hk::diag::logLine("valread was zero! Disconnecting.");
        mSockErrno = socket::GetLastErrno();
        return false;
    }
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
        os::SleepThread(TimeSpan::FromNanoSeconds(100_ms));
    }

    for (fails = 0; fails <= 20; fails++) {
        if (fails == 20)
            hk::diag::logLine("Failed to close socket!");

        if (socket::Close(mSockFd).IsSuccess())
            break;

        os::YieldThread();
        os::SleepThread(TimeSpan::FromNanoSeconds(100_ms));
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

    if (mState != RESET && mState != RECONNECT)
        mState = RESET;
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

    if (mState != RESET && mState != RECONNECT)
        mState = RESET;
}

void SocketClient::deletePacketAfterSend(Packet* packet) {
    if (!packet)
        return;

    switch (packet->mType) {
    case PacketType::PLAYERINF:
        delete static_cast<PlayerInf*>(packet);
        break;
    case PacketType::HACKCAPINF:
        delete static_cast<HackCapInf*>(packet);
        break;
    case PacketType::GAMEINF:
        delete static_cast<GameInf*>(packet);
        break;
    case PacketType::PLAYERCON:
        delete static_cast<PlayerConnect*>(packet);
        break;
    case PacketType::PLAYERDC:
        delete static_cast<PlayerDC*>(packet);
        break;
    case PacketType::COSTUMEINF:
        delete static_cast<CostumeInf*>(packet);
        break;
    case PacketType::SHINECOLL:
        delete static_cast<ShineCollect*>(packet);
        break;
    case PacketType::CAPTUREINF:
        delete static_cast<CaptureInf*>(packet);
        break;
    case PacketType::COINCOLLECTCOLL:
        delete static_cast<CoinCollectCollect*>(packet);
        break;
    case PacketType::CHECKPOINTGET:
        delete static_cast<CheckpointGet*>(packet);
        break;
    case PacketType::MOONROCKHIT:
        delete static_cast<MoonRockHit*>(packet);
        break;
    case PacketType::GAMESTART:
        delete packet;
        break;
    default:
        hk::diag::logLine("WARNING: Attempted to delete invalid packet type: %d!", packet->mType);
        break;
    }
}

bool SocketClient::queuePacket(Packet* packet) {
    if (mSockState == SockState::CONNECTED)
        if (mSendQueue.push((uintptr_t)packet, sead::MessageQueue::BlockType::NonBlocking))
            return true;

    deletePacketAfterSend(packet);
    return false;
}

bool SocketClient::trySendQueue() {
    Packet* curPacket = (Packet*)mSendQueue.pop(sead::MessageQueue::BlockType::Blocking);

    bool successful = send(curPacket);

    deletePacketAfterSend(curPacket);

    return successful;
}

Packet* SocketClient::tryGetPacket() {
    return mSockState == SockState::CONNECTED ?
               (Packet*)mRecvQueue.pop(sead::MessageQueue::BlockType::Blocking) :
               nullptr;
}