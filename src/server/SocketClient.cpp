#include "server/SocketClient.hpp"

#include "nn/nifm.h"
#include "nn/os.h"
#include "nn/socket.h"
#include "vapours/results/results_common.hpp"

#include "sead/heap/seadHakkunHeap.h"

#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

#include "Library/Thread/AsyncFunctorThread.h"
#include "Library/Thread/FunctorV0M.h"
#include "logger.hpp"
#include "packets/Packet.h"
#include "server/Client.hpp"
#include "syssocket/sockdefines.h"
#include "types.h"

SocketClient::SocketClient() : SocketBase("SocketClient") {
    mRecvQueue.allocate(100, sead::HakkunHeap::sInstance);
    mSendQueue.allocate(100, sead::HakkunHeap::sInstance);

    mSocketThread = new al::AsyncFunctorThread("SocketMainThread",
                                               al::FunctorV0M(this, &SocketClient::update), 0, 16_KB, {0});
    mRecvThread = new al::AsyncFunctorThread("SocketRecvThread",
                                             al::FunctorV0M(this, &SocketClient::recvFunc), 0, 16_KB, {0});
    mSendThread = new al::AsyncFunctorThread("SocketSendThread",
                                             al::FunctorV0M(this, &SocketClient::sendFunc), 0, 16_KB, {0});
}

void SocketClient::update() {
    // didnt use al nerves to be safe but idk maybe that couldve worked too
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
    Logger::log("socket client init\n");

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

    Logger::log("SocketClient::exeInit: %s:%d sock %s\n", getIP(), this->port, getStateChar());

    if (!this->stringToIPAddress(this->sock_ip.cstr(), &hostAddress)) {
        Logger::log("IP address is invalid or hostname not resolveable.\n");
        this->socket_errno = nn::socket::GetLastErrno();
        this->socket_log_state = SockState::INVALIP;

        mState = WAIT;
        return false;
    }

    if ((this->socket_log_socket = nn::socket::Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        Logger::log("Socket Unavailable.\n");
        this->socket_errno = nn::socket::GetLastErrno();
        this->socket_log_state = SockState::UNAVAILABLE;

        mState = WAIT;
        return false;
    }

    serverAddress.sin_addr = hostAddress;
    serverAddress.sin_port = nn::socket::InetHtons(this->port);
    serverAddress.sin_family = nn::socket::InetHtons(AF_INET);

    s32 sockOptValue = 1;
    nn::socket::SetSockOpt(this->socket_log_socket, 6, TCP_NODELAY, &sockOptValue, sizeof(sockOptValue));
    nn::socket::SetSockOpt(this->socket_log_socket, SOL_SOCKET, SO_REUSEADDR, &sockOptValue,
                           sizeof(sockOptValue));
    nn::socket::SetSockOpt(this->socket_log_socket, SOL_SOCKET, SO_REUSEPORT, &sockOptValue,
                           sizeof(sockOptValue));

    nn::Result result;

    if ((result =
             nn::socket::Connect(this->socket_log_socket, (sockaddr*)&serverAddress, sizeof(serverAddress)))
            .IsFailure()) {
        Logger::log("Socket Connection Failed!\n");
        this->socket_errno = nn::socket::GetLastErrno();
        this->socket_log_state = SockState::CONNFAIL;

        nn::socket::Close(this->socket_log_socket);
        mState = WAIT;
        return false;
    }

    this->socket_log_state = SockState::CONNECTED;

    Logger::log("Socket fd: %d\n", socket_log_socket);

    // startThreads();  // start recv and send threads after sucessful connection

    // send init packet to server once we connect (an issue with the server prevents this from
    // working properly, waiting for a fix to implement)

    PlayerConnect initPacket;

    initPacket.mUserID = Client::getClientId();
    strcpy(initPacket.clientName, Client::getUsername().cstr());

    initPacket.conType = mIsFirstConnect ? ConnectionTypes::INIT : ConnectionTypes::RECONNECT;
    mIsFirstConnect = false;

    send(&initPacket);

    if (mRecvThread->isDone())
        mRecvThread->start();
    if (mSendThread->isDone())
        mSendThread->start();

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

    mState = RECONNECT;
}

bool SocketClient::send(Packet* packet) {
    if (this->socket_log_state != SockState::CONNECTED || packet == nullptr)
        return false;

    char* buffer = reinterpret_cast<char*>(packet);

    int valread = 0;

    if (packet->mType != PLAYERINF && packet->mType != HACKCAPINF)
        Logger::log("Sending packet: %s\n", packetNames[packet->mType]);

    valread = nn::socket::Send(this->socket_log_socket, buffer, packet->mPacketSize + sizeof(Packet), 0);

    if (valread > 0) {
        return true;
    } else {
        Logger::log("Failed to Fully Send Packet! Result: %d Type: %s Packet Size: %d\n", valread,
                    packetNames[packet->mType], packet->mPacketSize);
        this->socket_errno = nn::socket::GetLastErrno();
        return false;
    }
    return true;
}

bool SocketClient::recv() {
    if (this->socket_log_state != SockState::CONNECTED) {
        Logger::log("Unable To Receive! Socket Not Connected.\n");
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
                Logger::log("Header Read Failed! Value: %d Total Read: %d\n", result, valread);
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
                Logger::log("Received packet (from %02X%02X):", header->mUserID.data[0],
                            header->mUserID.data[1]);
                Logger::disableName();
                Logger::log(" Size: %d", header->mPacketSize);
                Logger::log(" Type: %d", header->mType);
                if (packetNames[header->mType])
                    Logger::log(" Type String: %s\n", packetNames[header->mType]);
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
                        Logger::log("Packet Read Failed! Value: %d\nPacket Size: %d\nPacket Type: %s\n",
                                    result, header->mPacketSize, packetNames[header->mType]);
                        return false;
                    }
                }

                Packet* packet = reinterpret_cast<Packet*>(packetBuf);

                if (!(mRecvQueue.mMessageQueueInner._count == mRecvQueue.mMessageQueueInner._maxCount)) {
                    mRecvQueue.push((s64)packet, sead::MessageQueue::BlockType::NonBlocking);
                } else {
                    free(packetBuf);
                }
            }
        } else {
            Logger::log("Failed to aquire valid data! Packet Type: %d Full Packet Size %d valread size: %d\n",
                        header->mType, fullSize, valread);
        }

        return true;
    } else {  // if we error'd, close the socket
        Logger::log("valread was zero! Disconnecting.\n");
        this->socket_errno = nn::socket::GetLastErrno();
        return false;
    }
}

// prints packet to debug logger
void SocketClient::printPacket(Packet* packet) {
    packet->mUserID.print();
    Logger::log("Type: %s\n", packetNames[packet->mType]);

    switch (packet->mType) {
    case PacketType::PLAYERINF:
        Logger::log("Pos X: %f Pos Y: %f Pos Z: %f\n", ((PlayerInf*)packet)->playerPos.x,
                    ((PlayerInf*)packet)->playerPos.y, ((PlayerInf*)packet)->playerPos.z);
        Logger::log("Rot X: %f Rot Y: %f Rot Z: %f\nRot W: %f\n", ((PlayerInf*)packet)->playerRot.x,
                    ((PlayerInf*)packet)->playerRot.y, ((PlayerInf*)packet)->playerRot.z,
                    ((PlayerInf*)packet)->playerRot.w);
        break;
    default:
        break;
    }
}

bool SocketClient::closeSocket() {
    Logger::log("Closing Socket.\n");

    nn::Result result(-1);

    while (result.IsFailure()) {
        result = nn::socket::Close(this->socket_log_socket);

        if (result.IsFailure()) {
            Logger::log("Failed to close socket!\n");
            nn::os::YieldThread();
            nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(100000000));
        }
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
    struct hostent* he = nn::socket::GetHostByName(str);
    if (!he) {
        return false;
    }

    // might give us multiple IP addresses, so pick the first one
    struct in_addr** addr_list = (struct in_addr**)he->h_addr_list;
    for (int i = 0; addr_list[i] != NULL; i++) {
        *out = *addr_list[i];
        return true;
    }

    return false;
}

void SocketClient::sendFunc() {
    Logger::log("Starting Send Thread.\n");

    while (trySendQueue() && socket_log_state != SockState::DISCONNECTED) {
    }

    this->socket_log_state = SockState::DISCONNECTED;

    Logger::log("Sending packet failed!\n");
    Logger::log("Ending Send Thread.\n");

    if (mState != RESET && mState != RECONNECT)
        mState = RESET;
}

void SocketClient::recvFunc() {
    nn::socket::Recv(this->socket_log_socket, nullptr, 0, 0);

    Logger::log("Starting Recv Thread.\n");

    while (recv() && socket_log_state != SockState::DISCONNECTED) {
    }

    this->socket_log_state = SockState::DISCONNECTED;

    Logger::log("Receiving Packet Failed!\n");
    Logger::log("Ending Recv Thread.\n");

    if (mState != RESET && mState != RECONNECT)
        mState = RESET;
}

bool SocketClient::queuePacket(Packet* packet) {
    if (socket_log_state == SockState::CONNECTED &&
        !(mSendQueue.mMessageQueueInner._count == mSendQueue.mMessageQueueInner._maxCount)) {
        // as this is non-blocking, it will always return true.
        mSendQueue.push((s64)packet, sead::MessageQueue::BlockType::NonBlocking);
        return true;
    } else {
        delete packet;
        return false;
    }
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