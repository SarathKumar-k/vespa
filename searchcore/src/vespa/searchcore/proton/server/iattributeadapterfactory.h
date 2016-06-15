// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/searchcore/proton/attribute/attribute_collection_spec.h>
#include <vespa/searchcore/proton/attribute/i_attribute_writer.h>

namespace proton {

struct IAttributeAdapterFactory
{
    typedef std::unique_ptr<IAttributeAdapterFactory> UP;
    virtual ~IAttributeAdapterFactory() {}
    virtual IAttributeWriter::SP create(const IAttributeWriter::SP &old,
            const AttributeCollectionSpec &attrSpec) const = 0;
};

} // namespace proton

