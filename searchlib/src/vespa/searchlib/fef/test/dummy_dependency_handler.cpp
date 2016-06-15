// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.


#include <vespa/fastos/fastos.h>
#include "dummy_dependency_handler.h"

namespace search {
namespace fef {
namespace test {

DummyDependencyHandler::DummyDependencyHandler(Blueprint &blueprint_in)
    : blueprint(blueprint_in),
      object_type_map(),
      accept_type_mismatch(false),
      input(),
      accept_input(),
      output(),
      output_type()
{
    blueprint.attach_dependency_handler(*this);
}

DummyDependencyHandler::~DummyDependencyHandler()
{
    blueprint.detach_dependency_handler();
}

void
DummyDependencyHandler::define_object_input(const vespalib::string &name, const vespalib::eval::ValueType &type)
{
    object_type_map.emplace(name, FeatureType::object(type));
}

const FeatureType &
DummyDependencyHandler::resolve_input(const vespalib::string &feature_name, Blueprint::AcceptInput accept_type)
{
    input.push_back(feature_name);
    accept_input.push_back(accept_type);
    auto pos = object_type_map.find(feature_name);
    if (pos == object_type_map.end()) {
        if (accept_type == Blueprint::AcceptInput::OBJECT) {
            accept_type_mismatch = true;
        }
        return FeatureType::number();
    }
    if (accept_type == Blueprint::AcceptInput::NUMBER) {
        accept_type_mismatch = true;
    }
    return pos->second;
}

void DummyDependencyHandler::define_output(const vespalib::string &output_name, const FeatureType &type)
{
    output.push_back(output_name);
    output_type.push_back(type);
}

} // namespace search::fef::test
} // namespace search::fef
} // namespace search
