// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".features.jarowinklerdistance");

#include <vespa/searchlib/fef/featurenamebuilder.h>
#include <vespa/searchlib/fef/fieldinfo.h>
#include <vespa/searchlib/fef/fieldtype.h>
#include <vespa/searchlib/fef/properties.h>
#include <vespa/searchlib/fef/itermdata.h>
#include <vespa/vespalib/util/stringfmt.h>
#include "jarowinklerdistancefeature.h"
#include "utils.h"
#include <limits>

namespace search {
namespace features {

//-----------------------------------------------------------------------------
// JaroWinklerDistanceConfig
//-----------------------------------------------------------------------------
JaroWinklerDistanceConfig::JaroWinklerDistanceConfig() :
    fieldId(search::fef::IllegalHandle),
    fieldBegin(0),
    fieldEnd(std::numeric_limits<uint32_t>::max()),
    boostThreshold(0.7f),
    prefixSize(4u)
{
    // empty
}

//-----------------------------------------------------------------------------
// JaroWinklerDistanceExecutor
//-----------------------------------------------------------------------------
JaroWinklerDistanceExecutor::JaroWinklerDistanceExecutor(const search::fef::IQueryEnvironment &env,
                                                         const JaroWinklerDistanceConfig &config) :
    search::fef::FeatureExecutor(),
    _config(config),
    _termFieldHandles(),
    _lenHandle(search::fef::IllegalHandle)
{
    for (uint32_t i = 0; i < env.getNumTerms(); ++i) {
        _termFieldHandles.push_back(util::getTermFieldHandle(env, i, config.fieldId));
    }
}

void
JaroWinklerDistanceExecutor::execute(search::fef::MatchData &match)
{
    // Build a list of field position iterators, one per query term.
    std::vector<search::fef::FieldPositionsIterator> pos;
    for (uint32_t term = 0; term < _termFieldHandles.size(); ++term) {
        search::fef::FieldPositionsIterator it; // this is not vaild
        const search::fef::TermFieldHandle &handle = _termFieldHandles[term];
        if (handle != search::fef::IllegalHandle) {
            search::fef::TermFieldMatchData &tfmd = *match.resolveTermField(handle);
            if (tfmd.getDocId() == match.getDocId()) {
                it = tfmd.getIterator();
            }
        }
        pos.push_back(it);
    }

    // Assign the jaroWinkler distance to this executor's output.
    *match.resolveFeature(outputs()[0]) = 1 - jaroWinklerProximity(pos, (uint32_t)*match.resolveFeature(_lenHandle));
}

namespace {
uint32_t
matches(const std::vector<search::fef::FieldPositionsIterator> &termPos,
        uint32_t fieldLen, uint32_t *numTransposes)
{
    (*numTransposes) = 0u;
    uint32_t ret = 0;
    uint32_t halfLen = termPos.size() > fieldLen ? (fieldLen / 2 + 1) : (termPos.size() / 2 + 1);
    for (uint32_t i = 0; i < termPos.size(); ++i) {
        uint32_t min = i > halfLen ? i - halfLen : 0u;
        uint32_t max = std::min(fieldLen, i + halfLen);
        for (search::fef::FieldPositionsIterator it = termPos[i]; it.valid() && it.getPosition() <= max; it.next()) {
            uint32_t pos = it.getPosition();
            if (pos >= min && pos <= max) {
                if (pos != i) {
                    (*numTransposes)++;
                }
                ret++;
                break;
            }
        }
    }
    (*numTransposes) /= 2;
    return ret;
}

uint32_t
prefixMatch(const std::vector<search::fef::FieldPositionsIterator> &termPos, uint32_t fieldLen, uint32_t maxLen)
{
    uint32_t len = std::min((uint32_t)termPos.size(), std::min(fieldLen, maxLen));
    for (uint32_t i = 0; i < len; ++i) {
        if (!termPos[i].valid() || termPos[i].getPosition() != i) {
            return i;
        }
    }
    return len;
}

feature_t
jaroMeasure(const std::vector<search::fef::FieldPositionsIterator> &termPos, uint32_t fieldLen)
{
    // _P_A_R_A_N_O_I_A_
    if (termPos.empty() || fieldLen == 0) {
        return 0.0f;
    }
    uint32_t numTransposes = 0;
    uint32_t numMatches = matches(termPos, fieldLen, &numTransposes);
    if (numMatches == 0u) {
        return 0.0f;
    }
    return (((feature_t)numMatches / termPos.size()) +
            ((feature_t)numMatches / fieldLen) +
            ((feature_t)numMatches - numTransposes) / numMatches) / 3.0f;
}
}  // namespace

feature_t
JaroWinklerDistanceExecutor::jaroWinklerProximity(const std::vector<search::fef::FieldPositionsIterator> &termPos, uint32_t fieldLen)
{
    feature_t ret = std::min(1.0, std::max(0.0, jaroMeasure(termPos, fieldLen)));
    //LOG(debug, "Jaro measure is %f.", ret);
    if (ret > _config.boostThreshold) {
        ret += 0.1f * prefixMatch(termPos, fieldLen, _config.prefixSize) * (1 - ret); // less boost close to 1
        //LOG(debug, "Applying Winkler boost.");
    }
    //LOG(debug, "JaroWinkler measure is %f.", ret);
    return ret;
}

//-----------------------------------------------------------------------------
// JaroWinklerDistanceBlueprint
//-----------------------------------------------------------------------------
JaroWinklerDistanceBlueprint::JaroWinklerDistanceBlueprint() :
    search::fef::Blueprint("jaroWinklerDistance"),
    _config()
{
    // empty
}

void
JaroWinklerDistanceBlueprint::visitDumpFeatures(const search::fef::IIndexEnvironment &,
                                                search::fef::IDumpFeatureVisitor &) const
{
    // empty
}

bool
JaroWinklerDistanceBlueprint::setup(const search::fef::IIndexEnvironment &env,
                                    const search::fef::ParameterList &params)
{
    _config.fieldId = params[0].asField()->id();

    vespalib::string boostThreshold = env.getProperties().lookup(getName(), "boostThreshold").getAt(0);
    _config.boostThreshold = boostThreshold.empty() ? 0.7f : atof(boostThreshold.c_str());

    vespalib::string prefixSize = env.getProperties().lookup(getName(), "prefixSize").getAt(0);
    _config.prefixSize = prefixSize.empty() ? 4 : atoi(prefixSize.c_str());

    defineInput(vespalib::make_string("fieldLength(%s)", params[0].getValue().c_str()));
    describeOutput("out", "JaroWinklerDistance distance measure.");
    env.hintFieldAccess(_config.fieldId);
    return true;
}

search::fef::Blueprint::UP
JaroWinklerDistanceBlueprint::createInstance() const
{
    return search::fef::Blueprint::UP(new JaroWinklerDistanceBlueprint());
}

search::fef::FeatureExecutor::LP
JaroWinklerDistanceBlueprint::createExecutor(const search::fef::IQueryEnvironment &env) const
{
    return search::fef::FeatureExecutor::LP(new JaroWinklerDistanceExecutor(env, _config));
}

}}
