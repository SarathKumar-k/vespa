// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/storage/visiting/dumpvisitorsingle.h>
#include <vespa/documentapi/messagebus/messages/multioperationmessage.h>
#include <vespa/document/fieldvalue/document.h>
#include <vespa/log/log.h>
#include <vespa/vespalib/text/stringtokenizer.h>
#include <vespa/documentapi/messagebus/messages/putdocumentmessage.h>
#include <vespa/documentapi/messagebus/messages/removedocumentmessage.h>
#include <memory>

LOG_SETUP(".visitor.instance.dumpvisitorsingle");

namespace storage {

DumpVisitorSingle::DumpVisitorSingle(StorageComponent& component,
                                     const vdslib::Parameters&)
    : Visitor(component)
{
}

void DumpVisitorSingle::handleDocuments(const document::BucketId& /*bucketId*/,
                                        std::vector<spi::DocEntry::LP>& entries,
                                        HitCounter& hitCounter)
{
    LOG(debug, "Visitor %s handling block of %zu documents.",
               _id.c_str(), entries.size());

    for (size_t i = 0; i < entries.size(); ++i) {
        spi::DocEntry& entry(*entries[i]);
        const uint32_t docSize = entry.getDocumentSize();
        if (entry.isRemove()) {
            hitCounter.addHit(*entry.getDocumentId(), docSize);
            sendMessage(std::make_unique<documentapi::RemoveDocumentMessage>(
                    *entry.getDocumentId()));
        } else {
            hitCounter.addHit(*entry.getDocumentId(), docSize);
            auto msg = std::make_unique<documentapi::PutDocumentMessage>(
                    entry.releaseDocument());
            msg->setApproxSize(docSize);
            sendMessage(std::move(msg));
        }
    }
}

}
