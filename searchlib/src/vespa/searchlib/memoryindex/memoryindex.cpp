// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".searchlib.memoryindex.memoryindex");

#include "memoryindex.h"
#include <vespa/searchlib/fef/termfieldmatchdata.h>
#include <vespa/searchlib/index/docidandfeatures.h>
#include <vespa/searchlib/index/indexbuilder.h>
#include <vespa/searchlib/index/schemautil.h>
#include <vespa/searchlib/memoryindex/featurestore.h>
#include <vespa/searchlib/memoryindex/postingiterator.h>
#include <vespa/searchlib/queryeval/create_blueprint_visitor_helper.h>
#include <vespa/searchlib/queryeval/split_float.h>
#include <vespa/searchlib/queryeval/booleanmatchiteratorwrapper.h>
#include <vespa/searchlib/queryeval/emptysearch.h>
#include <vespa/searchlib/queryeval/searchiterator.h>
#include <vespa/searchlib/queryeval/searchable.h>
#include <vespa/searchlib/queryeval/blueprint.h>
#include <vespa/searchlib/queryeval/leaf_blueprints.h>
#include <vespa/searchlib/queryeval/termasstring.h>
#include <vespa/searchlib/query/tree/simplequery.h>
#include <vespa/searchlib/common/sequencedtaskexecutor.h>

#include <algorithm>

#include <vespa/searchlib/btree/btreenode.hpp>
#include <vespa/searchlib/btree/btreenodeallocator.hpp>
#include <vespa/searchlib/btree/btreeroot.hpp>

using document::ArrayFieldValue;
using document::WeightedSetFieldValue;
using vespalib::LockGuard;
using vespalib::GenerationHandler;

namespace search {

using fef::TermFieldMatchDataArray;
using index::IndexBuilder;
using index::Schema;
using index::SchemaUtil;
using query::NumberTerm;
using query::LocationTerm;
using query::Node;
using query::PredicateQuery;
using query::PrefixTerm;
using query::RangeTerm;
using query::RegExpTerm;
using query::StringTerm;
using query::SubstringTerm;
using query::SuffixTerm;
using queryeval::SearchIterator;
using queryeval::Searchable;
using queryeval::CreateBlueprintVisitorHelper;
using queryeval::Blueprint;
using queryeval::BooleanMatchIteratorWrapper;
using queryeval::EmptyBlueprint;
using queryeval::FieldSpecBase;
using queryeval::FieldSpecBaseList;
using queryeval::FieldSpec;
using queryeval::IRequestContext;

namespace memoryindex {

MemoryIndex::MemoryIndex(const Schema &schema,
                         ISequencedTaskExecutor &invertThreads,
                         ISequencedTaskExecutor &pushThreads)
    : _schema(schema),
      _invertThreads(invertThreads),
      _pushThreads(pushThreads),
      _inverter0(_schema, _invertThreads, _pushThreads),
      _inverter1(_schema, _invertThreads, _pushThreads),
      _inverter(&_inverter0),
      _dictionary(_schema),
      _frozen(false),
      _maxDocId(0), // docId 0 is reserved
      _numDocs(0),
      _lock(),
      _hiddenFields(schema.getNumIndexFields(), false),
      _wipeTimeSchema(),
      _indexedDocs(0),
      _staticMemoryFootprint(getMemoryUsage().allocatedBytes())
{
}

MemoryIndex::~MemoryIndex()
{
    _invertThreads.sync();
    _pushThreads.sync();
}

void
MemoryIndex::insertDocument(uint32_t docId, const document::Document &doc)
{
    if (_frozen) {
        LOG(warning, "Memory index frozen: ignoring insert of document '%s'(%u): '%s'",
            doc.getId().toString().c_str(), docId, doc.toString().c_str());
        return;
    }
    updateMaxDocId(docId);
    _inverter->invertDocument(docId, doc);
    if (_indexedDocs.insert(docId).second) {
        incNumDocs();
    }
}

void
MemoryIndex::removeDocument(uint32_t docId)
{
    if (_frozen) {
        LOG(warning, "Memory index frozen: ignoring remove of document (%u)",
            docId);
        return;
    }
    _inverter->removeDocument(docId);
    if (_indexedDocs.find(docId) != _indexedDocs.end()) {
        _indexedDocs.erase(docId);
        decNumDocs();
    }
}

void
MemoryIndex::commit(const std::shared_ptr<IDestructorCallback> &onWriteDone)
{
    _invertThreads.sync(); // drain inverting into this inverter
    _pushThreads.sync(); // drain use of other inverter
    _inverter->pushDocuments(_dictionary, onWriteDone);
    flipInverter();
}


void
MemoryIndex::flipInverter()
{
    if (_inverter != &_inverter0) {
        _inverter = &_inverter0;
    } else {
        _inverter = &_inverter1;
    }
}

void
MemoryIndex::freeze()
{
    _frozen = true;
}

void
MemoryIndex::dump(IndexBuilder &indexBuilder)
{
    _dictionary.dump(indexBuilder);
}

namespace {

class MemTermBlueprint : public queryeval::SimpleLeafBlueprint
{
private:
    GenerationHandler::Guard               _genGuard;
    Dictionary::PostingList::ConstIterator _pitr;
    const FeatureStore                    &_featureStore;
    const uint32_t                         _fieldId;
    const bool                             _useBitVector;

public:
    MemTermBlueprint(GenerationHandler::Guard &&genGuard,
                     Dictionary::PostingList::ConstIterator pitr,
                     const FeatureStore &featureStore,
                     const FieldSpecBase &field,
                     uint32_t fieldId,
                     bool useBitVector)
        : SimpleLeafBlueprint(field),
          _genGuard(),
          _pitr(pitr),
          _featureStore(featureStore),
          _fieldId(fieldId),
          _useBitVector(useBitVector)
    {
        _genGuard = std::move(genGuard);
        HitEstimate estimate(_pitr.size(), !_pitr.valid());
        setEstimate(estimate);
    }

    virtual SearchIterator::UP
    createLeafSearch(const TermFieldMatchDataArray &tfmda, bool) const
    {
        SearchIterator::UP search(new PostingIterator(_pitr, _featureStore, _fieldId, tfmda));
        if (_useBitVector) {
            LOG(debug, "Return BooleanMatchIteratorWrapper: fieldId(%u), docCount(%zu)",
                _fieldId, _pitr.size());
            return SearchIterator::UP(new BooleanMatchIteratorWrapper(std::move(search), tfmda));
        }
        LOG(debug, "Return PostingIterator: fieldId(%u), docCount(%zu)",
            _fieldId, _pitr.size());
        return search;
    }

};

/**
 * Determines the correct Blueprint to use.
 **/
class CreateBlueprintVisitor : public CreateBlueprintVisitorHelper
{
private:
    const FieldSpec &_field;
    const uint32_t   _fieldId;
    Dictionary &     _dictionary;

public:
    CreateBlueprintVisitor(Searchable &searchable,
                           const IRequestContext & requestContext,
                           const FieldSpec &field,
                           uint32_t fieldId,
                           Dictionary &dictionary)
        : CreateBlueprintVisitorHelper(searchable, field, requestContext),
          _field(field),
          _fieldId(fieldId),
          _dictionary(dictionary) {}

    template <class TermNode>
    void visitTerm(TermNode &n) {
        const vespalib::string termStr = queryeval::termAsString(n);
        LOG(debug, "searching for '%s' in '%s'",
            termStr.c_str(), _field.getName().c_str());
        MemoryFieldIndex *fieldIndex = _dictionary.getFieldIndex(_fieldId);
        GenerationHandler::Guard genGuard = fieldIndex->takeGenerationGuard();
        Dictionary::PostingList::ConstIterator pitr
            = fieldIndex->findFrozen(termStr);
        bool useBitVector = _field.isFilter();
        setResult(make_UP(new MemTermBlueprint(std::move(genGuard), pitr,
                                               fieldIndex->getFeatureStore(),
                                              _field, _fieldId, useBitVector)));
    }

    virtual void visit(LocationTerm &n)  { visitTerm(n); }
    virtual void visit(PrefixTerm &n)    { visitTerm(n); }
    virtual void visit(RangeTerm &n)     { visitTerm(n); }
    virtual void visit(StringTerm &n)    { visitTerm(n); }
    virtual void visit(SubstringTerm &n) { visitTerm(n); }
    virtual void visit(SuffixTerm &n)    { visitTerm(n); }
    virtual void visit(RegExpTerm &n)    { visitTerm(n); }
    virtual void visit(PredicateQuery &) { }

    virtual void visit(NumberTerm &n) {
        handleNumberTermAsText(n);
    }

};

} // namespace search::memoryindex::<unnamed>

Blueprint::UP
MemoryIndex::createBlueprint(const IRequestContext & requestContext,
                             const FieldSpec &field,
                             const Node &term)
{
    uint32_t fieldId = _schema.getIndexFieldId(field.getName());
    if (fieldId == Schema::UNKNOWN_FIELD_ID || _hiddenFields[fieldId]) {
        return Blueprint::UP(new EmptyBlueprint(field));
    }
    CreateBlueprintVisitor visitor(*this, requestContext, field, fieldId, _dictionary);
    const_cast<Node &>(term).accept(visitor);
    return visitor.getResult();
}

MemoryUsage
MemoryIndex::getMemoryUsage() const
{
    MemoryUsage usage;
    usage.merge(_dictionary.getMemoryUsage());
    return usage;
}

void
MemoryIndex::wipeHistory(const Schema &schema)
{
    LockGuard lock(_lock);
    if (_wipeTimeSchema.get() == NULL) {
        Schema::UP newSchema = Schema::intersect(_schema, schema);
        if (_schema == *newSchema)
            return;
        _wipeTimeSchema.reset(newSchema.release());
    } else {
        Schema::UP newSchema = Schema::intersect(*_wipeTimeSchema, schema);
        if (*_wipeTimeSchema == *newSchema)
            return;
        _wipeTimeSchema.reset(newSchema.release());
    }
    SchemaUtil::IndexIterator i(_schema);
    for (; i.isValid(); ++i) {
        uint32_t packedIndex = i.getIndex();
        assert(packedIndex < _hiddenFields.size());
        SchemaUtil::IndexIterator wi(*_wipeTimeSchema, i);
        _hiddenFields[packedIndex] = !wi.isValid();
    }
}

Schema::SP
MemoryIndex::getWipeTimeSchema() const
{
    LockGuard lock(_lock);
    return _wipeTimeSchema;
}

} // namespace memoryindex
} // namespace search
