// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/fastos/fastos.h>

struct PossCount {
    bool valid;
    uint64_t count;

    PossCount() : valid(false), count(0) {}

    bool operator != (const PossCount& other) {
        return (valid != other.valid) || (count != other.count);
    }
};
