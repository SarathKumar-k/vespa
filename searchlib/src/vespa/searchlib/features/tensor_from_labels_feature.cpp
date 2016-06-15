// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".features.tensor_from_labels_feature");

#include "tensor_from_labels_feature.h"
#include "array_parser.hpp"
#include "constant_tensor_executor.h"
#include "tensor_from_attribute_executor.h"
#include <vespa/searchcommon/attribute/attributecontent.h>
#include <vespa/searchcommon/attribute/iattributevector.h>
#include <vespa/searchlib/fef/properties.h>
#include <vespa/vespalib/tensor/default_tensor.h>
#include <vespa/vespalib/eval/value_type.h>
#include <vespa/searchlib/fef/feature_type.h>

using namespace search::fef;
using search::attribute::IAttributeVector;
using search::attribute::WeightedConstCharContent;
using search::attribute::WeightedStringContent;
using vespalib::tensor::DefaultTensor;
using vespalib::tensor::TensorBuilder;
using vespalib::eval::ValueType;
using search::fef::FeatureType;

namespace search {
namespace features {

TensorFromLabelsBlueprint::TensorFromLabelsBlueprint()
    : TensorFactoryBlueprint("tensorFromLabels")
{
}

bool
TensorFromLabelsBlueprint::setup(const search::fef::IIndexEnvironment &env,
                                 const search::fef::ParameterList &params)
{
    (void) env;
    // _params[0] = source ('attribute(name)' OR 'query(param)');
    // _params[1] = dimension (optional);
    bool validSource = extractSource(params[0].getValue());
    if (params.size() == 2) {
        _dimension = params[1].getValue();
    } else {
        _dimension = _sourceParam;
    }
    describeOutput("tensor",
                   "The tensor created from the given array source (attribute field or query parameter)",
                   FeatureType::object(ValueType::tensor_type({{_dimension}})));
    return validSource;
}

namespace {

FeatureExecutor::LP
createAttributeExecutor(const search::fef::IQueryEnvironment &env,
                        const vespalib::string &attrName,
                        const vespalib::string &dimension)
{
    const IAttributeVector *attribute = env.getAttributeContext().getAttribute(attrName);
    if (attribute == NULL) {
        LOG(warning, "The attribute vector '%s' was not found in the attribute manager."
                " Returning empty tensor.", attrName.c_str());
        return ConstantTensorExecutor::createEmpty();
    }
    if (attribute->getCollectionType() != search::attribute::CollectionType::ARRAY ||
            attribute->isFloatingPointType()) {
        LOG(warning, "The attribute vector '%s' is NOT of type array of string or integer."
                " Returning empty tensor.", attrName.c_str());
        return ConstantTensorExecutor::createEmpty();
    }
    // Note that for array attribute vectors the default weight is 1.0 for all values.
    // This means we can get the attribute content as weighted content and build
    // the tensor the same way as with weighted set attributes in tensorFromWeightedSet.
    if (attribute->isIntegerType()) {
        // Using WeightedStringContent ensures that the integer values are converted
        // to strings while extracting them from the attribute.
        return FeatureExecutor::LP
                (new TensorFromAttributeExecutor<WeightedStringContent>(attribute, dimension));
    }
    // When the underlying attribute is of type string we can reference these values
    // using WeightedConstCharContent.
    return FeatureExecutor::LP
            (new TensorFromAttributeExecutor<WeightedConstCharContent>(attribute, dimension));
}

FeatureExecutor::LP
createQueryExecutor(const search::fef::IQueryEnvironment &env,
                    const vespalib::string &queryKey,
                    const vespalib::string &dimension)
{
    search::fef::Property prop = env.getProperties().lookup(queryKey);
    if (prop.found() && !prop.get().empty()) {
        std::vector<vespalib::string> vector;
        ArrayParser::parse(prop.get(), vector);
        DefaultTensor::builder tensorBuilder;
        TensorBuilder::Dimension dimensionEnum = tensorBuilder.define_dimension(dimension);
        for (const auto &elem : vector) {
            tensorBuilder.add_label(dimensionEnum, elem);
            tensorBuilder.add_cell(1.0);
        }
        return ConstantTensorExecutor::create(tensorBuilder.build());
    }
    return ConstantTensorExecutor::createEmpty();
}

}

FeatureExecutor::LP
TensorFromLabelsBlueprint::createExecutor(const search::fef::IQueryEnvironment &env) const
{
    if (_sourceType == ATTRIBUTE_SOURCE) {
        return createAttributeExecutor(env, _sourceParam, _dimension);
    } else if (_sourceType == QUERY_SOURCE) {
        return createQueryExecutor(env, _sourceParam, _dimension);
    }
    return ConstantTensorExecutor::createEmpty();
}

} // namespace features
} // namespace search
