// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/vespalib/testkit/testapp.h>
#include <vbench/test/all.h>

using namespace vbench;

void checkMemory(const string &expect, const Memory &mem) {
    EXPECT_EQUAL(expect, string(mem.data, mem.size));
}

void checkBuffer(const string &expect, const SimpleBuffer &buf) {
    TEST_DO(checkMemory(expect, buf.get()));
}

TEST("simple buffer") {
    SimpleBuffer buf;
    TEST_DO(checkBuffer("", buf));
    { // read from empty buffer
        EXPECT_EQUAL(0u, buf.obtain(1, 1).size);
    }
    { // write to buffer
        WritableMemory mem = buf.reserve(10);
        TEST_DO(checkBuffer("", buf));
        EXPECT_EQUAL(10u, mem.size);
        mem.data[0] = 'a';
        mem.data[1] = 'b';
        mem.data[2] = 'c';
        EXPECT_EQUAL(&buf, &buf.commit(3, 0));
        mem = buf.reserve(0);
        TEST_DO(checkBuffer("abc", buf));
        EXPECT_EQUAL(0u, mem.size);
    }
    { // unaligned read across end (last byte not evicted)
        Memory mem = buf.obtain(2, 1);
        TEST_DO(checkBuffer("abc", buf));
        TEST_DO(checkMemory("ab", mem));
        EXPECT_EQUAL(&buf, &buf.evict(2));
        mem = buf.obtain(2, 1);
        TEST_DO(checkBuffer("c", buf));
        TEST_DO(checkMemory("c", mem));
        mem = buf.obtain(2, 1);
        TEST_DO(checkBuffer("c", buf));
        TEST_DO(checkMemory("c", mem));
    }
    { // write more to buffer
        WritableMemory mem = buf.reserve(10);
        EXPECT_EQUAL(10u, mem.size);
        TEST_DO(checkBuffer("c", buf));
        mem.data[0] = 'd';
        EXPECT_EQUAL(&buf, &buf.commit(1, 0));
        mem = buf.reserve(5);
        TEST_DO(checkBuffer("cd", buf));
        EXPECT_EQUAL(5u, mem.size);
    }
    { // aligned read until end
        Memory mem = buf.obtain(1, 1);
        TEST_DO(checkBuffer("cd", buf));
        TEST_DO(checkMemory("c", mem));
        EXPECT_EQUAL(&buf, &buf.evict(1));
        mem = buf.obtain(1, 1);
        TEST_DO(checkBuffer("d", buf));
        TEST_DO(checkMemory("d", mem));
        EXPECT_EQUAL(&buf, &buf.evict(1));
        mem = buf.obtain(1, 1);
        TEST_DO(checkBuffer("", buf));
        TEST_DO(checkMemory("", mem));
    }
}

TEST_MAIN() { TEST_RUN_ALL(); }
