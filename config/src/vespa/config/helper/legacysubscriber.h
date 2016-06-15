// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "configfetcher.h"
#include <vespa/config/subscription/sourcespec.h>
#include "legacy.h"

namespace config {

/**
 * A LegacySubscriber subscribes for a config similar to the old config API.
 */
class LegacySubscriber
{
public:
    LegacySubscriber();
    const vespalib::string & id() const { return _configId; }

    template <typename ConfigType>
    void subscribe(const std::string & configId, IFetcherCallback<ConfigType> * callback);

    void close();
private:
    std::unique_ptr<ConfigFetcher> _fetcher;
    vespalib::string _configId;
};

} // namespace config

#include "legacysubscriber.hpp"

