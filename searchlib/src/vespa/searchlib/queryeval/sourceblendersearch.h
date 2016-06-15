// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/vespalib/util/array.h>
#include "searchiterator.h"
#include "emptysearch.h"
#include "isourceselector.h"

namespace search {
namespace queryeval {

/**
 * A simple implementation of the source blender operation. This class
 * is used to blend results from multiple sources. Each source is
 * represented with a separate search iterator. A source selector
 * iterator is used to select the appropriate source for each
 * document. The source blender will make sure to only propagate
 * unpack requests to one of the sources below, enabling them to use
 * the same target location for detailed match data unpacking.
 **/
class SourceBlenderSearch : public SearchIterator
{
public:
    /**
     * Small wrapper used to specify the underlying searches to be
     * blended.
     **/
    struct Child {
        SearchIterator *search;
        uint32_t    sourceId;
        Child() : search(NULL), sourceId(0) { }
        Child(SearchIterator *s, uint32_t id) : search(s), sourceId(id) {}
    };
    typedef std::vector<Child> Children;

private:
    SourceBlenderSearch(const SourceBlenderSearch &);
    SourceBlenderSearch &operator=(const SourceBlenderSearch &);
    virtual void visitMembers(vespalib::ObjectVisitor &visitor) const;
    virtual bool isSourceBlender() const { return true; }
    static EmptySearch _emptySearch;
protected:
    typedef std::vector<Source> SourceIndex;
    SearchIterator                * _matchedChild;
    ISourceSelector::Iterator::UP   _sourceSelector;
    SourceIndex                     _children;
    uint32_t                        _docIdLimit;
    SearchIterator                * _sources[256];

    void doSeek(uint32_t docid) override;
    void doUnpack(uint32_t docid) override;
    Trinary is_strict() const override { return Trinary::False; }
    SourceBlenderSearch(ISourceSelector::Iterator::UP sourceSelector, const Children &children);
    SearchIterator * getSearch(Source source) const { return _sources[source]; }
public:
    /**
     * Create a new SourceBlender Search with the given children and
     * strictness. A strict blender can assume that all children below
     * are also strict. A non-strict blender has no strictness
     * assumptions about its children.
     *
     * @param sourceSelector This is an iterator that provide you with the
     *                       the correct source to use.
     * @param children the search objects we are blending
     *        this object takes ownership of the children.
     * @param strict whether this search is strict
     * (a strict search will locate its next hit when seeking fails)
     **/
    static SourceBlenderSearch * create(ISourceSelector::Iterator::UP sourceSelector,
                        const Children &children, bool strict);
    virtual ~SourceBlenderSearch();
    size_t getNumChildren() const { return _children.size(); }
    SearchIterator::UP steal(size_t index) {
        SearchIterator::UP retval(_sources[_children[index]]);
        _sources[_children[index]] = NULL;
        return retval;
    }
    void setChild(size_t index, SearchIterator::UP child) {
        assert(_sources[_children[index]] == NULL);
        _sources[_children[index]] = child.release();
    }
    void initRange(uint32_t beginId, uint32_t endId) override;
    void resetRange() override;
};

} // namespace queryeval
} // namespace search

void visit(vespalib::ObjectVisitor &self, const vespalib::string &name,
           const search::queryeval::SourceBlenderSearch::Child &obj);

