// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
// Copyright (C) 1998-2003 Fast Search & Transfer ASA
// Copyright (C) 2003 Overture Services Norway AS

#pragma once

#include <vespa/fastos/fastos.h>
#include <utility>

#include "docsumstorevalue.h"

namespace search {
namespace docsummary {


/**
 * Interface for object able to fetch docsum blobs based on local
 * document id.
 **/
class IDocsumStore
{
public:
    /**
     * Convenience typedef.
     */
    typedef std::unique_ptr<IDocsumStore> UP;

    /**
     * Destructor.  No cleanup needed for base class.
     */
    virtual ~IDocsumStore(void) { }

    /**
     * @return total number of documents.
     **/
    virtual uint32_t getNumDocs() = 0;

    /**
     * Get a reference to a docsum blob in memory.  The docsum store
     * owns the memory (which is either mmap()ed or from a memory-based
     * index of some kind).
     *
     * @return docsum blob location and size
     * @param docid local document id
     * @param useSlimeInsideFields use serialized slime instead of json for structured fields
     **/
    virtual DocsumStoreValue getMappedDocsum(uint32_t docid, bool useSlimeInsideFields) = 0;

    /**
     * Will return default input class used.
     **/
    virtual uint32_t getSummaryClassId() const = 0;
};

}
}

