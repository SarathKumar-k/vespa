// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/searchlib/query/base.h>

namespace search {

Object::~Object(void)
{
}

vespalib::string Object::toString() const
{
    return vespalib::string("");
}

}
