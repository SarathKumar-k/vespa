// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include "emptysearch.h"

namespace search {
namespace queryeval {

void
EmptySearch::doSeek(uint32_t)
{
}

void
EmptySearch::doUnpack(uint32_t)
{
}

EmptySearch::EmptySearch()
    : SearchIterator()
{
}

EmptySearch::~EmptySearch()
{
}

} // namespace queryeval
} // namespace search
