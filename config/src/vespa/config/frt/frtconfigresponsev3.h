// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "slimeconfigresponse.h"
#include <vespa/config/common/configkey.h>
#include <vespa/config/common/configvalue.h>
#include <vespa/config/common/trace.h>
#include <vespa/vespalib/data/slime/slime.h>
#include "protocol.h"

class FRT_RPCRequest;
class FRT_Values;

namespace config {

/**
 * Baseclass for config responses.
 */
class FRTConfigResponseV3 : public SlimeConfigResponse {
private:
    FRTConfigResponseV3& operator=(const FRTConfigResponseV3&);
public:
    FRTConfigResponseV3(FRT_RPCRequest * request);

private:
    static const vespalib::string RESPONSE_TYPES;
    const vespalib::string & getResponseTypes() const;
    const ConfigValue readConfigValue() const;
};

} // namespace config

