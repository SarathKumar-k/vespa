// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.logserver.filter;

import com.yahoo.log.LogMessage;

/**
 *
 *
 * @author  <a href="mailto:borud@yahoo-inc.com">Bjorn Borud</a>
 */
public class NullFilter implements LogFilter {
    public boolean isLoggable (LogMessage msg) {
        return true;
    }

    public String description () {
        return "Match all log messages";
    }
}
