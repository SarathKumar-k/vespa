// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "legacysubscriber.h"

namespace config {

LegacySubscriber::LegacySubscriber()
    : _fetcher(),
      _configId()
{
}

void
LegacySubscriber::close()
{
    if (_fetcher.get() != NULL)
        _fetcher->close();
}

} // namespace config
