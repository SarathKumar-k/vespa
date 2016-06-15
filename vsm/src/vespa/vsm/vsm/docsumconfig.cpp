// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
// Copyright (C) 1998-2003 Fast Search & Transfer ASA
// Copyright (C) 2003 Overture Services Norway AS

#include <vespa/fastos/fastos.h>
#include <vespa/vsm/vsm/docsumconfig.h>

using search::docsummary::IDocsumFieldWriter;
using search::docsummary::EmptyDFW;

namespace vsm {

IDocsumFieldWriter::UP
DynamicDocsumConfig::createFieldWriter(const string & fieldName, const string & overrideName, const string & argument, bool & rc)
{
    IDocsumFieldWriter::UP fieldWriter;
    if ((overrideName == "staticrank") ||
        (overrideName == "ranklog") ||
        (overrideName == "label") ||
        (overrideName == "project") ||
        (overrideName == "positions") ||
        (overrideName == "absdist") ||
        (overrideName == "subproject"))
    {
        fieldWriter.reset(new EmptyDFW());
        rc = true;
    } else if ((overrideName == "attribute") ||
               ((overrideName == "geopos"))) {
        rc = true;
    } else {
        fieldWriter = search::docsummary::DynamicDocsumConfig::createFieldWriter(fieldName, overrideName, argument, rc);
    }
    return fieldWriter;
}

}
