// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/searchcorespi/index/indexsearchable.h>
#include <vespa/searchcommon/common/schema.h>
#include <vespa/vespalib/stllike/string.h>

namespace searchcorespi {
namespace index {

/**
 * Interface for a disk index as seen from an index maintainer.
 */
struct IDiskIndex : public IndexSearchable {
    typedef std::shared_ptr<IDiskIndex> SP;
    virtual ~IDiskIndex() {}

    /**
     * Returns the directory in which this disk index exists.
     */
    virtual const vespalib::string &getIndexDir() const = 0;

    /**
     * Returns the schema used by this disk index.
     * Note that the schema should be part of the index on disk.
     */
    virtual const search::index::Schema &getSchema() const = 0;
};

} // namespace index
} // namespace searchcorespi


