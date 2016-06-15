// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/searchlib/attribute/multinumericenumattribute.h>
#include "postinglistattribute.h"
#include "i_document_weight_attribute.h"

namespace search {

/*
 * Implementation of multi value numeric attribute that in addition to enum store and
 * multi value mapping uses an underlying posting list to provide faster search.
 * This class is used for both array and weighted set types.
 *
 * B: EnumAttribute<P, BaseClass>
 * M: multivalue::Value<EnumStoreBase::Index> (array) or
 *    multivalue::WeightedValue<EnumStoreBase::Index> (weighted set)
 * M specifies the type stored in the MultiValueMapping
 */
template <typename B, typename M>
class MultiValueNumericPostingAttribute
    : public MultiValueNumericEnumAttribute<B, M>,
      protected PostingListAttributeSubBase<AttributeWeightPosting,
                                            typename B::LoadedVector,
                                            typename B::LoadedValueType,
                                            typename B::EnumStore>
{
private:
    struct DocumentWeightAttributeAdapter : IDocumentWeightAttribute
    {
        const MultiValueNumericPostingAttribute &self;
        DocumentWeightAttributeAdapter(const MultiValueNumericPostingAttribute &self_in) : self(self_in) {}
        virtual LookupResult lookup(const vespalib::string &term) const override final;
        virtual void create(btree::EntryRef idx, std::vector<DocumentWeightIterator> &dst) const override final;
        virtual DocumentWeightIterator create(btree::EntryRef idx) const override final;
    };
    DocumentWeightAttributeAdapter _document_weight_attribute_adapter;

    friend class PostingListAttributeTest;
    template <typename, typename, typename> 
    friend class attribute::PostingSearchContext; // getEnumStore()
    typedef MultiValueNumericPostingAttribute<B, M> SelfType;
public:
    typedef typename B::EnumStore  EnumStore;
    typedef typename EnumStore::Index  EnumIndex;
private:
    typedef typename B::DocId DocId;
    typedef typename B::LoadedVector    LoadedVector;
    typedef attribute::LoadedEnumAttributeVector LoadedEnumAttributeVector;
    typedef PostingListAttributeSubBase<AttributeWeightPosting,
                                        LoadedVector,
                                        typename B::LoadedValueType,
                                        EnumStore> PostingParent;
    typedef typename PostingParent::PostingList PostingList;
    typedef typename PostingParent::PostingMap  PostingMap;
    typedef typename PostingParent::Posting     Posting;
    typedef EnumPostingTree Dictionary;
    typedef typename Dictionary::Iterator DictionaryIterator;
    typedef typename Dictionary::ConstIterator DictionaryConstIterator;
    typedef typename Dictionary::FrozenView FrozenDictionary;
    typedef typename EnumStore::ComparatorType ComparatorType;

    typedef typename MultiValueNumericEnumAttribute<B, M>::DocIndices    DocIndices;
    typedef typename MultiValueNumericEnumAttribute<B, M>::generation_t  generation_t;
    typedef typename MultiValueNumericEnumAttribute<B, M>::WeightedIndex WeightedIndex;

    typedef typename MultiValueNumericEnumAttribute<B, M>::ArraySearchContext ArraySearchContext;
    typedef typename MultiValueNumericEnumAttribute<B, M>::SetSearchContext   SetSearchContext;
    typedef ArraySearchContext       ArrayNumericSearchContext;
    typedef SetSearchContext         SetNumericSearchContext;
    typedef attribute::NumericPostingSearchContext<ArrayNumericSearchContext,
                                                   SelfType,
                                                   int32_t>
    ArrayPostingSearchContext;
    typedef attribute::NumericPostingSearchContext<SetNumericSearchContext,
                                                   SelfType,
                                                   int32_t>
    SetPostingSearchContext;
    using PostingParent::_postingList;
    using PostingParent::clearAllPostings;
    using PostingParent::handleFillPostings;
    using PostingParent::fillPostingsFixupEnumBase;
    using PostingParent::forwardedOnAddDoc;

    virtual void freezeEnumDictionary();
    virtual void mergeMemoryStats(MemoryUsage & total);
    virtual void applyValueChanges(const DocIndices & docIndices, EnumStoreBase::IndexVector & unused);

public:
    MultiValueNumericPostingAttribute(const vespalib::string & name, const AttributeVector::Config & cfg);

    virtual
    ~MultiValueNumericPostingAttribute();

    virtual void removeOldGenerations(generation_t firstUsed);
    virtual void onGenerationChange(generation_t generation);

    AttributeVector::SearchContext::UP
    getSearch(QueryTermSimple::UP term, const AttributeVector::SearchContext::Params & params) const override;

    virtual const IDocumentWeightAttribute *asDocumentWeightAttribute() const override;

    virtual bool
    onAddDoc(DocId doc)
    {
        return forwardedOnAddDoc(doc,
                                 this->_mvMapping.getNumKeys(),
                                 this->_mvMapping.getCapacityKeys());
    }
    
    virtual void
    fillPostings(LoadedVector & loaded)
    {
        handleFillPostings(loaded);
    }

    virtual attribute::IPostingListAttributeBase *
    getIPostingListAttributeBase(void)
    {
        return this;
    }

    virtual void
    fillPostingsFixupEnum(const LoadedEnumAttributeVector &loaded)
    {
        fillPostingsFixupEnumBase(loaded);
    }
};


} // namespace search

