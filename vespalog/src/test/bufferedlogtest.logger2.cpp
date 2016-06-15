// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/log/log.h>
#include "bufferedlogtest.logger2.h"

LOG_SETUP(".logger2");

void logWithLogger2(const std::string& token, const std::string message)
{
    LOGBT(info, token, message.c_str());
}
