// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/document/datatype/numericdatatype.h>

namespace document {

IMPLEMENT_IDENTIFIABLE_ABSTRACT(NumericDataType, PrimitiveDataType);

NumericDataType::NumericDataType(Type type)
    : PrimitiveDataType(type)
{
}

void NumericDataType::print(std::ostream& out, bool, const std::string&) const
{
    out << "NumericDataType(" << getName() << ", id " << getId() << ")";
}
}  // namespace document
