// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
/**
 * @class document::select::Constant
 * @ingroup select
 *
 * @brief Class describing a constant in the select tree.
 *
 * @author H�kon Humberset
 * @date 2005-06-07
 * @version $Id$
 */

#pragma once

#include "node.h"

namespace document {
namespace select {

class Constant : public Node
{
private:
    bool _value;

public:
    explicit Constant(const vespalib::stringref & value);

    virtual ResultList
    contains(const Context&) const
    {
        return ResultList(Result::get(_value));
    }

    virtual ResultList
    trace(const Context&, std::ostream& trace) const;
    virtual void print(std::ostream& out, bool verbose,
                       const std::string& indent) const;
    virtual void visit(Visitor& v) const;

    bool getConstantValue() const { return _value; }

    Node::UP clone() const { return wrapParens(new Constant(_name)); }

};

} // select
} // document

