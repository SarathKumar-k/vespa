// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/storageapi/messageapi/storagereply.h>

#include <vespa/storageapi/messageapi/storagecommand.h>

namespace storage {
namespace api {

StorageReply::StorageReply(const StorageCommand& cmd, ReturnCode code)
    : StorageMessage(cmd.getType().getReplyType(), cmd.getMsgId()),
      _result(code)
{
    setPriority(cmd.getPriority());
    if (cmd.getAddress()) {
        setAddress(*cmd.getAddress());
    }
    setTrace(cmd.getTrace());
    setTransportContext(cmd.getTransportContext());
}

void
StorageReply::print(std::ostream& out, bool verbose,
                    const std::string& indent) const
{
    (void) verbose; (void) indent;
    out << "StorageReply(" << _type.getName() << ", "
        << _result.toString() << ")";
}

} // api
} // storage
