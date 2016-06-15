// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/vsm/searcher/utf8suffixstringfieldsearcher.h>

using search::byte;
using search::QueryTerm;
using search::QueryTermList;

namespace vsm
{

IMPLEMENT_DUPLICATE(UTF8SuffixStringFieldSearcher);

size_t
UTF8SuffixStringFieldSearcher::matchTerms(const FieldRef & f, const size_t mintsz)
{
    (void) mintsz;
    termcount_t words = 0;
    const byte * srcbuf = reinterpret_cast<const byte *> (f.c_str());
    const byte * srcend = srcbuf + f.size();
    if (f.size() >= _buf->size()) {
        _buf->reserve(f.size() + 1);
    }
    cmptype_t * dstbuf = &(*_buf.get())[0];
    size_t tokenlen = 0;

    for( ; srcbuf < srcend; ) {
        if (*srcbuf == 0) {
            ++_zeroCount;
            ++srcbuf;
        }
        srcbuf = tokenize(srcbuf, _buf->capacity(), dstbuf, tokenlen);
        for (QueryTermList::iterator it = _qtl.begin(), mt = _qtl.end(); it != mt; ++it) {
            QueryTerm & qt = **it;
            const cmptype_t * term;
            termsize_t tsz = qt.term(term);
            if (matchTermSuffix(term, tsz, dstbuf, tokenlen)) {
                addHit(qt, words);
            }
        }
        words++;
    }
    return words;
}

size_t
UTF8SuffixStringFieldSearcher::matchTerm(const FieldRef & f, QueryTerm & qt)
{
    return matchTermSuffix(f, qt);
}

}
