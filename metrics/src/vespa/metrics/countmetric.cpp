// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/metrics/countmetric.h>

#include <vespa/log/log.h>

LOG_SETUP(".metrics.metric.count");

namespace metrics {

void
AbstractCountMetric::logWarning(const char* msg) const
{
    LOG(warning, "%s", msg);
}

void
AbstractCountMetric::sendLogCountEvent(
        Metric::String name, uint64_t value) const
{
    EV_COUNT(name.c_str(), value);
}

} // metrics
