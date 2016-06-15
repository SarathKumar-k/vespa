// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.


#pragma once

#include <vespa/vespalib/util/runnable.h>
#include <vbench/core/input_file_reader.h>
#include <vbench/core/taintable.h>
#include <vbench/core/handler.h>

#include "request.h"
#include "generator.h"

namespace vbench {

/**
 * Reads lines from an input file and generates requests that are
 * passed to a request handler.
 **/
class RequestGenerator : public Generator
{
private:
    InputFileReader   _input;
    Handler<Request> &_next;
    bool              _aborted;

public:
    RequestGenerator(const string &inputFile,
                     Handler<Request> &next);
    void abort();
    virtual void run();
    virtual const Taint &tainted() const { return _input.tainted(); }
};

} // namespace vbench

