#pragma once

#include "al/Library/Thread/AsyncFunctorThread.h"

#include "sead/container/seadPtrArray.h"

#include <netinet/in.h>

#include "heap/seadHeap.h"
#include "packets/Packet.h"
#include "SocketBase.hpp"
#include "syssocket/sockdefines.h"
#include "thread/seadMessageQueue.h"
#include "thread/seadMutex.h"
#include "types.h"

class SocketClient : public SocketBase {
public:
    SocketClient(const char* name, sead::Heap* heap);
    nn::Result init(const char* ip, u16 port) override;
    bool tryReconnect() override;
    bool closeSocket() override;
    Packet* tryGetPacket() override;

    bool startThreads();
    void endThreads();

    bool send(Packet* packet);
    bool recv();

    bool queuePacket(Packet* packet);
    bool trySendQueue();

    void sendFunc();
    void recvFunc();

    void printPacket(Packet* packet);
    bool isConnected() { return socket_log_state == SOCKET_LOG_CONNECTED; }

    u32 getSendCount() { return mSendQueue.mMessageQueueInner._count; }
    u32 getSendMaxCount() { return mSendQueue.mMessageQueueInner._maxCount; }

    u32 getRecvCount() { return mRecvQueue.mMessageQueueInner._count; }
    u32 getRecvMaxCount() { return mRecvQueue.mMessageQueueInner._maxCount; }

    void setIsFirstConn(bool value) { mIsFirstConnect = value; }

private:
    sead::Heap* mHeap = nullptr;

    al::AsyncFunctorThread* mRecvThread = nullptr;
    al::AsyncFunctorThread* mSendThread = nullptr;

    sead::MessageQueue mRecvQueue;
    sead::MessageQueue mSendQueue;

    int maxBufSize = 100;
    bool mIsFirstConnect = true;

    /**
     * @param str a string containing an IPv4 address or a hostname that can be resolved via DNS
     * @param out IPv4 address
     * @return if this function was successfull and out contains a valid IP address
     */
    bool stringToIPAddress(const char* str, in_addr* out);
};

typedef void (SocketClient::*SocketThreadFunc)(void);