// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
/**
 * \file defs.h
 *
 * \brief Common declarations for the storage API code.
 */
#pragma once

#include <vespa/fastos/fastos.h>

namespace storage {
namespace api {

typedef uint64_t Timestamp;
typedef uint32_t VisitorId;

const Timestamp MAX_TIMESTAMP = (Timestamp)-1ll;

} // api
} // storage

