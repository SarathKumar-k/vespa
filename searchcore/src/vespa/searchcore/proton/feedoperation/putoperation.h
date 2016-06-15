// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "documentoperation.h"

namespace proton {

class PutOperation : public DocumentOperation
{
    document::Document::SP _doc;

public:
    PutOperation();
    PutOperation(const document::BucketId &bucketId,
                 const storage::spi::Timestamp &timestamp,
                 const document::Document::SP &doc);
    PutOperation(const document::BucketId &bucketId,
                 const storage::spi::Timestamp &timestamp,
                 const document::Document::SP &doc,
                 SerialNum serialNum,
                 DbDocumentId dbdId,
                 DbDocumentId prevDbdId);
    virtual ~PutOperation() {}
    const document::Document::SP &getDocument() const { return _doc; }
    void assertValid() const;
    virtual void serialize(vespalib::nbostream &os) const;
    virtual void deserialize(vespalib::nbostream &is,
                             const document::DocumentTypeRepo &repo);
    virtual vespalib::string toString() const;
};

} // namespace proton

