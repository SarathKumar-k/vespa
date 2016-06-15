// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <iosfwd>

namespace search {

/**
 * Represents an address space with number of bytes/entries used
 * and the limit number of bytes/entries this address space can represent.
 */
class AddressSpace
{
private:
    size_t _used;
    size_t _limit;

public:
    AddressSpace(size_t used_, size_t limit_);
    size_t used() const { return _used; }
    size_t limit() const  { return _limit; }
    double usage() const {
        if (_limit > 0) {
            return (double)_used / (double)_limit;
        }
        return 0;
    }
    bool operator==(const AddressSpace &rhs) const {
        return _used == rhs._used && _limit == rhs._limit;
    }
};

std::ostream &operator << (std::ostream &out, const AddressSpace &rhs);

} // namespace search
