// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <memory>
#include <vespa/vespalib/stllike/string.h>
#include <vespa/vespalib/data/slime/slime.h>
#include "value_converter.h"
#include <map>

namespace config {

namespace internal {

template<typename T, typename Converter = config::internal::ValueConverter<T> >
class MapInserter : public ::vespalib::slime::ObjectTraverser {
public:
    MapInserter(std::map<vespalib::string, T> & map);
    void field(const ::vespalib::slime::Memory & symbol, const ::vespalib::slime::Inspector & inspector);
private:
    std::map<vespalib::string, T> & _map;
};

} // namespace internal

} // namespace config

#include "map_inserter.hpp"

