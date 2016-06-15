// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.orchestrator.policy;

import com.yahoo.vespa.applicationmodel.HostName;
import com.yahoo.vespa.orchestrator.OrchestrationException;

import java.util.List;

public class BatchHostStateChangeDeniedException extends OrchestrationException {
    public BatchHostStateChangeDeniedException(HostName parentHostname,
                                               List<HostName> orderedHostNames,
                                               HostStateChangeDeniedException e) {
        super("Failed to suspend " + orderedHostNames + " with parent host "
                + parentHostname + ": " + e.getMessage(), e);

    }
}
