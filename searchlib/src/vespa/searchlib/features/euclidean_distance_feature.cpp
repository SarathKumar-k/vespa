// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
#include <cmath>
LOG_SETUP(".features.euclidean_distance_feature");
#include <vespa/searchcommon/attribute/attributecontent.h>
#include <vespa/searchcommon/attribute/iattributevector.h>
#include <vespa/searchlib/fef/properties.h>
#include "valuefeature.h"

#include "euclidean_distance_feature.h"
#include "array_parser.hpp"
#include <vespa/searchlib/attribute/integerbase.h>
#include <vespa/searchlib/attribute/floatbase.h>

using namespace search::attribute;
using namespace search::fef;

namespace search {
namespace features {


template <typename DataType>
EuclideanDistanceExecutor<DataType>::EuclideanDistanceExecutor(const search::attribute::IAttributeVector &attribute, QueryVectorType vector) :
    FeatureExecutor(),
    _attribute(attribute),
    _vector(std::move(vector)),
    _attributeBuffer()
{
}

template <typename DataType>
feature_t EuclideanDistanceExecutor<DataType>::euclideanDistance(const BufferType &v1, const QueryVectorType &v2)
{
    feature_t val = 0;
    size_t commonRange = std::min(static_cast<size_t>( v1.size() ), v2.size());
    for (size_t i = 0; i < commonRange; ++i)  {
        feature_t diff = v1[i] - v2[i];
        val += diff * diff;
    }
    return std::sqrt(val);
}


template <typename DataType>
void
EuclideanDistanceExecutor<DataType>::execute(MatchData &match)
{
    _attributeBuffer.fill(_attribute, match.getDocId());
    *match.resolveFeature(outputs()[0]) = euclideanDistance(_attributeBuffer, _vector);
}


EuclideanDistanceBlueprint::EuclideanDistanceBlueprint() :
    Blueprint("euclideanDistance"),
    _attributeName(),
    _queryVector()
{
}

void
EuclideanDistanceBlueprint::visitDumpFeatures(const IIndexEnvironment &, IDumpFeatureVisitor &) const
{
}

bool
EuclideanDistanceBlueprint::setup(const IIndexEnvironment &env, const ParameterList &params)
{
    _attributeName = params[0].getValue();
    _queryVector = params[1].getValue();
    describeOutput("distance", "The result after calculating the euclidean distance of the vector represented by the array "
                             "and the vector sent down with the query");
    env.hintAttributeAccess(_attributeName);
    return true;
}

Blueprint::UP
EuclideanDistanceBlueprint::createInstance() const
{
    return Blueprint::UP(new EuclideanDistanceBlueprint());
}

namespace {

template <typename DataType> 
FeatureExecutor::LP create(const IAttributeVector &attribute, const Property &queryVector)
{
    std::vector<DataType> v;
    ArrayParser::parse(queryVector.get(), v);
    return FeatureExecutor::LP(new EuclideanDistanceExecutor<DataType>(attribute, std::move(v)));        
}

}

FeatureExecutor::LP
EuclideanDistanceBlueprint::createExecutor(const IQueryEnvironment &env) const
{
    const IAttributeVector * attribute = env.getAttributeContext().getAttribute(_attributeName);
    if (attribute == NULL) {
        LOG(warning, "The attribute vector '%s' was not found in the attribute manager, returning executor with default value.",
            _attributeName.c_str());
        return FeatureExecutor::LP(new SingleZeroValueExecutor());
    }

    Property queryVector = env.getProperties().lookup(getBaseName(), _queryVector);

    if (attribute->getCollectionType() == attribute::CollectionType::ARRAY) {
        if (attribute->isIntegerType()) {
            return create<IAttributeVector::largeint_t>(*attribute, queryVector);
        } else if (attribute->isFloatingPointType()) {
            return create<double>(*attribute, queryVector);
        }
    }
    LOG(warning, "The attribute vector '%s' is NOT of type array<int/long/float/double>"
            ", returning executor with default value.", attribute->getName().c_str());
    return FeatureExecutor::LP(new SingleZeroValueExecutor());

}

} // namespace features
} // namespace search

