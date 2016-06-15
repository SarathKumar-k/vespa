// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/document/fieldvalue/collectionfieldvalue.h>

namespace document {

IMPLEMENT_IDENTIFIABLE_ABSTRACT(CollectionFieldValue, FieldValue);

CollectionFieldValue::CollectionFieldValue(const CollectionFieldValue& other)
    : FieldValue(other),
      _type(other._type)
{
}

void CollectionFieldValue::verifyType(const CollectionFieldValue& other) const
{
    if (*_type != *other._type) {
        throw vespalib::IllegalArgumentException(
                "Cannot assign value of type " + other.getDataType()->toString()
                + " to value of type " + getDataType()->toString() + ".",
                VESPA_STRLOC);
    }
}

}
