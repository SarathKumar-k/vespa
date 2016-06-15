// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once


namespace search {
namespace engine {

struct MonitorRequest
{
    typedef std::shared_ptr<MonitorRequest> SP;
    typedef std::unique_ptr<MonitorRequest> UP;

    bool     reportActiveDocs;
    uint32_t flags;

    MonitorRequest();
};

} // namespace engine
} // namespace search
