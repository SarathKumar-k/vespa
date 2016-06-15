// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/document/base/documentid.h>
#include <vespa/documentapi/messagebus/messages/documentmessage.h>

namespace documentapi {

class GetDocumentMessage : public DocumentMessage {
private:
    document::DocumentId _documentId; // The identifier of the document to retrieve.
    string               _fieldSet; // Comma-separated list of fields to return

protected:
    // Implements DocumentMessage.
    DocumentReply::UP doCreateReply() const;

public:
    /**
     * Convenience typedef.
     */
    typedef std::unique_ptr<GetDocumentMessage> UP;
    typedef std::shared_ptr<GetDocumentMessage> SP;

    enum {
        FLAG_NONE        = 0,
        FLAG_ONLY_HEADER = 1
    };

    /**
     * Constructs a new message for deserialization.
     */
    GetDocumentMessage();

    /**
     * Constructs a new document get message.
     *
     * @param documentId The identifier of the document to retrieve.
     * @param flags      How to retrieve the document.
     */
    GetDocumentMessage(const document::DocumentId &documentId, int flags = 0);

    /**
     * Constructs a new document get message.
     *
     * @param documentId The identifier of the document to retrieve.
     * @param fieldSet The fields to retrieve (comma-separated)
     */
    GetDocumentMessage(const document::DocumentId &documentId,
                       const vespalib::stringref & fieldSet);

    /**
     * Returns the identifier of the document to retrieve.
     *
     * @return The document id.
     */
    const document::DocumentId &getDocumentId() const;

    /**
     * Sets the identifier of the document to retrieve.
     *
     * @param documentId The document id to set.
     */
    void setDocumentId(const document::DocumentId &documentId);

    /**
     * Returs the storage flags of this message.
     *
     * @return The storage flags.
     */
    int getFlags() const { return (_fieldSet == "[header]" ? FLAG_ONLY_HEADER :
                                   FLAG_NONE); };

    /**
     * Sets the storage flags of this message.
     *
     * @param flags The flags to set.
     */
    void setFlags(int flags) {
        _fieldSet = (flags == FLAG_ONLY_HEADER) ? "[header]" : "[all]";
    }

    /**
     * Returns the fields to be retrieved by the get.
     */
    const string& getFieldSet() const { return _fieldSet; }

    // Overrides DocumentMessage.
    bool hasSequenceId() const;

    // Overrides DocumentMessage.
    uint64_t getSequenceId() const;

    // Implements DocumentMessage.
    uint32_t getType() const;

    string toString() const { return "getdocumentmessage"; }
};

}

