// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/vespalib/util/array.h>
#include <vespa/vespalib/stllike/string.h>

namespace search
{

class BitVector;

namespace diskindex
{

typedef vespalib::Array<uint8_t, vespalib::DefaultAlloc> SelectorArray;

class DocIdMapping
{
public:
    uint32_t _docIdLimit;
    const SelectorArray *_selector;	// External ownership
    uint8_t _selectorId;

    DocIdMapping(void);

    void
    clear(void);

    void
    setup(uint32_t docIdLimit);

    void
    setup(uint32_t docIdLimit,
          const SelectorArray *selector,
          uint8_t selectorId);

    bool
    readDocIdLimit(const vespalib::string &dir);
};


class DocIdMapper
{
public:
    const uint8_t *_selector;
    uint32_t _docIdLimit; // Limit on legal input values
    uint32_t _selectorLimit; // Limit on output
    uint8_t _selectorId;

    DocIdMapper(void)
        : _selector(NULL),
          _docIdLimit(0u),
          _selectorLimit(0),
          _selectorId(0u)
    {
    }

    void
    setup(const DocIdMapping &mapping)
    {
        _selector = (mapping._selector != NULL) ?
                    &((*mapping._selector)[0]) : NULL;
        _docIdLimit = mapping._docIdLimit;
        _selectorLimit = (mapping._selector != NULL) ?
                         (*mapping._selector).size() :
                         0u;
        _selectorId = mapping._selectorId;
    }

    static uint32_t
    noDocId(void)
    {
        return static_cast<uint32_t>(-1);
    }

    uint32_t
    mapDocId(uint32_t docId) const
    {
        assert(docId < _docIdLimit);
        if (_selector != NULL &&
            (docId >= _selectorLimit || _selector[docId] != _selectorId)) {
            docId = noDocId();
        }
        return docId;
    }
};

} // namespace diskindex

} // namespace search

