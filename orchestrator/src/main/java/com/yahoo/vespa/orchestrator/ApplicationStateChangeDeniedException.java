// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.orchestrator;

/**
 * Exception covering all cases where the state change could not
 * be executed.
 *
 * @author <a href="mailto:smorgrav@yahoo-inc.com">Toby</a>
 */
public class ApplicationStateChangeDeniedException extends Exception {

    final String reason;

    public ApplicationStateChangeDeniedException() {
        this("Unknown");
    }

    public ApplicationStateChangeDeniedException(String reason) {
        super();
        this.reason = reason;
    }
}
