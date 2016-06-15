// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/vespalib/data/slime/slime.h>

namespace config {

class ConfigPayload {
public:
    ConfigPayload(const ::vespalib::slime::Inspector & inspector)
        : _inspector(inspector)
    {}
    const ::vespalib::slime::Inspector & get() const { return _inspector; }
private:
    const ::vespalib::slime::Inspector & _inspector;
};

} // namespace config

