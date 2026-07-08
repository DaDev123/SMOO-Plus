#include "server/SocketClient.hpp"

#include "hk/diag/diag.h"

#include "nn/nifm.h"
#include "nn/os.h"
#include "nn/socket.h"
#include "vapours/results/results_common.hpp"

#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "heap/seadHeapMgr.h"
#include "Library/Thread/AsyncFunctorThread.h"
#include "Library/Thread/FunctorV0M.h"
#include "logger.hpp"
#include "main.hpp"
#include "packets/Packet.h"
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
            if (!exeInit())
                mState = RECONNECT;
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
}

bool SocketClient::exeInit() {
    hk::diag::logLine("socket client init");

// emulators (ryujinx) make this return false always, so skip it during init
#ifndef EMU
    if (!nn::nifm::IsNetworkAvailable()) {
        this->socket_log_state = SockState::NONET;
        this->socket_errno = nn::socket::GetLastErrno();

        mState = WAIT;
        return false;
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

    if ((this->socket_log_socket = nn::socket::Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        hk::diag::logLine("Socket Unavailable.");
        this->socket_errno = nn::socket::GetLastErrno();
        this->socket_log_state = SockState::UNAVAILABLE;

        mState = WAIT;
        return false;
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

    nn::Result result;

    if ((result =
             nn::socket::Connect(this->socket_log_socket, (sockaddr*)&serverAddress, sizeof(serverAddress)))
            .IsFailure()) {
        hk::diag::logLine("Socket Connection Failed!");
        this->socket_errno = nn::socket::GetLastErrno();
        this->socket_log_state = SockState::CONNFAIL;

        nn::socket::Close(this->socket_log_socket);
        mState = WAIT;
        return false;
    }

    this->socket_log_state = SockState::CONNECTED;

    hk::diag::logLine("Socket fd: %d", socket_log_socket);

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

    nn::os::YieldThread();
    nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(100000000));

    // Free up all blocked threads
    mSendQueue.push(0, sead::MessageQueue::BlockType::NonBlocking);
    mRecvQueue.push(0, sead::MessageQueue::BlockType::NonBlocking);

    while (!(mRecvThread->isDone() && mSendThread->isDone())) {
        nn::os::YieldThread();
        nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(100000000));
    }

    // clear send and recv queue (idk man)
    for (s32 i = 0; i < 100; i++) {
        mSendQueue.pop(sead::MessageQueue::BlockType::NonBlocking);
        mRecvQueue.pop(sead::MessageQueue::BlockType::NonBlocking);
    }

    mState = RECONNECT;
}

bool SocketClient::send(Packet* packet) {
    if (this->socket_log_state != SockState::CONNECTED || packet == nullptr)
        return false;

    char* buffer = reinterpret_cast<char*>(packet);

    int valread = 0;

    if (packet->mType != PLAYERINF && packet->mType != HACKCAPINF)
        hk::diag::logLine("Sending packet: %s", packetNames[packet->mType]);

    valread = nn::socket::Send(this->socket_log_socket, buffer, packet->mPacketSize + sizeof(Packet), 0);

    if (valread <= 0) {
        hk::diag::logLine("Failed to Fully Send Packet! Result: %d Type: %s Packet Size: %d", valread,
                          packetNames[packet->mType], packet->mPacketSize);
        this->socket_errno = nn::socket::GetLastErrno();
        return false;
    }

    return true;
}

bool SocketClient::recv() {
    if (this->socket_log_state != SockState::CONNECTED) {
        hk::diag::logLine("Unable To Receive! Socket Not Connected.");
        this->socket_errno = nn::socket::GetLastErrno();
        return false;
    }

    int headerSize = sizeof(Packet);
    char headerBuf[sizeof(Packet)] = {};
    int valread = 0;

    // read only the size of a header
    while (valread < headerSize) {
        int result = nn::socket::Recv(this->socket_log_socket, headerBuf + valread, headerSize - valread,
                                      this->sock_flags);

        this->socket_errno = nn::socket::GetLastErrno();

        if (result > 0) {
            valread += result;
        } else {
            if (this->socket_errno == 11) {
                return true;
            } else {
                hk::diag::logLine("Header Read Failed! Value: %d Total Read: %d", result, valread);
                return false;
            }
        }
    }

    if (valread > 0) {
        Packet* header = reinterpret_cast<Packet*>(headerBuf);

        int fullSize = header->mPacketSize + sizeof(Packet);

        if (header->mType > PacketType::UNKNOWN && header->mType < PacketType::End &&
            fullSize <= MAXPACKSIZE && fullSize > 0 && valread == sizeof(Packet)) {
            if (header->mType != PLAYERINF && header->mType != HACKCAPINF) {
                hk::diag::log("Received packet (from %02X%02X):", header->mUserID.data[0],
                              header->mUserID.data[1]);
                Logger::disableName();
                hk::diag::log(" Size: %d", header->mPacketSize);
                hk::diag::log(" Type: %d", header->mType);
                if (packetNames[header->mType])
                    hk::diag::logLine(" Type String: %s", packetNames[header->mType]);
                Logger::enableName();
            }

            char* packetBuf = (char*)malloc(fullSize);

            if (packetBuf) {
                memcpy(packetBuf, headerBuf, sizeof(Packet));

                while (valread < fullSize) {
                    int result = nn::socket::Recv(this->socket_log_socket, packetBuf + valread,
                                                  fullSize - valread, this->sock_flags);

                    this->socket_errno = nn::socket::GetLastErrno();

                    if (result > 0) {
                        valread += result;
                    } else {
                        free(packetBuf);
                        hk::diag::logLine("Packet Read Failed! Value: %d\nPacket Size: %d\nPacket Type: %s",
                                          result, header->mPacketSize, packetNames[header->mType]);
                        return false;
                    }
                }

                Packet* packet = reinterpret_cast<Packet*>(packetBuf);

                if (!mRecvQueue.push((s64)packet, sead::MessageQueue::BlockType::NonBlocking))
                    free(packetBuf);
            }
        } else {
            hk::diag::logLine(
                "Failed to aquire valid data! Packet Type: %d Full Packet Size %d valread size: %d",
                header->mType, fullSize, valread);
        }

        return true;
    } else {  // if we error'd, close the socket
        hk::diag::logLine("valread was zero! Disconnecting.");
        this->socket_errno = nn::socket::GetLastErrno();
        return false;
    }
}

// prints packet to debug logger
void SocketClient::printPacket(Packet* packet) {
    packet->mUserID.print();
    hk::diag::logLine("Type: %s", packetNames[packet->mType]);

    switch (packet->mType) {
    case PacketType::PLAYERINF:
        hk::diag::logLine("Pos X: %f Pos Y: %f Pos Z: %f", ((PlayerInf*)packet)->playerPos.x,
                          ((PlayerInf*)packet)->playerPos.y, ((PlayerInf*)packet)->playerPos.z);
        hk::diag::logLine("Rot X: %f Rot Y: %f Rot Z: %f\nRot W: %f", ((PlayerInf*)packet)->playerRot.x,
                          ((PlayerInf*)packet)->playerRot.y, ((PlayerInf*)packet)->playerRot.z,
                          ((PlayerInf*)packet)->playerRot.w);
        break;
    default:
        break;
    }
}

bool SocketClient::closeSocket() {
    hk::diag::logLine("Closing Socket.");

    nn::Result result = nn::socket::Close(this->socket_log_socket);

    while (result.IsFailure()) {
            hk::diag::logLine("Failed to close socket!");
            nn::os::YieldThread();
            nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(100000000));

        result = nn::socket::Close(this->socket_log_socket);
    }

    this->socket_log_state = SockState::DISCONNECTED;
    return true;
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

    while (trySendQueue() && socket_log_state != SockState::DISCONNECTED) {
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

    while (recv() && socket_log_state != SockState::DISCONNECTED) {
    }

    this->socket_log_state = SockState::DISCONNECTED;

    hk::diag::logLine("Receiving Packet Failed!");
    hk::diag::logLine("Ending Recv Thread.");

    if (mState != RESET && mState != RECONNECT)
        mState = RESET;
}

bool SocketClient::queuePacket(Packet* packet) {
    if (socket_log_state == SockState::CONNECTED)
        if (mSendQueue.push((s64)packet, sead::MessageQueue::BlockType::NonBlocking))
            return true;

    delete packet;
    return false;
}

bool SocketClient::trySendQueue() {
    Packet* curPacket = (Packet*)mSendQueue.pop(sead::MessageQueue::BlockType::Blocking);

    bool successful = send(curPacket);

    delete curPacket;

    return successful;
}

Packet* SocketClient::tryGetPacket() {
    return socket_log_state == SockState::CONNECTED ?
               (Packet*)mRecvQueue.pop(sead::MessageQueue::BlockType::Blocking) :
               nullptr;
}