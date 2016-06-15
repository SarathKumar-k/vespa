// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.clustercontroller.core;

import com.yahoo.vdslib.distribution.ConfiguredNode;
import com.yahoo.vdslib.distribution.Distribution;
import com.yahoo.vdslib.distribution.Group;
import com.yahoo.vdslib.state.Node;
import com.yahoo.vdslib.state.NodeState;
import com.yahoo.vdslib.state.NodeType;
import com.yahoo.vdslib.state.State;
import com.yahoo.vespa.clustercontroller.core.hostinfo.HostInfo;
import com.yahoo.vespa.clustercontroller.utils.staterestapi.requests.SetUnitStateRequest;
import org.junit.Before;
import org.junit.Test;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import static org.hamcrest.core.StringContains.containsString;
import static org.hamcrest.core.Is.is;
import static org.junit.Assert.*;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class NodeStateChangeCheckerTest {

    private static final int minStorageNodesUp = 3;
    private static final int requiredRedundancy = 4;
    private static final int currentClusterState = 2;
    private static final double minRatioOfStorageNodesUp = 0.9;

    private static final Node nodeDistributor = new Node(NodeType.DISTRIBUTOR, 1);
    private static final Node nodeStorage = new Node(NodeType.STORAGE, 1);

    private static final NodeState upNodeState = new NodeState(NodeType.STORAGE, State.UP);
    public static final NodeState maintenanceNodeState = createNodeState(State.MAINTENANCE, "Orchestrator");

    private static NodeState createNodeState(State state, String description) {
        return new NodeState(NodeType.STORAGE, state).setDescription(description);
    }

    private NodeStateChangeChecker createChangeChecker(ContentCluster cluster) {
        return new NodeStateChangeChecker(minStorageNodesUp, minRatioOfStorageNodesUp, requiredRedundancy, cluster.clusterInfo());
    }

    private ContentCluster createCluster(Collection<ConfiguredNode> nodes) {
        Distribution distribution = mock(Distribution.class);
        Group group = new Group(2, "to");
        when(distribution.getRootGroup()).thenReturn(group);
        return new ContentCluster("Clustername", nodes, distribution, minStorageNodesUp, 0.0);
    }

    private StorageNodeInfo createStorageNodeInfo(int index, State state) {
        Distribution distribution = mock(Distribution.class);
        Group group = new Group(2, "to");
        when(distribution.getRootGroup()).thenReturn(group);

        String clusterName = "Clustername";
        Set<ConfiguredNode> configuredNodeIndexes = new HashSet<>();
        ContentCluster cluster = new ContentCluster(clusterName, configuredNodeIndexes, distribution, minStorageNodesUp, 0.0);

        String rpcAddress = "";
        StorageNodeInfo storageNodeInfo = new StorageNodeInfo(cluster, index, false, rpcAddress, distribution);
        storageNodeInfo.setReportedState(new NodeState(NodeType.STORAGE, state), 3 /* time */);
        return storageNodeInfo;
    }

    private String createDistributorHostInfo(int replicationfactor1, int replicationfactor2, int replicationfactor3) {
        return "{\n" +
                "    \"cluster-state-version\": 2,\n" +
                "    \"distributor\": {\n" +
                "        \"storage-nodes\": [\n" +
                "            {\n" +
                "                \"node-index\": 0,\n" +
                "                \"min-current-replication-factor\": " + replicationfactor1 + "\n" +
                "            },\n" +
                "            {\n" +
                "                \"node-index\": 1,\n" +
                "                \"min-current-replication-factor\": " + replicationfactor2 + "\n" +
                "            },\n" +
                "            {\n" +
                "                \"node-index\": 2,\n" +
                "                \"min-current-replication-factor\": " + replicationfactor3 + "\n" +
                "            },\n" +
                "            {\n" +
                "                \"node-index\": 3\n" +
                "            }\n" +
                "        ]\n" +
                "    }\n" +
                "}\n";
    }

    @Test
    public void testCanUpgradeForce() {
        NodeStateChangeChecker nodeStateChangeChecker = createChangeChecker(createCluster(createNodes(1)));
        NodeState newState = new NodeState(NodeType.STORAGE, State.INITIALIZING);
        NodeStateChangeChecker.Result result = nodeStateChangeChecker.evaluateTransition(
                nodeDistributor, currentClusterState, SetUnitStateRequest.Condition.FORCE,
                upNodeState, newState);
        assertTrue(result.settingWantedStateIsAllowed());
        assertTrue(!result.wantedStateAlreadySet());
    }

    @Test
    public void testSafeSetStateDistributors() {
        NodeStateChangeChecker nodeStateChangeChecker = createChangeChecker(createCluster(createNodes(1)));
        NodeStateChangeChecker.Result result = nodeStateChangeChecker.evaluateTransition(
                nodeDistributor, currentClusterState, SetUnitStateRequest.Condition.SAFE,
                upNodeState, maintenanceNodeState);
        assertFalse(result.settingWantedStateIsAllowed());
        assertFalse(result.wantedStateAlreadySet());
        assertThat(result.getReason(), containsString("Safe-set of node state is only supported for storage nodes"));
    }

    @Test
    public void testCanUpgradeSafeMissingStorage() {
        ContentCluster cluster = createCluster(createNodes(4));
        setAllNodesUp(cluster, HostInfo.createHostInfo(createDistributorHostInfo(4, 5, 6)));
        NodeStateChangeChecker nodeStateChangeChecker = new NodeStateChangeChecker(
                5 /* min storage nodes */, minRatioOfStorageNodesUp, requiredRedundancy, cluster.clusterInfo());
        NodeStateChangeChecker.Result result = nodeStateChangeChecker.evaluateTransition(
                nodeStorage, currentClusterState, SetUnitStateRequest.Condition.SAFE,
                upNodeState, maintenanceNodeState);
        assertFalse(result.settingWantedStateIsAllowed());
        assertFalse(result.wantedStateAlreadySet());
        assertThat(result.getReason(), is("There are only 4 storage nodes up, while config requires at least 5"));
    }

    @Test
    public void testCanUpgradeStorageSafeYes() {
        NodeStateChangeChecker.Result result = transitionToMaintenanceWithNoStorageNodesDown();
        assertTrue(result.settingWantedStateIsAllowed());
        assertFalse(result.wantedStateAlreadySet());
    }

    @Test
    public void testSetUpFailsIfReportedIsDown() {
        ContentCluster cluster = createCluster(createNodes(4));
        NodeStateChangeChecker nodeStateChangeChecker = createChangeChecker(cluster);
        // Not setting nodes up -> all are down

        NodeStateChangeChecker.Result result = nodeStateChangeChecker.evaluateTransition(
                nodeStorage, currentClusterState, SetUnitStateRequest.Condition.SAFE,
                maintenanceNodeState, upNodeState);
        assertFalse(result.settingWantedStateIsAllowed());
        assertFalse(result.wantedStateAlreadySet());
    }

    @Test
    public void testCannotSetUpIfUnknownOldStateAndReportedIsDown() {
        ContentCluster cluster = createCluster(createNodes(4));
        NodeStateChangeChecker nodeStateChangeChecker = createChangeChecker(cluster);
        // Not setting nodes up -> all are down

        NodeStateChangeChecker.Result result = nodeStateChangeChecker.evaluateTransition(
                nodeStorage, currentClusterState, SetUnitStateRequest.Condition.SAFE,
                new NodeState(NodeType.STORAGE, State.DOWN), upNodeState);
        assertFalse(result.settingWantedStateIsAllowed());
        assertFalse(result.wantedStateAlreadySet());
        assertThat(result.getReason(), is("Refusing to set wanted state to up when it is currently in Down"));
    }

    @Test
    public void testCanUpgradeStorageSafeNo() {
        ContentCluster cluster = createCluster(createNodes(4));
        NodeStateChangeChecker nodeStateChangeChecker = createChangeChecker(cluster);
        setAllNodesUp(cluster, HostInfo.createHostInfo(createDistributorHostInfo(4, 3, 6)));

        NodeStateChangeChecker.Result result = nodeStateChangeChecker.evaluateTransition(
                nodeStorage, currentClusterState, SetUnitStateRequest.Condition.SAFE,
                upNodeState, maintenanceNodeState);
        assertFalse(result.settingWantedStateIsAllowed());
        assertFalse(result.wantedStateAlreadySet());
        assertThat(result.getReason(), is("Distributor 0 says storage node 1 " +
                "has buckets with redundancy as low as 3, but we require at least 4"));
    }

    @Test
    public void testCanUpgradeIfMissingMinReplicationFactor() {
        ContentCluster cluster = createCluster(createNodes(4));
        NodeStateChangeChecker nodeStateChangeChecker = createChangeChecker(cluster);
        setAllNodesUp(cluster, HostInfo.createHostInfo(createDistributorHostInfo(4, 3, 6)));

        NodeStateChangeChecker.Result result = nodeStateChangeChecker.evaluateTransition(
                new Node(NodeType.STORAGE, 3), currentClusterState, SetUnitStateRequest.Condition.SAFE,
                upNodeState, maintenanceNodeState);
        assertTrue(result.settingWantedStateIsAllowed());
        assertFalse(result.wantedStateAlreadySet());
    }

    @Test
    public void testCanUpgradeIfStorageNodeMissingFromNodeInfo() {
        ContentCluster cluster = createCluster(createNodes(4));
        NodeStateChangeChecker nodeStateChangeChecker = createChangeChecker(cluster);
        String hostInfo = "{\n" +
                "    \"cluster-state-version\": 2,\n" +
                "    \"distributor\": {\n" +
                "        \"storage-nodes\": [\n" +
                "            {\n" +
                "                \"node-index\": 0,\n" +
                "                \"min-current-replication-factor\": " + requiredRedundancy + "\n" +
                "            }\n" +
                "        ]\n" +
                "    }\n" +
                "}\n";
        setAllNodesUp(cluster, HostInfo.createHostInfo(hostInfo));

        NodeStateChangeChecker.Result result = nodeStateChangeChecker.evaluateTransition(
                new Node(NodeType.STORAGE, 1), currentClusterState, SetUnitStateRequest.Condition.SAFE,
                upNodeState, maintenanceNodeState);
        assertTrue(result.settingWantedStateIsAllowed());
        assertFalse(result.wantedStateAlreadySet());
    }

    @Test
    public void testMissingDistributorState() {
        ContentCluster cluster = createCluster(createNodes(4));
        NodeStateChangeChecker nodeStateChangeChecker = createChangeChecker(cluster);
        cluster.clusterInfo().getStorageNodeInfo(1).setReportedState(new NodeState(NodeType.STORAGE, State.UP), 0);

        NodeStateChangeChecker.Result result = nodeStateChangeChecker.evaluateTransition(
                nodeStorage, currentClusterState, SetUnitStateRequest.Condition.SAFE, upNodeState, maintenanceNodeState);
        assertFalse(result.settingWantedStateIsAllowed());
        assertFalse(result.wantedStateAlreadySet());
        assertThat(result.getReason(), is("Distributor node (0) has not reported any cluster state version yet."));
    }

    private NodeStateChangeChecker.Result transitionToSameState(State state, String oldDescription, String newDescription) {
        ContentCluster cluster = createCluster(createNodes(4));
        NodeStateChangeChecker nodeStateChangeChecker = createChangeChecker(cluster);

        NodeState currentNodeState = createNodeState(state, oldDescription);
        NodeState newNodeState = createNodeState(state, newDescription);
        return nodeStateChangeChecker.evaluateTransition(
                nodeStorage, currentClusterState, SetUnitStateRequest.Condition.SAFE,
                currentNodeState, newNodeState);
    }

    private NodeStateChangeChecker.Result transitionToSameState(String oldDescription, String newDescription) {
        return transitionToSameState(State.MAINTENANCE, oldDescription, newDescription);
    }

    @Test
    public void testSettingUpWhenUpCausesAlreadySet() {
        NodeStateChangeChecker.Result result = transitionToSameState(State.UP, "foo", "bar");
        assertTrue(result.wantedStateAlreadySet());
    }

    @Test
    public void testSettingAlreadySetState() {
        NodeStateChangeChecker.Result result = transitionToSameState("foo", "foo");
        assertFalse(result.settingWantedStateIsAllowed());
        assertTrue(result.wantedStateAlreadySet());
    }

    @Test
    public void testDifferentDescriptionImpliesAlreadySet() {
        NodeStateChangeChecker.Result result = transitionToSameState("foo", "bar");
        assertFalse(result.settingWantedStateIsAllowed());
        assertTrue(result.wantedStateAlreadySet());
    }

    private NodeStateChangeChecker.Result transitionToMaintenanceWithOneStorageNodeDown(
            int storageNodeIndex, boolean alternatingUpRetiredAndInitializing) {
        ContentCluster cluster = createCluster(createNodes(4));
        NodeStateChangeChecker nodeStateChangeChecker = createChangeChecker(cluster);

        for (int x = 0; x < cluster.clusterInfo().getConfiguredNodes().size(); x++) {
            State state = State.UP;
            // Pick some retired and initializing nodes too
            if (alternatingUpRetiredAndInitializing) { // TODO: Move this into the calling test
                if (x % 3 == 1) state = State.RETIRED;
                else if (x % 3 == 2) state = State.INITIALIZING;
            }
            cluster.clusterInfo().getDistributorNodeInfo(x).setReportedState(new NodeState(NodeType.DISTRIBUTOR, state), 0);
            cluster.clusterInfo().getDistributorNodeInfo(x).setHostInfo(HostInfo.createHostInfo(createDistributorHostInfo(4, 5, 6)));
            cluster.clusterInfo().getStorageNodeInfo(x).setReportedState(new NodeState(NodeType.STORAGE, state), 0);
        }

        if (storageNodeIndex >= 0) { // TODO: Move this into the calling test
            NodeState downNodeState = new NodeState(NodeType.STORAGE, State.DOWN);
            cluster.clusterInfo().getStorageNodeInfo(storageNodeIndex).setReportedState(downNodeState, 4 /* time */);
        }

        return nodeStateChangeChecker.evaluateTransition(
                nodeStorage, currentClusterState, SetUnitStateRequest.Condition.SAFE, upNodeState, maintenanceNodeState);
    }

    private void setAllNodesUp(ContentCluster cluster, HostInfo distributorHostInfo) {
        for (int x = 0; x < cluster.clusterInfo().getConfiguredNodes().size(); x++) {
            State state = State.UP;
            cluster.clusterInfo().getDistributorNodeInfo(x).setReportedState(new NodeState(NodeType.DISTRIBUTOR, state), 0);
            cluster.clusterInfo().getDistributorNodeInfo(x).setHostInfo(distributorHostInfo);
            cluster.clusterInfo().getStorageNodeInfo(x).setReportedState(new NodeState(NodeType.STORAGE, state), 0);
        }
    }

    private NodeStateChangeChecker.Result transitionToMaintenanceWithOneStorageNodeDown(int storageNodeIndex) {
        return transitionToMaintenanceWithOneStorageNodeDown(storageNodeIndex, false);
    }

    private NodeStateChangeChecker.Result transitionToMaintenanceWithNoStorageNodesDown() {
        return transitionToMaintenanceWithOneStorageNodeDown(-1, false);
    }

    @Test
    public void testCanUpgradeWhenAllUp() {
        NodeStateChangeChecker.Result result = transitionToMaintenanceWithNoStorageNodesDown();
        assertTrue(result.settingWantedStateIsAllowed());
        assertFalse(result.wantedStateAlreadySet());
    }

    @Test
    public void testCanUpgradeWhenAllUpOrRetired() {
        NodeStateChangeChecker.Result result = transitionToMaintenanceWithNoStorageNodesDown();
        assertTrue(result.settingWantedStateIsAllowed());
        assertFalse(result.wantedStateAlreadySet());
    }

    @Test
    public void testCanUpgradeWhenStorageIsDown() {
        NodeStateChangeChecker.Result result = transitionToMaintenanceWithOneStorageNodeDown(nodeStorage.getIndex());
        assertTrue(result.settingWantedStateIsAllowed());
        assertFalse(result.wantedStateAlreadySet());
    }

    @Test
    public void testCannotUpgradeWhenOtherStorageIsDown() {
        int otherIndex = 2;
        // If this fails, just set otherIndex to some other valid index.
        assertNotEquals(nodeStorage.getIndex(), otherIndex);

        NodeStateChangeChecker.Result result = transitionToMaintenanceWithOneStorageNodeDown(otherIndex);
        assertFalse(result.settingWantedStateIsAllowed());
        assertFalse(result.wantedStateAlreadySet());
        assertThat(result.getReason(), containsString("Not enough storage nodes running"));
    }

    private List<ConfiguredNode> createNodes(int count) {
        List<ConfiguredNode> nodes = new ArrayList<>();
        for (int i = 0; i < count; i++)
            nodes.add(new ConfiguredNode(i, false));
        return nodes;
    }

}