// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/log/log.h>
LOG_SETUP("info_test");
#include <vespa/fastos/fastos.h>
#include <vespa/vespalib/testkit/testapp.h>

TEST_SETUP(Test);

int
Test::Main()
{
    TEST_INIT("info_test");
    TEST_DONE();
}
