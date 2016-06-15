// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/config/subscription/configsubscriber.h>
#include <vespa/config/common/timingvalues.h>
#include <vespa/vespalib/util/runnable.h>
#include "ifetchercallback.h"
#include "ihandle.h"

namespace config {

/**
 * A config poller runs a polling sequence on a set of configs that it has
 * subscribed to.
 */
class ConfigPoller : public vespalib::Runnable {
public:
    ConfigPoller(const IConfigContext::SP & context);
    void run();
    template <typename ConfigType>
    void subscribe(const std::string & configId, IFetcherCallback<ConfigType> * callback, uint64_t subscribeTimeout = DEFAULT_SUBSCRIBE_TIMEOUT);
    void subscribeGenerationChanges(IGenerationCallback * callback) { _genCallback = callback; }
    void poll();
    void close();
    int64_t getGeneration() const { return _generation; }
private:
    int64_t _generation;
    ConfigSubscriber _subscriber;
    std::vector<IHandle::UP> _handleList;
    std::vector<ICallback *> _callbackList;
    IGenerationCallback *_genCallback;
};


} // namespace config

#include "configpoller.hpp"


