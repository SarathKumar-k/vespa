// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".proton.flushengine.flushcontext");

#include "flushcontext.h"

namespace proton {

FlushContext::FlushContext(
        const IFlushHandler::SP &handler,
        const IFlushTarget::SP &target,
        search::SerialNum oldestFlushable,
        search::SerialNum lastSerial)
    : _name(createName(*handler, *target)),
      _handler(handler),
      _target(target),
      _task(),
      _oldestFlushable(oldestFlushable),
      _lastSerial(lastSerial)
{
    // empty
}

vespalib::string FlushContext::createName(const IFlushHandler & handler, const IFlushTarget & target)
{
    return (handler.getName() + "." + target.getName());
}

FlushContext::~FlushContext()
{
    if (_task.get() != NULL) {
        LOG(warning, "Unexecuted flush task for '%s' destroyed.",
            _name.c_str());
    }
}

bool
FlushContext::initFlush()
{
    LOG(debug, "Attempting to flush '%s'.", _name.c_str());
    _task = _target->initFlush(std::max(_handler->getCurrentSerialNumber(), _lastSerial));
    if (_task.get() == NULL) {
        LOG(debug, "Target refused to init flush.");
        return false;
    }
    return true;
}

} // namespace proton
