// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP("directio_test");
#include <vespa/vespalib/testkit/test_kit.h>
#include <vespa/vespalib/stllike/string.h>
#include <vespa/vespalib/data/databuffer.h>

using namespace fastos;
using namespace vespalib;

TEST("that DirectIOException propagates the correct information.") {
    const char *msg("The buffer");
    DirectIOException e("file.a", msg, 10, 3);
    EXPECT_EQUAL(10u, e.getLength());
    EXPECT_EQUAL(3u, e.getOffset());
    EXPECT_EQUAL(msg, e.getBuffer());
    EXPECT_EQUAL(0u, string(e.what()).find("DirectIO failed for file 'file.a' buffer="));
    EXPECT_EQUAL(string("file.a"), e.getFileName());
}

TEST("that DirectIOException is thrown on unaligned buf.") {
    FastOS_File f("staging_vespalib_directio_test_app");
    f.EnableDirectIO();
    EXPECT_TRUE(f.OpenReadOnly());
    DataBuffer buf(10000, 4096);
    bool caught(false);
    try {
        f.ReadBuf(buf.getFree()+1, 4096, 0);
    } catch (const DirectIOException & e) {
        EXPECT_EQUAL(4096u, e.getLength());
        EXPECT_EQUAL(0u, e.getOffset());
        EXPECT_EQUAL(buf.getFree()+1, e.getBuffer());
        EXPECT_EQUAL(string(f.GetFileName()), e.getFileName());
        caught = true;
    }
    EXPECT_TRUE(caught);
}

TEST("that DirectIOException is thrown on unaligned offset.") {
    FastOS_File f("staging_vespalib_directio_test_app");
    f.EnableDirectIO();
    EXPECT_TRUE(f.OpenReadOnly());
    DataBuffer buf(10000, 4096);
    bool caught(false);
    try {
        f.ReadBuf(buf.getFree(), 4096, 1);
    } catch (const DirectIOException & e) {
        EXPECT_EQUAL(4096u, e.getLength());
        EXPECT_EQUAL(1u, e.getOffset());
        EXPECT_EQUAL(buf.getFree(), e.getBuffer());
        EXPECT_EQUAL(string(f.GetFileName()), e.getFileName());
        caught = true;
    }
    EXPECT_TRUE(caught);
}

TEST_MAIN() { TEST_RUN_ALL(); }
