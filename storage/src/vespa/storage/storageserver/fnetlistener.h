// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/fnet/frt/frt.h>
#include <vespa/slobrok/sbmirror.h>
#include <vespa/slobrok/sbregister.h>


namespace storage {

class CommunicationManager;
class StorageServerInterface;

class FNetListener :  public FRT_Invokable
{
public:
    FNetListener(CommunicationManager& comManager,
                 const config::ConfigUri & configUri, uint32_t port);
    ~FNetListener();

    void initRPC();
    void RPC_getNodeState2(FRT_RPCRequest *req);
    void RPC_setSystemState2(FRT_RPCRequest *req);
    void RPC_getCurrentTime(FRT_RPCRequest *req);

    void registerHandle(const vespalib::stringref & handle);
    void close();


    // Used by unit tests.
    bool serviceExists(const vespalib::stringref & connectionSpec);

private:
    CommunicationManager& _comManager;
    FRT_Supervisor _orb;
    bool _closed;
    slobrok::api::RegisterAPI       _slobrokRegister;
    vespalib::string _handle;
};

}

