// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".rpcnetworkparams");

#include "rpcnetworkparams.h"

#include <vespa/slobrok/cfg.h>

namespace mbus {

RPCNetworkParams::RPCNetworkParams() :
    _identity(Identity("")),
    _slobrokConfig("admin/slobrok.0"),
    _oosServerPattern(""),
    _listenPort(0),
    _maxInputBufferSize(256*1024),
    _maxOutputBufferSize(256*1024),
    _connectionExpireSecs(30)
{
    // empty
}

}

