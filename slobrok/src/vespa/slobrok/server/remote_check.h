// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/fnet/fnet.h>
#include <vespa/fnet/frt/frt.h>

namespace slobrok {

class SBEnv;
class RpcServerMap;
class RpcServerManager;
class ExchangeManager;

/**
 * @class RemoteCheck
 * @brief Periodic healthcheck task for remote objects
 *
 * Checks the health of partner location brokers
 * and their NamedService objects periodically.
 **/
class RemoteCheck : public FNET_Task
{
private:
    RpcServerMap &_rpcsrvmap;
    RpcServerManager &_rpcsrvmanager;
    ExchangeManager &_exchanger;

    RemoteCheck(const RemoteCheck &);            // Not used
    RemoteCheck &operator=(const RemoteCheck &); // Not used
public:
    explicit RemoteCheck(FNET_Scheduler *sched,
                         RpcServerMap& rpcsrvmap,
                         RpcServerManager& rpcsrvman,
                         ExchangeManager& exchanger);
    virtual ~RemoteCheck();
private:
    virtual void PerformTask();
};

} // namespace slobrok

