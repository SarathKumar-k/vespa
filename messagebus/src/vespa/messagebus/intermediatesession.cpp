// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".intermediatesession");

#include <vespa/vespalib/util/vstringfmt.h>
#include "intermediatesession.h"
#include "messagebus.h"
#include "replygate.h"

namespace mbus {

IntermediateSession::IntermediateSession(MessageBus &mbus, const IntermediateSessionParams &params) :
    _mbus(mbus),
    _name(params.getName()),
    _msgHandler(params.getMessageHandler()),
    _replyHandler(params.getReplyHandler()),
    _gate(new ReplyGate(_mbus))
{
    // empty
}

IntermediateSession::~IntermediateSession()
{
    _gate->close();
    close();
    _gate->subRef();
}

void
IntermediateSession::close()
{
    _mbus.unregisterSession(_name);
    _mbus.sync();
}

void
IntermediateSession::forward(Routable::UP routable)
{
    if (routable->isReply()) {
        forward(Reply::UP(static_cast<Reply*>(routable.release())));
    } else {
        forward(Message::UP(static_cast<Message*>(routable.release())));
    }
}

void
IntermediateSession::forward(Reply::UP reply)
{
    IReplyHandler &handler = reply->getCallStack().pop(*reply);
    handler.handleReply(std::move(reply));
}

void
IntermediateSession::forward(Message::UP msg)
{
    msg->pushHandler(*this);
    _gate->handleMessage(std::move(msg));
}

void
IntermediateSession::handleMessage(Message::UP msg)
{
    _msgHandler.handleMessage(std::move(msg));
}

void
IntermediateSession::handleReply(Reply::UP reply)
{
    _replyHandler.handleReply(std::move(reply));
}

const string
IntermediateSession::getConnectionSpec() const
{
    return _mbus.getConnectionSpec() + "/" + _name;
}

} // namespace mbus
