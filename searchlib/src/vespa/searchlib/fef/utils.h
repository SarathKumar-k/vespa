// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "rank_program.h"
#include <vespa/vespalib/eval/value.h>
#include <map>

namespace search {
namespace fef {

struct Utils
{
    /**
     * Extract a single score feature from the given rank program.
     */
    static const feature_t *getScoreFeature(const RankProgram &rankProgram);

    /**
     * Extract a single object feature from the given rank program.
     */
    static const vespalib::eval::Value::CREF *getObjectFeature(const RankProgram &rankProgram);

    /**
     * Extract all seed feature values from the given rank program.
     **/
    static std::map<vespalib::string, feature_t> getSeedFeatures(const RankProgram &rankProgram);

    /**
     * Extract all feature values from the given rank program.
     **/
    static std::map<vespalib::string, feature_t> getAllFeatures(const RankProgram &rankProgram);

};

} // namespace fef
} // namespace search
