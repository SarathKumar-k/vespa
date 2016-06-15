// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include "storagelinkqueued.h"
#include <vespa/log/log.h>

LOG_SETUP(".application.link.queued");

namespace storage {

StorageLinkQueued::~StorageLinkQueued()
{
    if (_closeState != 7) {
        LOG(error, "Link %s has closing state %u at destruction. Has likely "
                   "implemented onFlush/onClose without calling storage link "
                   "queued's implementations. This is a bug which can cause "
                   "crashes on shutdown.",
            getName().c_str(), _closeState);
    }
}

void StorageLinkQueued::dispatchDown(
        const std::shared_ptr<api::StorageMessage>& msg)
{
        // Verify acceptable state to send messages down
    switch(getState()) {
        case OPENED:
        case CLOSING:
        case FLUSHINGDOWN:
            break;
        default:
            LOG(error, "Link %s trying to dispatch %s down while in state %u",
                toString().c_str(), msg->toString().c_str(), getState());
            assert(false);
    }
    _commandDispatcher.add(msg);
}

void StorageLinkQueued::dispatchUp(
        const std::shared_ptr<api::StorageMessage>& msg)
{
        // Verify acceptable state to send messages up
    switch(getState()) {
        case OPENED:
        case CLOSING:
        case FLUSHINGDOWN:
        case FLUSHINGUP:
            break;
        default:
            LOG(error, "Link %s trying to dispatch %s up while in state %u",
                toString().c_str(), msg->toString().c_str(), getState());
            assert(false);
    }
    _replyDispatcher.add(msg);
}

void StorageLinkQueued::logError(const char* err) {
    LOG(error, "%s", err);
};

void StorageLinkQueued::logDebug(const char* err) {
    LOG(info, "%s", err);
};

} // storage
