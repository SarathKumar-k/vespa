// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "document_db_maintenance_config.h"
#include "i_maintenance_job.h"
#include <persistence/spi/types.h>
// #include <vespa/searchlib/common/idocumentmetastore.h>

namespace proton
{

class IDocumentMetaStore;
class IPruneRemovedDocumentsHandler;
class IFrozenBucketHandler;

/**
 * Job that regularly checks whether old removed documents should be
 * forgotten.
 */
class PruneRemovedDocumentsJob : public IMaintenanceJob
{
private:
    const IDocumentMetaStore      &_metaStore;	// external ownership
    uint32_t                       _subDbId;
    double                         _cfgAgeLimit;
    const vespalib::string        &_docTypeName;
    IPruneRemovedDocumentsHandler &_handler;
    IFrozenBucketHandler          &_frozenHandler;

    typedef uint32_t DocId;
    std::vector<DocId>             _pruneLids;
    DocId                          _nextLid;

    void
    flush(DocId lowLid, DocId nextLowLid, const storage::spi::Timestamp ageLimit);
public:
    using Config = DocumentDBPruneRemovedDocumentsConfig;

    PruneRemovedDocumentsJob(const Config &config,
                             const IDocumentMetaStore &metaStore,
                             uint32_t subDbId,
                             const vespalib::string &docTypeName,
                             IPruneRemovedDocumentsHandler &handler,
                             IFrozenBucketHandler &frozenHandler);

    // Implements IMaintenanceJob
    virtual bool run();
};

} // namespace proton

