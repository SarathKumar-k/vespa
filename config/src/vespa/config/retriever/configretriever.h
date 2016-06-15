// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/vespalib/stllike/string.h>
#include <vespa/config/common/configkey.h>
#include <vespa/config/subscription/configsubscription.h>
#include "configkeyset.h"
#include "configsnapshot.h"
#include "genericconfigsubscriber.h"
#include "fixedconfigsubscriber.h"

namespace config {

/**
 * A ConfigRetriever is a helper class for retrieving a set of dynamically
 * changing and depending configs. You should use this class whenever you have a
 * set of bootstrap configs, and want to subscribe to a dynamically changing set
 * of configs based on those.
 *
 * The retriever should be used from one thread only, but close can be called
 * from another thread.
 */
class ConfigRetriever
{
public:
    ConfigRetriever(const ConfigKeySet & bootstrapSet,
                    const IConfigContext::SP & context,
                    int64_t subscribeTimeout = DEFAULT_SUBSCRIBE_TIMEOUT);

    /**
     * Waits for the next generation of bootstrap configs to arrive, and returns
     * them. If no new generation has arrived, return an empty snapshot.
     *
     * @param timeoutInMillis The timeout of the nextGeneration call, in
     *                        milliseconds. Optional.
     * @return a snapshot of bootstrap configs, empty if no new snapshot or
     *         retriever has been closed.
     * @throws ConfigTimeoutException if initial subscribe timed out.
     */
    ConfigSnapshot getBootstrapConfigs(int timeoutInMillis = DEFAULT_NEXTGENERATION_TIMEOUT);

    /**
     * Return the configs represented by a ConfigKeySet in a snapshot, and makes
     * sure that it is in sync with the bootstrap config. If it is not, an empty
     * snapshot is returned.
     *
     * @param keySet The set of configs that should be fetched. The set may only
     *               change when bootstrap has been changed.
     * @param timeoutInMillis The timeout, in milliseconds. Optional.
     * @return a snapshot of configs corresponding to the keySet or
     *         an empty snapshot if
     *              a) retriever has been closed. The isClosed() method can be
     *                 used to check for this condition.
     *              b) no new generation was found, in which case getConfigs()
     *                 should be called again.
     *              c) generation is not in sync with bootstrap, in which case
     *                 getBootstrapConfigs() must be called. The bootstrapRequired
     *                 method can be used to check for this condition.
     * @throws ConfigTimeoutException if resubscribe timed out.
     */
    ConfigSnapshot getConfigs(const ConfigKeySet & keySet, int timeoutInMillis = DEFAULT_NEXTGENERATION_TIMEOUT);

    /**
     * Close this retriever in order to shut down.
     */
    void close();

    /**
     * Check if this retriever is closed.
     *
     * @return true if closed, false if not.
     */
    bool isClosed() const;

    /**
     * Returns if a new bootstrap call is required.
     *
     * @return true if required, false if not.
     */
    bool bootstrapRequired() const { return _bootstrapRequired; }

    /**
     * Get the current generation of the configs managed by this retriever.
     *
     * @return the generation
     */
    int64_t getGeneration() const { return _generation; }

    static const int DEFAULT_SUBSCRIBE_TIMEOUT = 60000;
    static const int DEFAULT_NEXTGENERATION_TIMEOUT = 60000;
private:
    FixedConfigSubscriber _bootstrapSubscriber;
    std::unique_ptr<GenericConfigSubscriber> _configSubscriber;
    vespalib::Lock _lock;
    std::vector<ConfigSubscription::SP> _subscriptionList;
    ConfigKeySet _lastKeySet;
    IConfigContext::SP _context;
    std::unique_ptr<SourceSpec> _spec;
    bool _closed;
    int64_t _generation;
    int64_t _subscribeTimeout;
    bool _bootstrapRequired;
};

} // namespace config

