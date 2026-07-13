#pragma once

#include <netinet/in.h>
#include <thread/seadAtomic.h>
#include <thread/seadCriticalSection.h>
#include <thread/seadMessageQueue.h>
#include <prim/seadScopedLock.h>

#include "Library/Thread/AsyncFunctorThread.h"
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
    void closeSocket();
    // Frames are owned by the caller and must be released with delete[].  They
    // are raw legacy-wire bytes, never Packet objects.
    u8* tryGetFrame();

    bool startThreads();
    void endThreads();

    bool recv();

    // This is the only public transmit entry point.  The send worker is the
    // sole writer to the TCP socket and performs complete-frame writes.
    bool queueFrame(const u8* frame, s32 frameSize);
    bool trySendQueue();

    void sendFunc();
    void recvFunc();

    void setLogState(SockState state) { socket_log_state = state; };

    bool isConnected() { return socket_log_state == SockState::CONNECTED; }

    u32 getSendCount() { return mSendQueue.mMessageQueueInner._count; }
    u32 getSendMaxCount() { return mSendQueue.mMessageQueueInner._maxCount; }

    u32 getRecvCount() { return mRecvQueue.mMessageQueueInner._count; }
    u32 getRecvMaxCount() { return mRecvQueue.mMessageQueueInner._maxCount; }

    SocketClientState getSocketClientState() { return mState; }

    void setSocketClientState(SocketClientState state) { mState = state; }

private:
    al::AsyncFunctorThread* mSocketThread = nullptr;
    al::AsyncFunctorThread* mRecvThread = nullptr;
    al::AsyncFunctorThread* mSendThread = nullptr;

    sead::MessageQueue mRecvQueue;
    sead::MessageQueue mSendQueue;

    // Transform snapshots are deliberately not FIFO.  At most one latest
    // player and cap frame are retained while reliable messages stay ordered.
    sead::CriticalSection mStateFrameLock;
    u8* mLatestPlayerFrame = nullptr;
    u8* mLatestCapFrame = nullptr;
    bool mStateWakeQueued = false;

    static constexpr s64 StateWakeMessage = 1;

    bool mIsFirstConnect = true;
    s32 mReconnectBackoffMs = 250;

    sead::Atomic<SocketClientState> mState = INIT;

    bool sendAll(const u8* buffer, s32 size);
    bool readExact(u8* buffer, s32 size);
    void clearFrameQueue(sead::MessageQueue& queue);
    bool queueStateFrame(u8* frame, s16 type);
    bool sendLatestStateFrames();

    /**
     * @param str a string containing an IPv4 address or a hostname that can be resolved via DNS
     * @param out IPv4 address
     * @return if this function was successfull and out contains a valid IP address
     */
    bool stringToIPAddress(const char* str, in_addr* out);
};
