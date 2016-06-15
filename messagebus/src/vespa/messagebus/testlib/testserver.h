// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <boost/utility.hpp>
#include <memory>
#include <vespa/messagebus/messagebus.h>
#include <vespa/messagebus/network/rpcnetwork.h>

namespace mbus {

class Identity;
class RoutingTableSpec;
class Slobrok;
class SlobrokState;
class OOSState;

class VersionedRPCNetwork : public RPCNetwork {
private:
    vespalib::Version _version;

protected:
    const vespalib::Version &getVersion() const { return _version; }

public:
    VersionedRPCNetwork(const RPCNetworkParams &params);
    void setVersion(const vespalib::Version &version);
};

class TestServer : public boost::noncopyable {
public:
    typedef std::unique_ptr<TestServer> UP;

    VersionedRPCNetwork net;
    MessageBus mb;

    TestServer(const Identity &ident,
               const RoutingSpec &spec,
               const Slobrok &slobrok,
               const string &oosServerPattern = "",
               IProtocol::SP protocol = IProtocol::SP());
    TestServer(const MessageBusParams &mbusParams,
               const RPCNetworkParams &netParams);

    bool waitSlobrok(const string &pattern, uint32_t cnt = 1);
    bool waitOOS(const string &service);

    bool waitState(const SlobrokState &slobrokState);
    bool waitState(const OOSState &oosState);
};

} // namespace mbus

