// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/fnet/fnet.h>
#include <vespa/vespalib/util/signalhandler.h>

void
FNET_SignalShutDown::PerformTask()
{
    typedef vespalib::SignalHandler SIG;
    if (SIG::INT.check() || SIG::TERM.check()) {
        fprintf(stderr, "got signal, shutting down...\n");
        _transport.ShutDown(false);
    } else {
        Schedule(0.1);
    }
}

void
FNET_SignalShutDown::hookSignals()
{
    typedef vespalib::SignalHandler SIG;
    SIG::INT.hook();
    SIG::TERM.hook();
}
