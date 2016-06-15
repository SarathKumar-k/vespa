// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/documentapi/messagebus/messages/removelocationmessage.h>
#include <vespa/documentapi/messagebus/documentprotocol.h>
#include <vespa/document/select/parser.h>
#include <vespa/document/bucket/bucketselector.h>

namespace documentapi {

RemoveLocationMessage::RemoveLocationMessage(
        const document::BucketIdFactory& factory,
        document::select::Parser& parser,
        const string& documentSelection)
    : _documentSelection(documentSelection)
{
    document::BucketSelector bucketSel(factory);
    std::unique_ptr<document::BucketSelector::BucketVector> exprResult(
            bucketSel.select(*parser.parse(documentSelection)));

    if (exprResult.get() && exprResult->size() == 1) {
        _bucketId = (*exprResult)[0];
    } else {
        throw vespalib::IllegalArgumentException(
                "Document selection doesn't map to a single bucket!",
                VESPA_STRLOC);
    }
}

DocumentReply::UP
RemoveLocationMessage::doCreateReply() const {
    return DocumentReply::UP(
            new DocumentReply(DocumentProtocol::REPLY_REMOVELOCATION));
}

uint32_t
RemoveLocationMessage::getType() const {
    return DocumentProtocol::MESSAGE_REMOVELOCATION;
}

}
