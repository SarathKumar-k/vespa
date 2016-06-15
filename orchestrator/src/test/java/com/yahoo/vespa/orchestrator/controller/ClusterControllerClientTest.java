// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.orchestrator.controller;

import com.yahoo.vespa.jaxrs.client.JaxRsStrategy;
import com.yahoo.vespa.jaxrs.client.LocalPassThroughJaxRsStrategy;
import org.junit.Test;

import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

public class ClusterControllerClientTest {
    private static final String CLUSTER_NAME = "clusterName";
    private static final int STORAGE_NODE_INDEX = 0;

    @Test
    public void correctParametersArePassedThrough() throws Exception {
        final ClusterControllerJaxRsApi clusterControllerApi = mock(ClusterControllerJaxRsApi.class);
        final JaxRsStrategy<ClusterControllerJaxRsApi> strategyMock = new LocalPassThroughJaxRsStrategy<>(clusterControllerApi);
        final ClusterControllerClient clusterControllerClient = new ClusterControllerClientImpl(
                strategyMock,
                CLUSTER_NAME);

        final ClusterControllerState wantedState = ClusterControllerState.MAINTENANCE;

        clusterControllerClient.setNodeState(STORAGE_NODE_INDEX, wantedState);

        final ClusterControllerStateRequest expectedNodeStateRequest = new ClusterControllerStateRequest(
                new ClusterControllerStateRequest.State(wantedState, ClusterControllerClientImpl.REQUEST_REASON),
                ClusterControllerStateRequest.Condition.SAFE);
        verify(clusterControllerApi, times(1))
                .setNodeState(
                        eq(CLUSTER_NAME),
                        eq(STORAGE_NODE_INDEX),
                        eq(expectedNodeStateRequest));
    }
}
