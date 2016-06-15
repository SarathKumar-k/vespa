// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/persistence/spi/bucketinfo.h>

namespace storage {
namespace spi {

BucketInfo::BucketInfo()
    : _checksum(0),
      _documentCount(0),
      _documentSize(0),
      _entryCount(0),
      _size(0),
      _ready(NOT_READY),
      _active(NOT_ACTIVE)
{
}

BucketInfo::BucketInfo(BucketChecksum checksum,
                       uint32_t docCount,
                       uint32_t docSize,
                       uint32_t metaEntryCount,
                       uint32_t size,
                       ReadyState ready,
                       ActiveState active)
    : _checksum(checksum),
      _documentCount(docCount),
      _documentSize(docSize),
      _entryCount(metaEntryCount),
      _size(size),
      _ready(ready),
      _active(active)
{
}

bool
BucketInfo::operator==(const BucketInfo& o) const
{
    return (_checksum == o._checksum
            && _documentCount == o._documentCount
            && _documentSize == o._documentSize
            && _entryCount == o._entryCount
            && _size == o._size
            && _ready == o._ready
            && _active == o._active);
}

void
BucketInfo::print(std::ostream& out) const
{
    out << "BucketInfo(";
    out << "crc 0x" << std::hex << _checksum << std::dec
        << ", documentCount " << _documentCount;
    if (_documentSize != 0) {
        out << ", documentSize " << _documentSize;
    }
    out << ", entryCount " << _entryCount;
    if (_size != 0) {
        out << ", usedSize " << _size;
    }
    out << ", ready "  << (_ready ? "true" : "false")
        << ", active " << (_active ? "true" : "false");
    out << ")";
}

std::string
BucketInfo::toString() const {
    std::ostringstream ost;
    print(ost);
    return ost.str();
}

} // spi
} // storage
