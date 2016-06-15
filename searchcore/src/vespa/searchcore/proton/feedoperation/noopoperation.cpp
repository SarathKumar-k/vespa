// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".proton.feedoperation.noopoperation");

#include "noopoperation.h"
#include <vespa/vespalib/util/stringfmt.h>

using vespalib::make_string;

namespace proton {

NoopOperation::NoopOperation(SerialNum serialNum)
    : FeedOperation(FeedOperation::NOOP)
{
    setSerialNum(serialNum);
}

vespalib::string NoopOperation::toString() const {
    return make_string("Noop(serialNum=%" PRIu64 ")", getSerialNum());
}

} // namespace proton
