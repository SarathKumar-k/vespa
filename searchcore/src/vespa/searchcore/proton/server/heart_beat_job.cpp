// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include "heart_beat_job.h"

namespace proton {

HeartBeatJob::HeartBeatJob(IHeartBeatHandler &handler,
                           const DocumentDBHeartBeatConfig &config)
    : IMaintenanceJob("heart_beat", config.getInterval(), config.getInterval()),
      _handler(handler)
{
}

bool
HeartBeatJob::run()
{
    _handler.heartBeat();
    return true;
}

} // namespace proton
