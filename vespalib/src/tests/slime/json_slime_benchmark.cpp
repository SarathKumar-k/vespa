// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/vespalib/data/slime/slime.h>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace vespalib::slime::convenience;

std::string make_json(const Slime &slime, bool compact) {
    vespalib::slime::SimpleBuffer buf;
    vespalib::slime::JsonFormat::encode(slime, buf, compact);
    return buf.get().make_string();
}

bool parse_json(const std::string &json, Slime &slime) {
    size_t size = vespalib::slime::JsonFormat::decode(json, slime);
    if (size == 0) {
        fprintf(stderr, "json parsing failed:\n%s", make_json(slime, false).c_str());
    }
    return (size > 0);
}

bool parse_json_bytes(const Memory & json, Slime &slime) {
    size_t size = vespalib::slime::JsonFormat::decode(json, slime);
    if (size == 0) {
        fprintf(stderr, "json parsing failed:\n%s", make_json(slime, false).c_str());
    }
    return (size > 0);
}


int main(int argc, char *argv[])
{
    size_t numRep(10000);
    if (argc > 1) {
        numRep = strtoul(argv[1], 0, 0);
    }
    std::ifstream file("large_json.txt");
    assert(file.is_open());
    std::stringstream buf;
    buf << file.rdbuf();
    std::string str = buf.str();
    Memory mem(str.c_str(), 18911);
    for (size_t i(0); i < numRep; i++) {
        Slime f;
        assert(parse_json_bytes(mem, f));
    }
}
