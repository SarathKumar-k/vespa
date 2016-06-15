// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "ihopdirective.h"

namespace mbus {

/**
 * This class represents an error directive within a {@link Hop}'s selector. This means to stop whatever is being
 * resolved, and instead return a reply containing a specified error.
 *
 * @author <a href="mailto:simon@yahoo-inc.com">Simon Thoresen</a>
 * @version $Id$
 */
class ErrorDirective : public IHopDirective {
private:
    string _msg;

public:
    /**
     * Constructs a new error directive.
     *
     * @param msg The error message.
     */
    ErrorDirective(const vespalib::stringref &msg);

    /**
     * Returns the error string that is to be assigned to the reply.
     *
     * @return The error string.
     */
    const string &getMessage() const { return _msg; }

    virtual Type getType() const { return TYPE_ERROR; }
    virtual bool matches(const IHopDirective &) const { return false; }
    virtual string toString() const;
    virtual string toDebugString() const;
};

} // mbus

