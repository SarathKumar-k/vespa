// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <map>
#include <vespa/vespalib/stllike/string.h>
#include <vector>
#include "feature.h"

namespace search {

/**
 * This class holds information about a set of features for a set of
 * documents.
 **/
class FeatureSet
{
public:
    typedef vespalib::string string;
    typedef std::vector<string> StringVector;
private:
    StringVector _names;
    std::vector<uint32_t>    _docIds;
    std::vector<feature_t>   _values;

    FeatureSet(const FeatureSet &);
    FeatureSet & operator=(const FeatureSet &);

public:
    /**
     * Convenience typedef for a shared pointer to an object of this
     * class.
     **/
    typedef std::shared_ptr<FeatureSet> SP;

    /**
     * Create a new object without any feature information.
     **/
    FeatureSet();

    /**
     * Create a new object that will contain information about the
     * given features.
     *
     * @param names names of all features
     * @param expectDocs the number of documents we expect to store information about
     **/
    FeatureSet(const StringVector &names, uint32_t expectDocs);

    /**
     * Check whether this object is equal to the given object.
     *
     * @return true if the objects are equal.
     **/
    bool equals(const FeatureSet &rhs) const;

    /**
     * Obtain the names of all the features tracked by this object.
     *
     * @return feature names
     **/
    const StringVector &getNames() const { return _names; }

    /**
     * Obtain the number of features this object contains information
     * about.
     *
     * @return number of features
     **/
    uint32_t numFeatures() const { return _names.size(); }

    /**
     * Obtain the number of documents this object contains information
     * about.
     *
     * @return number of documents.
     **/
    uint32_t numDocs() const { return _docIds.size(); }

    /**
     * Add a document to the set of documents this object contains
     * information about. Documents must be added in ascending
     * order. When a new document is added, all features are
     * initialized to 0.0. The return value from this method can be
     * used together with the @ref getFeaturesByIndex method to set
     * the actual feature values. The ordering among features are
     * assumed to be the same as in the name vector passed to the
     * constructor.
     *
     * @return the index of the document just added
     * @param docid the id of the document to add
     **/
    uint32_t addDocId(uint32_t docid);

    /**
     * Check whether this object contains information about the given
     * set of documents. The given set of documents must be sorted on
     * document id; lowest first.
     *
     * @return true if this object contains information about all the given documents
     * @param docIds the documents we want information about
     **/
    bool contains(const std::vector<uint32_t> &docIds) const;

    /**
     * Obtain the feature values belonging to a document based on the
     * index into the internal docid array. This method is intended
     * for use only when filling in the feature values during object
     * initialization.
     *
     * @return pointer to features
     * @param idx index into docid array
     **/
    feature_t *getFeaturesByIndex(uint32_t idx);

    /**
     * Obtain the feature values belonging to a document based on the
     * docid value. This method is intended for lookup when generating
     * the summary features or rank features docsum field.
     *
     * @return pointer to features
     * @param docId docid value
     **/
    const feature_t *getFeaturesByDocId(uint32_t docId) const;
};

} // namespace search

