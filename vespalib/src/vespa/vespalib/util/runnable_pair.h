// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "runnable.h"

namespace vespalib {

/**
 * Simple utility running two runnables in order.
 **/
class RunnablePair : public Runnable
{
private:
    Runnable &_first;
    Runnable &_second;

public:
    RunnablePair(Runnable &first, Runnable &second);
    virtual void run();
};

} // namespace vesaplib

