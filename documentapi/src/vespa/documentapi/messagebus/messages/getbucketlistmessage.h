// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/document/bucket/bucketid.h>
#include <vespa/documentapi/messagebus/messages/documentmessage.h>

namespace documentapi {

class GetBucketListMessage : public DocumentMessage {
private:
    document::BucketId _bucketId;

protected:
    // Implements DocumentMessage.
    DocumentReply::UP doCreateReply() const;

public:
    /**
     * Constructs a new message for deserialization.
     */
    GetBucketListMessage();

    /**
     * Constructs a new message with initial content.
     *
     * @param bucketId The bucket whose list to retrieve.
     */
    GetBucketListMessage(const document::BucketId &bucketId);

    /**
     * Returns the bucket whose list to retrieve.
     *
     * @return The bucket id.
     */
    const document::BucketId& getBucketId() const { return _bucketId; }

    /**
     * Sets the bucket whose list to retrieve.
     *
     * @param id The bucket id to set.
     */
    void setBucketId(const document::BucketId& id) { _bucketId = id; }

    // Overrides DocumentMessage.
    bool hasSequenceId() const;

    // Overrides DocumentMessage.
    uint64_t getSequenceId() const;

    // Implements DocumentMessage.
    uint32_t getType() const;

    string toString() const { return "getbucketlistmessage"; }
};

}

