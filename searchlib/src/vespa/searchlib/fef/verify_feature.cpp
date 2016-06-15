// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".fef.verify_feature");
#include "verify_feature.h"
#include "blueprintresolver.h"

namespace search {
namespace fef {

bool verifyFeature(const BlueprintFactory &factory,
                   const IIndexEnvironment &indexEnv,
                   const std::string &featureName,
                   const std::string &desc)
{
    indexEnv.hintFeatureMotivation(IIndexEnvironment::VERIFY_SETUP);
    BlueprintResolver resolver(factory, indexEnv);
    resolver.addSeed(featureName);
    bool result = resolver.compile();
    if (!result) {
        LOG(error, "rank feature verification failed: %s (%s)",
            featureName.c_str(), desc.c_str());
    }
    return result;
}

} // namespace fef
} // namespace search
