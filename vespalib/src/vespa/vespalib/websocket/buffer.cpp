// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.


#include <vespa/fastos/fastos.h>
#include "buffer.h"

namespace vespalib {
namespace ws {

void
Buffer::ensure_free(size_t bytes)
{
    memmove(&_data[0], &_data[_read_pos], used());
    _write_pos -= _read_pos;
    _read_pos = 0;
    if (free() < bytes) {
        _data.resize(_write_pos + bytes);
    }
}

} // namespace vespalib::ws
} // namespace vespalib
