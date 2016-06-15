// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".proton.documentmetastore.documentmetastoreattribute");
#include "documentmetastoreattribute.h"

namespace proton {

namespace {

const vespalib::string _G_documentMetaStoreName("[documentmetastore]");

}

const vespalib::string &
DocumentMetaStoreAttribute::getFixedName()
{
    return _G_documentMetaStoreName;
}


void
DocumentMetaStoreAttribute::notImplemented() const
{
    throw vespalib::IllegalStateException(
            "The function is not implemented for DocumentMetaStoreAttribute");
}


DocumentMetaStoreAttribute::DocumentMetaStoreAttribute(const vespalib::string &name)
    : NotImplementedAttribute(name, Config(BasicType::NONE))
{
}


DocumentMetaStoreAttribute::~DocumentMetaStoreAttribute()
{
}

}
