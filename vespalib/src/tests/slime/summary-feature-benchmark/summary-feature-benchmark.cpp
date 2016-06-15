// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/vespalib/testkit/test_kit.h>
#include <vespa/vespalib/util/stringfmt.h>
#include <vespa/vespalib/data/slime/slime.h>

using namespace vespalib;
using namespace vespalib::slime::convenience;

struct MyBuffer : public slime::Output {
    std::vector<char> data;
    size_t            used;
    MyBuffer() : data(1024 * 1024), used(0) {}
    virtual char *exchange(char *, size_t commit, size_t) {
        return &data[used += commit];
    }
};

std::string make_name(size_t idx) {
    return make_string("summary_feature_%zu", idx);
}

double make_value(size_t idx) {
    return (0.017 * idx);
}

struct FeatureFixture {
    Slime slime;
    FeatureFixture() {
        Cursor &obj = slime.setObject();
        for (size_t i = 0; i < 1000; ++i) {
            obj.setDouble(make_name(i), make_value(i));
        }
    }
};

TEST_F("slime -> json speed", FeatureFixture()) {
    size_t size = 0;
    double minTime = 1000000.0;
    MyBuffer buffer;
    for (size_t i = 0; i < 16; ++i) {
        FastOS_Time timer;
        timer.SetNow();
        for (size_t j = 0; j < 256; ++j) {
            buffer.used = 0;
            slime::JsonFormat::encode(f1.slime, buffer, true);
        }
        minTime = std::min(minTime, timer.MilliSecsToNow() / 256.0);
        size = buffer.used;
    }
    fprintf(stderr, "time: %g ms (size: %zu bytes)\n", minTime, size);
}

TEST_F("slime -> binary speed", FeatureFixture()) {
    size_t size = 0;
    double minTime = 1000000.0;
    MyBuffer buffer;
    for (size_t i = 0; i < 16; ++i) {
        FastOS_Time timer;
        timer.SetNow();
        for (size_t j = 0; j < 256; ++j) {
            buffer.used = 0;
            slime::BinaryFormat::encode(f1.slime, buffer);
        }
        minTime = std::min(minTime, timer.MilliSecsToNow() / 256.0);
        size = buffer.used;
    }
    fprintf(stderr, "time: %g ms (size: %zu bytes)\n", minTime, size);
}

TEST_MAIN() { TEST_RUN_ALL(); }
