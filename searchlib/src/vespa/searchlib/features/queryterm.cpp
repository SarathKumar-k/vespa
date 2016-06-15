// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".features.queryterm");
#include "queryterm.h"
#include "utils.h"

using namespace search::fef;
using search::feature_t;

namespace search {
namespace features {

QueryTerm::QueryTerm() :
    _termData(NULL),
    _handle(IllegalHandle),
    _significance(0),
    _connectedness(0)
{
}

QueryTerm::QueryTerm(const ITermData * td, feature_t sig, feature_t con) :
    _termData(td),
    _handle(IllegalHandle),
    _significance(sig),
    _connectedness(con)
{
}

QueryTerm
QueryTermFactory::create(const IQueryEnvironment & env,
                         uint32_t termIdx,
                         bool lookupSignificance,
                         bool lookupConnectedness)
{
    const ITermData *termData = env.getTerm(termIdx);
    feature_t significance = 0;
    if (lookupSignificance) {
        feature_t fallback = util::getSignificance(*termData);
        significance = util::lookupSignificance(env, termIdx, fallback);
    }
    feature_t connectedness = 0;
    if (lookupConnectedness) {
        connectedness = search::features::util::lookupConnectedness(env, termIdx);
    }
    return QueryTerm(termData, significance, connectedness);
}


} // namespace features
} // namespace search
