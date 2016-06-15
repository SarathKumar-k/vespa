// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".features.debug_wait");
#include "debug_attribute_wait.h"

using search::attribute::IAttributeVector;

namespace search {
namespace features {

//-----------------------------------------------------------------------------

DebugAttributeWaitExecutor::DebugAttributeWaitExecutor(const search::fef::IQueryEnvironment &env,
        const IAttributeVector *attribute,
        const DebugAttributeWaitParams &params)
    : _attribute(attribute),
      _buf(),
      _params(params)
{
    (void)env;
}

void
DebugAttributeWaitExecutor::execute(search::fef::MatchData &data)
{
    double waitTime = 0.0;
    FastOS_Time time;
    time.SetNow();

    if (_attribute != NULL) {
        _buf.fill(*_attribute, data.getDocId());
        waitTime = _buf[0];
    }
    double millis = waitTime * 1000.0;

    while (time.MilliSecsToNow() < millis) {
        if (_params.busyWait) {
            for (int i = 0; i < 1000; i++)
                ;
        } else {
            int rem = (int)(millis - time.MilliSecsToNow());
            FastOS_Thread::Sleep(rem);
        }
    }
    *data.resolveFeature(outputs()[0]) = 1.0e-6 * time.MicroSecsToNow();
}

//-----------------------------------------------------------------------------

DebugAttributeWaitBlueprint::DebugAttributeWaitBlueprint()
    : Blueprint("debugAttributeWait"),
      _params()
{
}

void
DebugAttributeWaitBlueprint::visitDumpFeatures(const search::fef::IIndexEnvironment &env,
                                      search::fef::IDumpFeatureVisitor &visitor) const
{
    (void)env;
    (void)visitor;
}

search::fef::Blueprint::UP
DebugAttributeWaitBlueprint::createInstance() const
{
    return Blueprint::UP(new DebugAttributeWaitBlueprint());
}

bool
DebugAttributeWaitBlueprint::setup(const search::fef::IIndexEnvironment &env,
                          const search::fef::ParameterList &params)
{
    (void)env;
    _attribute = params[0].getValue();
    _params.busyWait = (params[1].asDouble() == 1.0);

    describeOutput("out", "actual time waited");
    env.hintAttributeAccess(_attribute);
    return true;
}

search::fef::FeatureExecutor::LP
DebugAttributeWaitBlueprint::createExecutor(const search::fef::IQueryEnvironment &env) const
{
    // Get attribute vector
    const IAttributeVector * attribute = env.getAttributeContext().getAttribute(_attribute);
    return search::fef::FeatureExecutor::LP(new DebugAttributeWaitExecutor(env, attribute, _params));
}

//-----------------------------------------------------------------------------

} // namespace features
} // namespace search
