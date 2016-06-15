// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

namespace proton {
namespace flushengine {

/*
 * Class representing statistics for a transaction log server domain used to
 * adjust flush strategy.
 */
class TlsStats
{
    uint64_t _numBytes;
    uint64_t _firstSerial;
    uint64_t _lastSerial;

public:
    TlsStats()
        : _numBytes(0),
          _firstSerial(0),
          _lastSerial(0)
    {
    }
    TlsStats(uint64_t numBytes, uint64_t firstSerial, uint64_t lastSerial)
        : _numBytes(numBytes),
          _firstSerial(firstSerial),
          _lastSerial(lastSerial)
    {
    }

    uint64_t getNumBytes() const    { return _numBytes; }
    uint64_t getFirstSerial() const { return _firstSerial; }
    uint64_t getLastSerial() const  { return _lastSerial; }
};

} // namespace proton::flushengine
} // namespace proton
