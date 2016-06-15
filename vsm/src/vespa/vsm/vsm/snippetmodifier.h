// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/document/fieldvalue/fieldvalue.h>
#include <vespa/vsm/common/charbuffer.h>
#include <vespa/vsm/common/document.h>
#include <vespa/vsm/common/fieldmodifier.h>
#include <vespa/vsm/searcher/utf8substringsnippetmodifier.h>
#include <vespa/vsm/vsm/fieldsearchspec.h>

namespace vsm {

/**
 * This class is responsible for modifying field values where we have substring search and that are used
 * as input to snippet generation.
 *
 * The class implements the FieldModifier interface to modify field values, and the IteratorHandler interface
 * to traverse complex field values. Primitive field values are passed to the underlying searcher that is
 * responsible for modifying the field value by inserting unit separators before and after matches.
 * A group separator is inserted between primitive field values the same way as done by FlattenDocsumWriter.
 **/
class SnippetModifier : public FieldModifier, public document::FieldValue::IteratorHandler
{
private:
    typedef document::FieldValue::IteratorHandler::Content Content;

    UTF8SubstringSnippetModifier::SP _searcher;
    CharBuffer::SP                   _valueBuf; // buffer to store the final modified field value
    char                             _groupSep;
    bool                             _useSep;
    document::FieldPath    _empty;

    void considerSeparator();
    // Inherrit doc from document::FieldValue::IteratorHandler
    virtual void onPrimitive(const Content & c);
    void reset();

public:
    /**
     * Creates a new instance.
     *
     * @param searcher the searcher used to modify primitive field values.
     **/
    SnippetModifier(const UTF8SubstringSnippetModifier::SP & searcher);

    /**
     * Creates a new instance.
     *
     * @param searcher the searcher used to modify primitive field values.
     * @param valueBuf the shared buffer used to store the final modified field value.
     **/
    SnippetModifier(const UTF8SubstringSnippetModifier::SP & searcher, const CharBuffer::SP & valueBuf);

    /**
     * Modifies the complete given field value.
     **/
    virtual document::FieldValue::UP modify(const document::FieldValue & fv) {
        return modify(fv, _empty);
    }

    /**
     * Modifies the given field value by passing all primitive field values to the searcher and
     * inserting group separators between them. A string field value is returned.
     * The iterating of the field value is limited by the given field path.
     *
     * @param fv the field value to modify.
     * @param path the field path used to iterate the field value.
     * @return the new modified field value.
     **/
    virtual document::FieldValue::UP modify(const document::FieldValue & fv,
                                            const document::FieldPath & path);

    const CharBuffer & getValueBuf() const { return *_valueBuf; }
    const UTF8SubstringSnippetModifier::SP & getSearcher() const { return _searcher; }
};

/**
 * This class manages a set of snippet modifiers.
 * The modifiers are instantiated and prepared in the setup function.
 * This class also holds shared buffers that are used by the modifiers.
 **/
class SnippetModifierManager
{
private:
    FieldModifierMap   _modifiers;
    SharedSearcherBuf  _searchBuf;
    CharBuffer::SP     _searchModifyBuf;
    SharedOffsetBuffer _searchOffsetBuf;
    CharBuffer::SP     _modifierBuf;

public:
    SnippetModifierManager();

    /**
     * Setups snippet modifiers for all fields where we have substring search.
     *
     * @param queryTerms the query terms to take into consideration.
     * @param specMap mapping from field id to search spec objects.
     * @param fieldMap mapping from index (used in the query) to a list of field ids.
     **/
    void setup(const search::QueryTermList & queryTerms,
               const FieldSearchSpecMapT & specMap, const IndexFieldMapT & fieldMap);

    const FieldModifierMap & getModifiers() const { return _modifiers; }
};

}

