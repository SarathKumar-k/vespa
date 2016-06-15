// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
/**
 * @class storage::api::Parameters
 * @ingroup messageapi
 *
 * @brief A serializable way of setting parameters.
 *
 * Utility class for passing sets of name-value parameter pairs around.
 *
 * @author Fledsbo, Håkon Humberset
 * @date 2004-03-24
 * @version $Id$
 */

#pragma once

#include <vespa/document/util/serializable.h>
#include <vespa/document/util/xmlserializable.h>
#include <vespa/vespalib/stllike/asciistream.h>
#include <vespa/vespalib/stllike/hash_map.h>

namespace vdslib {

class Parameters : public document::Deserializable,
                   public document::XmlSerializable {
public:
    typedef vespalib::stringref KeyT;
    class Value : public vespalib::string
    {
    public:
      Value() { }
      Value(const vespalib::stringref & s) : vespalib::string(s) { }
      Value(const vespalib::string & s) : vespalib::string(s) { }
      Value(const void *v, size_t sz) : vespalib::string(v, sz) { }
      size_t length() const  { return size() - 1; }
    };
    typedef vespalib::stringref ValueRef;
    typedef vespalib::hash_map<vespalib::string, Value> ParametersMap;
private:
    ParametersMap _parameters;

    void onSerialize(document::ByteBuffer& buffer) const;
    void onDeserialize(const document::DocumentTypeRepo &repo,
                       document::ByteBuffer& buffer);
    void printXml(document::XmlOutputStream& xos) const;

public:
    Parameters() : _parameters() {}
    Parameters(const document::DocumentTypeRepo &repo,
               document::ByteBuffer& buffer)
        : _parameters() { deserialize(repo, buffer); }
    virtual ~Parameters();

    bool operator==(const Parameters &other) const;

    Parameters* clone() const;

    size_t getSerializedSize() const;

    bool hasValue(const KeyT & id)                const { return (_parameters.find(id) != _parameters.end()); }
    unsigned int size()                           const { return _parameters.size(); }
    bool get(const KeyT & id, ValueRef & v ) const;
    void set(const KeyT & id, const void * v, size_t sz) { _parameters[id] = Value(v, sz); }

    void print(std::ostream& out, bool verbose, const std::string& indent) const;

    // Disallow
    ParametersMap::const_iterator begin() const { return _parameters.begin(); }
    ParametersMap::const_iterator end() const { return _parameters.end(); }
    /// Convenience from earlier use.
    void set(const KeyT & id, const vespalib::stringref & value) { _parameters[id] = Value(value.c_str(), value.size()); }
    vespalib::stringref get(const KeyT & id, const vespalib::stringref & def = "") const;
    /**
     * Set the value identified by the id given. This requires the type to be
     * convertible by stringstreams.
     *
     * @param id The value to get.
     * @param t The value to save. Will be converted to a string.
     */
    template<typename T>
    void set(const KeyT & id, const T& t) {
        vespalib::asciistream ost;
        ost << t;
        _parameters[id] = ost.str();
    }

    /**
     * Get the value identified by the id given, as the same type as the default
     * value given. This requires the type to be convertible by stringstreams.
     *
     * @param id The value to get.
     * @param def The value to return if the value does not exist.
     * @return The value represented as the same type as the default given, or
     *         the default itself if value did not exist.
     */
    template<typename T>
    T get(const KeyT & id, const T& def) const {
        vespalib::stringref ref;
        if (!get(id, ref)) return def;
        vespalib::asciistream ist(ref);
        T t;
        ist >> t;
        return t;
    }

    std::string toString() const;
};

} // vdslib

