// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/storage/distributor/bucketdb/bucketcopy.h>

namespace storage {

namespace distributor {

void
BucketCopy::print(std::ostream& out, bool /*verbose*/, const std::string&) const
{
    out << "node("
        << "idx=" << _node
        << ",crc=" << std::hex << "0x" << getChecksum() << std::dec
        << ",docs=" << getDocumentCount() << "/" << getMetaCount()
        << ",bytes=" << getTotalDocumentSize() << "/" << getUsedFileSize()
        << ",trusted=" << (trusted() ? "true" : "false")
        << ",active=" << (active() ? "true" : "false")
        << ")";
}

}

}
