// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".features.nativeproximityfeature");
#include <vespa/searchlib/fef/fieldinfo.h>
#include <vespa/searchlib/fef/properties.h>
#include "termdistancefeature.h"
#include "valuefeature.h"
#include "utils.h"

using namespace search::fef;

namespace search {
namespace features {


TermDistanceExecutor::TermDistanceExecutor(const IQueryEnvironment & env,
                                           const TermDistanceParams & params) :
    FeatureExecutor(),
    _params(params),
    _termA(env.getTerm(params.termX)),
    _termB(env.getTerm(params.termY))
{
    _termA.fieldHandle(util::getTermFieldData(env, params.termX, params.fieldId));
    _termB.fieldHandle(util::getTermFieldData(env, params.termY, params.fieldId));
}

bool TermDistanceExecutor::valid() const
{
    return ((_termA.termData() != 0) && (_termB.termData() != 0) &&
            (_termA.fieldHandle() != IllegalHandle) && (_termB.fieldHandle() != IllegalHandle));
}

void
TermDistanceExecutor::execute(MatchData & match)
{
    TermDistanceCalculator::Result result;
    TermDistanceCalculator::run(_termA, _termB, match, result);
    *match.resolveFeature(outputs()[0]) = result.forwardDist;
    *match.resolveFeature(outputs()[1]) = result.forwardTermPos;
    *match.resolveFeature(outputs()[2]) = result.reverseDist;
    *match.resolveFeature(outputs()[3]) = result.reverseTermPos;
}


TermDistanceBlueprint::TermDistanceBlueprint() :
    Blueprint("termDistance"),
    _params()
{
}

void
TermDistanceBlueprint::visitDumpFeatures(const IIndexEnvironment &,
                                         IDumpFeatureVisitor &) const
{
}

Blueprint::UP
TermDistanceBlueprint::createInstance() const
{
    return Blueprint::UP(new TermDistanceBlueprint());
}

bool
TermDistanceBlueprint::setup(const IIndexEnvironment &,
                             const ParameterList & params)
{
    _params.fieldId = params[0].asField()->id();
    _params.termX = params[1].asInteger();
    _params.termY = params[2].asInteger();

    describeOutput("forward",             "the min distance between term X and term Y in the field");
    describeOutput("forwardTermPosition", "the position of term X for the forward distance");
    describeOutput("reverse",             "the min distance between term Y and term X in the field");
    describeOutput("reverseTermPosition", "the position of term Y for the reverse distance");

    return true;
}

FeatureExecutor::LP
TermDistanceBlueprint::createExecutor(const IQueryEnvironment & env) const
{
    std::unique_ptr<TermDistanceExecutor> tde(new TermDistanceExecutor(env, _params));
    if (tde->valid()) {
        return FeatureExecutor::LP(tde.release());
    } else {
        TermDistanceCalculator::Result r;
        std::vector<feature_t> values(4);
        values[0] = r.forwardDist;
        values[1] = r.forwardTermPos;
        values[2] = r.reverseDist;
        values[3] = r.reverseTermPos;
        return FeatureExecutor::LP(new ValueExecutor(values));
    }
}


} // namespace features
} // namespace search
