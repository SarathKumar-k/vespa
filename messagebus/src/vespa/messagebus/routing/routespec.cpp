// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".routespec");

#include <vespa/vespalib/util/vstringfmt.h>
#include "routingspec.h"

namespace mbus {

RouteSpec::RouteSpec(const string &name) :
    _name(name),
    _hops()
{
    // empty
}

RouteSpec &
RouteSpec::addHops(const std::vector<string> &hops)
{
    _hops.insert(_hops.end(), hops.begin(), hops.end());
    return *this;
}

string
RouteSpec::removeHop(uint32_t i)
{
    string ret = _hops[i];
    _hops.erase(_hops.begin() + i);
    return ret;
}

void
RouteSpec::toConfig(string &cfg, const string &prefix) const
{
    cfg.append(prefix).append("name ").append(RoutingSpec::toConfigString(_name)).append("\n");
    uint32_t numHops = _hops.size();
    if (numHops > 0) {
        cfg.append(prefix).append("hop[").append(vespalib::make_vespa_string("%d", numHops)).append("]\n");
        for (uint32_t i = 0; i < numHops; ++i) {
            cfg.append(prefix).append("hop[").append(vespalib::make_vespa_string("%d", i)).append("] ");
            cfg.append(RoutingSpec::toConfigString(_hops[i])).append("\n");
        }
    }
}

string
RouteSpec::toString() const
{
    string ret = "";
    toConfig(ret, "");
    return ret;
}

bool
RouteSpec::operator==(const RouteSpec &rhs) const
{
    if (_name != rhs._name) {
        return false;
    }
    if (_hops.size() != rhs._hops.size()) {
        return false;
    }
    for (uint32_t i = 0, len = _hops.size(); i < len; ++i) {
        if (_hops[i] != rhs._hops[i]) {
            return false;
        }
    }
    return true;
}

} // namespace mbus
