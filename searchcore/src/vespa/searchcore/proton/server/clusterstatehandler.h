// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/document/bucket/bucketid.h>
#include "ibucketmodifiedhandler.h"
#include "ibucketstatecalculator.h"
#include "iclusterstatechangednotifier.h"
#include <vespa/persistence/spi/clusterstate.h>
#include <vespa/searchcore/proton/persistenceengine/resulthandler.h>
#include <vespa/vespalib/util/threadstackexecutorbase.h>

namespace proton {

/**
 * Class handling operations in IPersistenceHandler that are dealing with
 * that the cluster state is changing.
 */
class ClusterStateHandler : public IBucketModifiedHandler,
                            public IClusterStateChangedNotifier
{
private:
    vespalib::Executor &_executor;
    std::vector<IClusterStateChangedHandler *> _changedHandlers;
    // storage::spi::BucketIdListResult::List _modifiedBuckets;
    std::set<document::BucketId> _modifiedBuckets;

    void
    performSetClusterState(const storage::spi::ClusterState *calc,
                           IGenericResultHandler *resultHandler);

    void
    performGetModifiedBuckets(IBucketIdListResultHandler *resultHandler);

    // Implements IBucketModifiedHandler
    virtual void
    notifyBucketModified(const document::BucketId &bucket);

public:
    ClusterStateHandler(vespalib::Executor &executor);

    virtual
    ~ClusterStateHandler();

    virtual void
    addClusterStateChangedHandler(IClusterStateChangedHandler *handler);

    virtual void
    removeClusterStateChangedHandler(IClusterStateChangedHandler *handler);

    /**
     * Implements the cluster state aspect of IPersistenceHandler.
     */
    void
    handleSetClusterState(const storage::spi::ClusterState &calc,
                               IGenericResultHandler &resultHandler);

    void
    handleGetModifiedBuckets(IBucketIdListResultHandler &resultHandler);
};

} // namespace proton

