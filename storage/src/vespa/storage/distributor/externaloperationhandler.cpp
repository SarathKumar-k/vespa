// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/storage/distributor/externaloperationhandler.h>

#include <vespa/document/bucket/bucketidfactory.h>
#include <vespa/document/base/documentid.h>
#include <vespa/log/log.h>
#include <vespa/storage/config/config-stor-server.h>
#include <vespa/storage/storageutil/log.h>
#include <vespa/storage/distributor/operations/external/putoperation.h>
#include <vespa/storage/distributor/operations/external/twophaseupdateoperation.h>
#include <vespa/storage/distributor/operations/external/updateoperation.h>
#include <vespa/storage/distributor/operations/external/removeoperation.h>
#include <vespa/storage/distributor/operations/external/getoperation.h>
#include <vespa/storage/distributor/operations/external/multioperationoperation.h>
#include <vespa/storage/distributor/operations/external/statbucketoperation.h>
#include <vespa/storage/distributor/operations/external/statbucketlistoperation.h>
#include <vespa/storage/distributor/operations/external/removelocationoperation.h>
#include <vespa/storage/distributor/operations/external/visitoroperation.h>
#include <vespa/document/util/stringutil.h>
#include <vespa/storageapi/message/persistence.h>
#include <vespa/storageapi/message/state.h>
#include <vespa/storageapi/message/multioperation.h>
#include <vespa/storageapi/message/removelocation.h>
#include <vespa/storageapi/message/batch.h>
#include <vespa/storage/storageserver/storagemetricsset.h>
#include <vespa/storage/distributor/distributormetricsset.h>
#include <vespa/storageapi/message/bucket.h>
#include <vespa/storageapi/message/stat.h>
#include <vespa/vdslib/distribution/distribution.h>
#include <vespa/storage/distributor/distributor.h>
#include <vespa/storage/distributor/distributormetricsset.h>

LOG_SETUP(".distributor.manager");

namespace storage {
namespace distributor {

ExternalOperationHandler::ExternalOperationHandler(
        Distributor& owner,
        const MaintenanceOperationGenerator& gen,
        DistributorComponentRegister& compReg)
    : DistributorComponent(owner, compReg, "Distributor manager"),
      _visitorMetrics(getLoadTypes()->getMetricLoadTypes(),
                      *&VisitorMetricSet(NULL)),
      _operationGenerator(gen)
{
}

ExternalOperationHandler::~ExternalOperationHandler()
{
}

bool
ExternalOperationHandler::handleMessage(
        const std::shared_ptr<api::StorageMessage>& msg,
        Operation::SP& op)
{
    _op = Operation::SP();
    bool retVal = msg->callHandler(*this, msg);
    op = _op;
    return retVal;
}

IMPL_MSG_COMMAND_H(ExternalOperationHandler, Put)
{
    if (!checkDistribution(*cmd, getBucketId(cmd->getDocumentId()))) {
        LOG(debug,
            "Distributor manager received put for %s, bucket %s with wrong "
            "distribution",
            cmd->getDocumentId().toString().c_str(),
            getBucketId(cmd->getDocumentId()).toString().c_str());

        getMetrics().puts[cmd->getLoadType()].failures.wrongdistributor++;
        return true;
    }

    if (cmd->getTimestamp() == 0) {
        cmd->setTimestamp(getUniqueTimestamp());
    }

    _op = Operation::SP(new PutOperation(*this,
                                        cmd,
                                        getMetrics().puts[cmd->getLoadType()]));

    return true;
}


IMPL_MSG_COMMAND_H(ExternalOperationHandler, Update)
{
    if (!checkDistribution(*cmd, getBucketId(cmd->getDocumentId()))) {
        LOG(debug, "Distributor manager received update for %s, bucket %s with wrong distribution", cmd->getDocumentId().toString().c_str(), getBucketId(cmd->getDocumentId()).toString().c_str());

        getMetrics().updates[cmd->getLoadType()].failures.wrongdistributor++;
        return true;
    }

    if (cmd->getTimestamp() == 0) {
        cmd->setTimestamp(getUniqueTimestamp());
    }
    _op = Operation::SP(new TwoPhaseUpdateOperation(*this, cmd, getMetrics()));
    return true;
}


IMPL_MSG_COMMAND_H(ExternalOperationHandler, Remove)
{
    if (!checkDistribution(*cmd, getBucketId(cmd->getDocumentId()))) {
        LOG(debug,
            "Distributor manager received remove for %s, bucket %s with "
            "wrong distribution",
            cmd->getDocumentId().toString().c_str(),
            getBucketId(cmd->getDocumentId()).toString().c_str());

        getMetrics().removes[cmd->getLoadType()].failures.wrongdistributor++;
        return true;
    }

    if (cmd->getTimestamp() == 0) {
        cmd->setTimestamp(getUniqueTimestamp());
    }
    _op = Operation::SP(new RemoveOperation(
                                *this,
                                cmd,
                                getMetrics().removes[cmd->getLoadType()]));
    return true;
}

IMPL_MSG_COMMAND_H(ExternalOperationHandler, RemoveLocation)
{
    document::BucketId bid;
    RemoveLocationOperation::getBucketId(*this, *cmd, bid);

    if (!checkDistribution(*cmd, bid)) {
        LOG(debug,
            "Distributor manager received %s with wrong distribution",
            cmd->toString().c_str());

        getMetrics().removelocations[cmd->getLoadType()].
            failures.wrongdistributor++;
        return true;
    }

    _op = Operation::SP(new RemoveLocationOperation(
                                *this,
                                cmd,
                                getMetrics().removelocations[cmd->getLoadType()]));
    return true;
}

IMPL_MSG_COMMAND_H(ExternalOperationHandler, Get)
{
    if (!checkDistribution(*cmd, getBucketId(cmd->getDocumentId()))) {
        LOG(debug,
            "Distributor manager received get for %s, "
            "bucket %s with wrong distribution",
            cmd->getDocumentId().toString().c_str(),
            getBucketId(cmd->getDocumentId()).toString().c_str());

        getMetrics().gets[cmd->getLoadType()].failures.wrongdistributor++;
        return true;
    }

    _op = Operation::SP(new GetOperation(
                                *this,
                                cmd,
                                getMetrics().gets[cmd->getLoadType()]));
    return true;
}

IMPL_MSG_COMMAND_H(ExternalOperationHandler, MultiOperation)
{
    if (!checkDistribution(*cmd, cmd->getBucketId())) {
        LOG(debug,
            "Distributor manager received multi-operation message, "
            "bucket %s with wrong distribution",
            cmd->getBucketId().toString().c_str());
        return true;
    }

    _op = Operation::SP(new MultiOperationOperation(
                                *this,
                                cmd,
                                getMetrics().multioperations[cmd->getLoadType()]));
    return true;
}

IMPL_MSG_COMMAND_H(ExternalOperationHandler, StatBucket)
{
    if (!checkDistribution(*cmd, cmd->getBucketId())) {
        return true;
    }

    _op = Operation::SP(new StatBucketOperation(*this, cmd));
    return true;
}

IMPL_MSG_COMMAND_H(ExternalOperationHandler, GetBucketList)
{
    if (!checkDistribution(*cmd, cmd->getBucketId())) {
        return true;
    }
    _op = Operation::SP(new StatBucketListOperation(
            getBucketDatabase(), _operationGenerator, getIndex(), cmd));
    return true;
}

IMPL_MSG_COMMAND_H(ExternalOperationHandler, CreateVisitor)
{
    const DistributorConfiguration& config(getDistributor().getConfig());
    VisitorOperation::Config visitorConfig(
            framework::MilliSecTime(config.getMinTimeLeftToResend()),
            config.getMinBucketsPerVisitor(),
            config.getMaxVisitorsPerNodePerClientVisitor());
    _op = Operation::SP(new VisitorOperation(
                                *this,
                                cmd,
                                visitorConfig,
                                &_visitorMetrics[cmd->getLoadType()]));
    return true;
}

} // distributor
} // storage

