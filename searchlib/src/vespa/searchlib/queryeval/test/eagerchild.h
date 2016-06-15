// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/searchlib/queryeval/searchiterator.h>

namespace search {
namespace queryeval {
namespace test {

/**
 * Child iterator that has initial docid > 0.
 **/
struct EagerChild : public SearchIterator
{
    EagerChild(uint32_t initial) : SearchIterator() { setDocId(initial); }
    virtual void doSeek(uint32_t) { setAtEnd(); }
    virtual void doUnpack(uint32_t) {}
};

} // namespace test
} // namespace queryeval
} // namespace search

