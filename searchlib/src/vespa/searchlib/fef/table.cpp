// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include "table.h"
#include <limits>

namespace search {
namespace fef {

Table::Table() :
    _table(),
    _max(-std::numeric_limits<double>::max())
{
    _table.reserve(256);
}

Table::~Table()
{
}

} // namespace fef
} // namespace search
