// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/storage/distributor/operations/external/removeoperation.h>
#include <vespa/document/fieldvalue/document.h>
#include <vespa/storageapi/message/persistence.h>
#include <vespa/log/log.h>
#include <vespa/storage/bucketdb/distrbucketdb.h>
#include <vespa/storage/distributor/distributormetricsset.h>

LOG_SETUP(".distributor.operation.external.remove");


using namespace storage::distributor;
using namespace storage;

RemoveOperation::RemoveOperation(DistributorComponent& manager,
                               const std::shared_ptr<api::RemoveCommand> & msg,
                               PersistenceOperationMetricSet& metric)
    : Operation(),
      _trackerInstance(metric,
               std::shared_ptr<api::BucketInfoReply>(new api::RemoveReply(*msg)),
               manager, msg->getTimestamp()),
      _tracker(_trackerInstance),
      _msg(msg),
      _manager(manager)
{
}

void
RemoveOperation::onStart(DistributorMessageSender& sender)
{
    LOG(spam,
        "Started remove on document %s",
        _msg->getDocumentId().toString().c_str());

    document::BucketId bucketId(
            _manager.getBucketIdFactory().getBucketId(
                    _msg->getDocumentId()));

    std::vector<BucketDatabase::Entry> entries;
    _manager.getBucketDatabase().getParents(bucketId, entries);

    bool sent = false;

    for (uint32_t j = 0; j < entries.size(); j++) {
        const BucketDatabase::Entry& e = entries[j];
        std::vector<MessageTracker::ToSend> messages;

        for (uint32_t i = 0; i < e->getNodeCount(); i++) {
            std::shared_ptr<api::RemoveCommand> command(new api::RemoveCommand(
                                                                  e.getBucketId(),
                                                                  _msg->getDocumentId(),
                                                                  _msg->getTimestamp()));

            copyMessageSettings(*_msg, *command);
            command->getTrace().setLevel(_msg->getTrace().getLevel());
            command->setCondition(_msg->getCondition());

            messages.push_back(
                    MessageTracker::ToSend(command, e->getNodeRef(i).getNode()));
            sent = true;
        }

        _tracker.queueMessageBatch(messages);
    }

    if (!sent) {
        LOG(debug,
            "Remove document %s failed since no available nodes found. "
            "System state is %s",
            _msg->getDocumentId().toString().c_str(),
            _manager.getClusterState().toString().c_str());

        _tracker.fail(sender, api::ReturnCode(api::ReturnCode::OK));
    } else {
        _tracker.flushQueue(sender);
    }
};


void
RemoveOperation::onReceive(DistributorMessageSender& sender, const std::shared_ptr<api::StorageReply> & msg)
{
    api::RemoveReply& reply(static_cast<api::RemoveReply&>(*msg));

    if (_tracker.getReply().get()) {
        api::RemoveReply& replyToSend =
            static_cast<api::RemoveReply&>(*_tracker.getReply());


        if (reply.getOldTimestamp() > replyToSend.getOldTimestamp()) {
            replyToSend.setOldTimestamp(reply.getOldTimestamp());
        }
    }

    _tracker.receiveReply(sender, reply);
}

void
RemoveOperation::onClose(DistributorMessageSender& sender)
{
    _tracker.fail(sender, api::ReturnCode(api::ReturnCode::ABORTED, "Process is shutting down"));
}
