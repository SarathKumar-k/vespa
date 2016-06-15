// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".proton.metrics.content_proton_metrics");
#include "content_proton_metrics.h"

namespace proton {

ContentProtonMetrics::ContentProtonMetrics()
    : metrics::MetricSet("content.proton", "", "Search engine metrics", nullptr),
      transactionLog(this),
      resourceUsage(this)
{
}

} // namespace proton
