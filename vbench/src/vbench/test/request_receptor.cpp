// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.


#include <vespa/fastos/fastos.h>
#include "request_receptor.h"

namespace vbench {

void
RequestReceptor::handle(Request::UP req)
{
    request = std::move(req);
}

} // namespace vbench
