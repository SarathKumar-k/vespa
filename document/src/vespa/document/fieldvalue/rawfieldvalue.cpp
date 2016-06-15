// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/document/fieldvalue/rawfieldvalue.h>

#include <vespa/document/util/stringutil.h>

namespace document {

IMPLEMENT_IDENTIFIABLE(RawFieldValue, LiteralFieldValueB);

void
RawFieldValue::printXml(XmlOutputStream& out) const
{
    out << XmlBase64Content()
        << XmlContentWrapper(_value.c_str(), _value.size());
}

void
RawFieldValue::print(std::ostream& out, bool, const std::string&) const
{
    StringUtil::printAsHex(out, _value.c_str(),
                                _value.size());
}

} // document
