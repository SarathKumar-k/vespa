// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.prelude.fastsearch;

import java.io.IOException;

/**
 * Thrown on communication timeouts
 *
 * @author <a href="mailto:bratseth@yahoo-inc.com">Jon Bratseth</a>
 */
@SuppressWarnings("serial")
public class TimeoutException extends IOException {

    public TimeoutException(String message) {
        super(message);
    }
}

