// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/documentapi/messagebus/messages/documentreply.h>
#include <vespa/document/bucket/bucketid.h>

namespace documentapi {

class GetBucketListReply : public DocumentReply {
public:
    class BucketInfo {
    public:
        document::BucketId _bucket;
        string        _bucketInformation;

        BucketInfo();
        BucketInfo(const document::BucketId &bucketId,
                   const string &bucketInformation);
        bool operator==(const BucketInfo &rhs) const;
    };

private:
    std::vector<BucketInfo> _buckets;

public:
    /**
     * Constructs a new reply with no content.
     */
    GetBucketListReply();

    /**
     * Returns the bucket state contained in this.
     *
     * @return The state object.
     */
    std::vector<BucketInfo> &getBuckets() { return _buckets; }

    /**
     * Returns a const reference to the bucket state contained in this.
     *
     * @return The state object.
     */
    const std::vector<BucketInfo> &getBuckets() const { return _buckets; }

    string toString() const { return "getbucketlistreply"; }
};

inline std::ostream &
operator<<(std::ostream &out, const GetBucketListReply::BucketInfo &info)
{
    out << "BucketInfo(" << info._bucket << ": " << info._bucketInformation << ")";
    return out;
}

} // documentapi

