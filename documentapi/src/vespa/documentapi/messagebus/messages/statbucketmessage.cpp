// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/documentapi/messagebus/documentprotocol.h>
#include <vespa/documentapi/messagebus/messages/statbucketmessage.h>
#include <vespa/documentapi/messagebus/messages/statbucketreply.h>

namespace documentapi {

StatBucketMessage::StatBucketMessage() :
    DocumentMessage(),
    _bucket(),
    _documentSelection()
{
    // empty
}

StatBucketMessage::StatBucketMessage(document::BucketId bucket, const string& documentSelection) :
    DocumentMessage(),
    _bucket(bucket),
    _documentSelection(documentSelection)
{
    // empty
}

DocumentReply::UP
StatBucketMessage::doCreateReply() const
{
    return DocumentReply::UP(new StatBucketReply());
}

bool
StatBucketMessage::hasSequenceId() const
{
    return true;
}

uint64_t
StatBucketMessage::getSequenceId() const
{
    return _bucket.getRawId();
}

uint32_t
StatBucketMessage::getType() const
{
    return DocumentProtocol::MESSAGE_STATBUCKET;
}

}
