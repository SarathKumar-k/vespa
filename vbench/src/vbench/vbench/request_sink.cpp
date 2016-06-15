// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.


#include <vespa/fastos/fastos.h>
#include "request_sink.h"
#include <vbench/core/string.h>

namespace vbench {

RequestSink::RequestSink()
{
}

void
RequestSink::handle(Request::UP request)
{
    request.reset();
}

void
RequestSink::report()
{
}

} // namespace vbench
