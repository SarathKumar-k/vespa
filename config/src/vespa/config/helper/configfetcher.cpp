// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/log/log.h>
LOG_SETUP(".config.helper.configfetcher");
#include "configfetcher.h"

namespace config {

ConfigFetcher::ConfigFetcher(const IConfigContext::SP & context)
    : _poller(context),
      _thread(_poller),
      _closed(false),
      _started(false)
{
}

ConfigFetcher::ConfigFetcher(const SourceSpec & spec)
    : _poller(IConfigContext::SP(new ConfigContext(spec))),
      _thread(_poller),
      _closed(false),
      _started(false)
{
}

void
ConfigFetcher::start()
{
    if (!_closed) {
        LOG(debug, "Polling for config");
        _poller.poll();
        LOG(debug, "Starting fetcher thread...");
        _thread.start();
        _started = true;
        LOG(debug, "Fetcher thread started");
    }
}

ConfigFetcher::~ConfigFetcher()
{
    close();
}

void
ConfigFetcher::close()
{
    if (!_closed) {
        _poller.close();
        if (_started)
            _thread.join();
    }
}

} // namespace config
