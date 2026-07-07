#pragma once

#include <netinet/in.h>
#include <thread/seadAtomic.h>
#include <thread/seadMessageQueue.h>

#include "Library/Thread/AsyncFunctorThread.h"
#include "packets/Packet.h"
#include "SocketBase.hpp"
#include "types.h"

class SocketClient : public SocketBase {
public:
    enum SocketClientState { WAIT = 0, INIT = 1, RESET = 2, RECONNECT = 3 };

    SocketClient();

    void update();
    bool exeInit();
    void exeReset();

    void init(const char* ip, u16 port);
    bool closeSocket();
    Packet* tryGetPacket();

    bool startThreads();
    void endThreads();

    bool send(Packet* packet);
    bool recv();

    bool queuePacket(Packet* packet);
    bool trySendQueue();

    void sendFunc();
    void recvFunc();

    void setLogState(SockState state) { socket_log_state = state; };

    void printPacket(Packet* packet);
    bool isConnected() { return socket_log_state == SockState::CONNECTED; }

    u32 getSendCount() { return mSendQueue.mMessageQueueInner._count; }
    u32 getSendMaxCount() { return mSendQueue.mMessageQueueInner._maxCount; }

    u32 getRecvCount() { return mRecvQueue.mMessageQueueInner._count; }
    u32 getRecvMaxCount() { return mRecvQueue.mMessageQueueInner._maxCount; }

    SocketClientState getSocketClientState() { return mState; }

private:
    al::AsyncFunctorThread* mSocketThread = nullptr;
    al::AsyncFunctorThread* mRecvThread = nullptr;
    al::AsyncFunctorThread* mSendThread = nullptr;

    sead::MessageQueue mRecvQueue;
    sead::MessageQueue mSendQueue;

    bool mIsFirstConnect = true;

    sead::Atomic<SocketClientState> mState = INIT;

    /**
     * @param str a string containing an IPv4 address or a hostname that can be resolved via DNS
     * @param out IPv4 address
     * @return if this function was successfull and out contains a valid IP address
     */
    bool stringToIPAddress(const char* str, in_addr* out);
};
