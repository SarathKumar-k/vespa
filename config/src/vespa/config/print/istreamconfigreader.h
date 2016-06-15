// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <istream>
#include "configreader.h"
#include "configformatter.h"

namespace config {

/**
 * Read a config from istream
 */
template <typename ConfigType>
class IstreamConfigReader : public ConfigReader<ConfigType>
{
public:
    IstreamConfigReader(std::istream & is);
    std::unique_ptr<ConfigType> read();
    std::unique_ptr<ConfigType> read(const ConfigFormatter & formatter);
private:
    std::istream & _is;
};

} // namespace config

#include "istreamconfigreader.hpp"

