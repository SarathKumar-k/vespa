// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
// @author Vegard Sjonfjell

#pragma once

#include <vespa/vespalib/util/growablebytebuffer.h>
#include <vespa/documentapi/loadtypes/loadtypeset.h>
#include <vespa/storageapi/mbusprot/protocolserialization5_1.h>
#include <vespa/storageapi/message/persistence.h>

namespace storage {
namespace mbusprot {

class ProtocolSerialization5_2 : public ProtocolSerialization5_1
{
public:
    ProtocolSerialization5_2(
        const document::DocumentTypeRepo::SP& repo,
        const documentapi::LoadTypeSet & loadTypes)
        : ProtocolSerialization5_1(repo, loadTypes)
        {}

protected:
    virtual void onEncode(GBBuf &, const api::PutCommand &) const override;
    virtual void onEncode(GBBuf &, const api::RemoveCommand &) const override;
    virtual void onEncode(GBBuf &, const api::UpdateCommand &) const override;

    virtual SCmd::UP onDecodePutCommand(BBuf &) const override;
    virtual SCmd::UP onDecodeRemoveCommand(BBuf &) const override;
    virtual SCmd::UP onDecodeUpdateCommand(BBuf &) const override;

    static void decodeTasCondition(api::StorageCommand & cmd, BBuf & buf);
    static void encodeTasCondition(GBBuf & buf, const api::StorageCommand & cmd);
};

} // mbusprot
} // storage
