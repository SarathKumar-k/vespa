// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/storage/distributor/operations/operation.h>
#include <vespa/storage/distributor/bucketdb/bucketdatabase.h>
#include <vespa/vespalib/util/sync.h>

namespace storage {

namespace api {
class GetBucketListCommand;
}

namespace distributor {

class MaintenanceOperationGenerator;

class StatBucketListOperation : public Operation
{
public:
    StatBucketListOperation(
            const BucketDatabase& bucketDb,
            const MaintenanceOperationGenerator& generator,
            uint16_t distributorIndex,
            const std::shared_ptr<api::GetBucketListCommand>& cmd);
    virtual ~StatBucketListOperation() {}

    virtual const char* getName() const { return "statBucketList"; }
    virtual std::string getStatus() const { return ""; }

    virtual void onStart(DistributorMessageSender& sender);
    virtual void onReceive(DistributorMessageSender&,
                           const std::shared_ptr<api::StorageReply>&)
    {
        // Never called.
        assert(false);
    }
    void onClose(DistributorMessageSender&) {
    }

private:
    void getBucketStatus(const BucketDatabase::Entry& entry,
                         std::ostream& os) const;

    const BucketDatabase& _bucketDb;
    const MaintenanceOperationGenerator& _generator;
    uint16_t _distributorIndex;
    std::shared_ptr<api::GetBucketListCommand> _command;
};

} // distributor
} // storage

