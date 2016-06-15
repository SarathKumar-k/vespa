// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include "truesearch.h"

namespace search {
namespace queryeval {

void
TrueSearch::doSeek(uint32_t docid)
{
    setDocId(docid);
}

void
TrueSearch::doUnpack(uint32_t docid)
{
    _tfmd.resetOnlyDocId(docid);
}

TrueSearch::TrueSearch(fef::TermFieldMatchData & tfmd) :
    SearchIterator(),
    _tfmd(tfmd)
{
    _tfmd.resetOnlyDocId(0);
}

TrueSearch::~TrueSearch()
{
}

} // namespace queryeval
} // namespace search
