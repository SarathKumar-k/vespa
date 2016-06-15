// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/vespalib/stllike/string.h>

namespace vespalib
{

class IFieldBase
{
public:
    virtual ~IFieldBase() { }
    virtual stringref getName() const = 0;
};

class FieldBase : public IFieldBase
{
public:
    FieldBase(stringref name) : _name(name) { }
    virtual stringref getName() const { return _name; }
private:
    string _name;
};

class SerializerCommon
{
protected:
    static FieldBase _unspecifiedField;
    static FieldBase _sizeField;
};

}

