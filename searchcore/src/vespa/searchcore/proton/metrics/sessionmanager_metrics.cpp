// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".grouping.sessionmanagermetrics");
#include "sessionmanager_metrics.h"

namespace search {
namespace grouping {

SessionManagerMetrics::SessionManagerMetrics(metrics::MetricSet *parent)
    : metrics::MetricSet("sessionmanager", "",
                         "Grouping session manager metrics",
                         parent),
      numInsert("numinsert", "", "Number of inserted sessions", this),
      numPick("numpick", "", "Number if picked sessions", this),
      numDropped("numdropped", "", "Number of dropped cached sessions", this),
      numCached("numcached", "", "Number of currently cached sessions", this),
      numTimedout("numtimedout", "", "Number of timed out sessions", this)
{
}

void
SessionManagerMetrics::update(
        const proton::matching::SessionManager::Stats &stats)
{
    numInsert.inc(stats.numInsert);
    numPick.inc(stats.numPick);
    numDropped.inc(stats.numDropped);
    numCached.set(stats.numCached);
    numTimedout.inc(stats.numTimedout);
}


}  // namespace grouping
}  // namespace search
