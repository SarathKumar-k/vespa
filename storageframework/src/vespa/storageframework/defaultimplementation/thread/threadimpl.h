// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/vespalib/util/document_runnable.h>
#include <vespa/storageframework/storageframework.h>
#include <array>
#include <atomic>

namespace storage {
namespace framework {
namespace defaultimplementation {

class ThreadPoolImpl;

class ThreadImpl : public Thread
{
    struct BackendThread : public document::Runnable {
        ThreadImpl& _impl;
        BackendThread(ThreadImpl& impl) : _impl(impl) {}
        virtual void run() { _impl.run(); }
    };

    /**
     * Internal data race free implementation of tick data that maps to and
     * from ThreadTickData. We hide the atomicity of this since atomic vars
     * are not CopyConstructible and thus would impose unnecessary limitations
     * on code using it.
     */
    struct AtomicThreadTickData {
        std::atomic<CycleType> _lastTickType;
        std::atomic<uint64_t> _lastTickMs;
        std::atomic<uint64_t> _maxProcessingTimeSeenMs;
        std::atomic<uint64_t> _maxWaitTimeSeenMs;
        // struct stores and loads are both data race free with relaxed
        // memory semantics. This means it's possible to observe stale/partial
        // state in a case with concurrent readers/writers.
        ThreadTickData loadRelaxed() const noexcept;
        void storeRelaxed(const ThreadTickData& newState) noexcept;
    };

    ThreadPoolImpl& _pool;
    Runnable& _runnable;
    ThreadProperties _properties;
    std::array<AtomicThreadTickData, 3> _tickData;
    uint32_t _tickDataPtr;
    bool _interrupted;
    bool _joined;
    BackendThread _thread;

    void run();

public:
    ThreadImpl(ThreadPoolImpl&,
               Runnable&,
               vespalib::stringref id,
               uint64_t waitTimeMs,
	       uint64_t maxProcessTimeMs,
	       int ticksBeforeWait);
    ~ThreadImpl();

    virtual bool interrupted() const;
    virtual bool joined() const;
    virtual void interrupt();
    virtual void join();
    virtual void registerTick(CycleType, MilliSecTime);
    virtual uint64_t getWaitTime() const {
        return _properties.getWaitTime();
    }
    virtual int getTicksBeforeWait() const {
        return _properties.getTicksBeforeWait();
    }
    virtual uint64_t getMaxProcessTime() const {
        return _properties.getMaxProcessTime();
    }

    virtual void updateParameters(uint64_t waitTime,
                                  uint64_t maxProcessTime,
                                  int ticksBeforeWait);

    void setTickData(const ThreadTickData&);
    ThreadTickData getTickData() const;
    const ThreadProperties& getProperties() const
        { return _properties; }
};

} // defaultimplementation
} // framework
} // storage

