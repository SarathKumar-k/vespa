// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/document/fieldvalue/document.h>
#include <persistence/spi/types.h>
#include <vespa/persistence/spi/bucket.h>
#include <vespa/persistence/spi/read_consistency.h>
#include <vespa/searchlib/common/idocumentmetastore.h>
#include <vespa/searchlib/docstore/idocumentstore.h>
#include <vespa/searchcore/proton/common/cachedselect.h>
#include <vespa/searchlib/query/base.h>
#include <memory>

namespace proton
{

/**
 * This is an interface that allows retrieval of documents by local id and document metadata
 * by either bucket or document id.
 * It also provides a callback interface known in VDS as visitation.
 **/ 
class IDocumentRetriever
{
public:
    using ReadConsistency = storage::spi::ReadConsistency;
    typedef std::unique_ptr<IDocumentRetriever> UP;
    typedef std::shared_ptr<IDocumentRetriever> SP;

    typedef search::IDocumentStore::LidVector LidVector;
    virtual ~IDocumentRetriever() {}

    virtual const document::DocumentTypeRepo &getDocumentTypeRepo() const = 0;
    virtual void getBucketMetaData(const storage::spi::Bucket &bucket, search::DocumentMetaData::Vector &result) const = 0;
    virtual search::DocumentMetaData getDocumentMetaData(const document::DocumentId &id) const = 0;
    virtual document::Document::UP getDocument(search::DocumentIdT lid) const = 0;
    /**
     * Will visit all documents in the the given list. Visit order is undefined and will
     * be conducted in most efficient retrieval order.
     * @param lids to visit
     * @param Visitor to receive callback for each document found.
     */
    virtual void visitDocuments(const LidVector &lids, search::IDocumentVisitor &visitor, ReadConsistency readConsistency) const = 0;

    virtual CachedSelect::SP parseSelect(const vespalib::string &selection) const = 0;
};

class DocumentRetrieverBaseForTest : public IDocumentRetriever {
public:
    virtual void visitDocuments(const LidVector &lids, search::IDocumentVisitor &visitor, ReadConsistency readConsistency) const override;
};

} // namespace proton

