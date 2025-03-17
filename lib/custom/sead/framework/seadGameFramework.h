#pragma once

#include "sead/math/seadVector.h"
#include "sead/thread/seadThread.h"
#include "sead/prim/seadRuntimeTypeInfo.h"
#include <framework/seadFramework.h>
#include <heap/seadHeap.h>
#include <prim/seadRuntimeTypeInfo.h>
#include <prim/seadSafeString.h>

#include "nvn/nvn.h"

namespace sead
{
    class GameFrameworkNx {
        public:
            struct CreateArg;

            GameFrameworkNx(sead::GameFrameworkNx::CreateArg const&);
            ~GameFrameworkNx();
            void initializeGraphicsSystem(sead::Heap *,sead::Vector2<float> const&);
            void outOfMemoryCallback_(NVNcommandBuffer *,NVNcommandBufferMemoryEvent,ulong,void *);
            void presentAsync_(sead::Thread *,long);
            void getAcquiredDisplayBufferTexture(void);
            void setVBlankWaitInterval(unsigned int);
            void requestChangeUseGPU(bool);
            void getGraphicsDevToolsAllocatorTotalFreeSize(void);
            void initRun_(sead::Heap *);
            void runImpl_(void);
            void createMethodTreeMgr_(sead::Heap *);
            void mainLoop_(void);
            void procFrame_(void);
            void procDraw_(void);
            void procCalc_(void);
            void present_(void);
            void waitVsyncEvent_(void);
            void swapBuffer_(void);
            void clearFrameBuffers_(int);
            void waitForGpuDone_(void);
            void setGpuTimeStamp_(void);
            void getMethodFrameBuffer(int);
            void getMethodLogicalFrameBuffer(int);
            void checkDerivedRuntimeTypeInfo(sead::RuntimeTypeInfo::Interface const*);
            void getRuntimeTypeInfo(void);
            float calcFps(void);
            void setCaption(sead::SafeStringBase<char> const&);
    };

class GameFramework : public Framework
{
    SEAD_RTTI_OVERRIDE(GameFramework, Framework);

public:
    GameFramework();
    // TODO: implement (missing unk1)
    ~GameFramework() override;

    void createSystemTasks(TaskBase* base,
                           const Framework::CreateSystemTaskArg& createSystemTaskArg) override;
    void quitRun_(Heap* heap) override;
    // TODO: implement (missing TaskBase::SystemMgrTaskArg, sead::TTaskFactory)
    virtual void createControllerMgr(TaskBase* base);
    virtual void createHostIOMgr(TaskBase* base, HostIOMgr::Parameter* hostioParam, Heap* heap);
    virtual void createProcessMeter(TaskBase* base);
    virtual void createSeadMenuMgr(TaskBase* base);
    virtual void createInfLoopChecker(TaskBase* base, const TickSpan&, int);
    virtual void createCuckooClock(TaskBase* base);
    virtual void saveScreenShot(const SafeString&);
    virtual bool isScreenShotBusy() const;
    virtual void waitStartDisplayLoop_();
    virtual void initHostIO_();

    void startDisplay();
    void initialize(const Framework::InitializeArg&);
    void lockFrameDrawContext();
    void unlockFrameDrawContext();

public:
    int mDisplayStarted = 0;
    sead::SafeString mUnk1 = "";
    sead::SafeString mUnk2 = "";
    sead::SafeString mUnk3 = "";
    void* mUnk4 = nullptr;
    void (*mUnk5)(bool) = nullptr;
    void (*mUnk6)(bool);
};
} // namespace sead