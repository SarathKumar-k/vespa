// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.node.admin;

import com.yahoo.vespa.applicationmodel.HostName;
import com.yahoo.vespa.hosted.node.admin.docker.Container;
import com.yahoo.vespa.hosted.node.admin.docker.ContainerName;
import com.yahoo.vespa.hosted.node.admin.docker.Docker;
import com.yahoo.vespa.hosted.node.admin.docker.DockerImage;
import com.yahoo.vespa.hosted.node.admin.docker.ProcessResult;
import com.yahoo.vespa.hosted.node.admin.noderepository.NodeRepository;
import com.yahoo.vespa.hosted.node.admin.noderepository.NodeState;
import com.yahoo.vespa.hosted.node.admin.orchestrator.Orchestrator;
import com.yahoo.vespa.hosted.node.admin.orchestrator.OrchestratorException;
import org.junit.Test;
import org.mockito.InOrder;

import java.io.IOException;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;

import static org.hamcrest.core.Is.is;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyDouble;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.anyVararg;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

/**
 * @author bakksjo
 */
public class NodeAgentImplTest {
    private static final Optional<Double> MIN_CPU_CORES = Optional.of(1.0);
    private static final Optional<Double> MIN_MAIN_MEMORY_AVAILABLE_GB = Optional.of(1.0);
    private static final Optional<Double> MIN_DISK_AVAILABLE_GB = Optional.of(1.0);
    
    private static final Optional<Container> NO_CONTAINER = Optional.empty();

    private static final ProcessResult NODE_PROGRAM_DOESNT_EXIST = new ProcessResult(1, "");

    private final HostName hostName = new HostName("hostname");
    private final Docker docker = mock(Docker.class);
    private final NodeRepository nodeRepository = mock(NodeRepository.class);
    private final Orchestrator orchestrator = mock(Orchestrator.class);

    private final NodeAgentImpl nodeAgent = new NodeAgentImpl(hostName, docker, nodeRepository, orchestrator);

    @Test
    public void upToDateContainerIsUntouched() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.ACTIVE,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final boolean isRunning = true;
        final Container existingContainer = new Container(hostName, dockerImage, containerName, isRunning);
        final String vespaVersion = "7.8.9";
        
        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);
        when(docker.executeInContainer(eq(containerName), anyVararg())).thenReturn(NODE_PROGRAM_DOESNT_EXIST);
        when(docker.getVespaVersion(containerName)).thenReturn(vespaVersion);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, Optional.of(existingContainer));

        verify(orchestrator, never()).suspend(any(HostName.class));
        verify(docker, never()).stopContainer(any(ContainerName.class));
        verify(docker, never()).deleteContainer(any(ContainerName.class));
        verify(docker, never()).startContainer(
                any(DockerImage.class),
                any(HostName.class),
                any(ContainerName.class),
                anyDouble(),
                anyDouble(),
                anyDouble());
        verify(docker, times(1)).executeInContainer(any(), anyVararg());
        final InOrder inOrder = inOrder(orchestrator, nodeRepository);
        inOrder.verify(nodeRepository).updateNodeAttributes(hostName, restartGeneration, dockerImage, vespaVersion);
        inOrder.verify(orchestrator).resume(hostName);
    }

    @Test
    public void newRestartGenerationCausesRestart() throws Exception {
        final long wantedRestartGeneration = 2;
        final long currentRestartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.ACTIVE,
                Optional.of(wantedRestartGeneration),
                Optional.of(currentRestartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final boolean isRunning = true;
        final Container existingContainer = new Container(hostName, dockerImage, containerName, isRunning);
        final String vespaVersion = "7.8.9";

        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);
        when(docker.executeInContainer(eq(containerName), anyVararg())).thenReturn(NODE_PROGRAM_DOESNT_EXIST);
        when(docker.getVespaVersion(containerName)).thenReturn(vespaVersion);
        when(orchestrator.suspend(any(HostName.class))).thenReturn(true);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, Optional.of(existingContainer));

        final InOrder inOrder = inOrder(orchestrator, docker, nodeRepository);
        inOrder.verify(orchestrator).suspend(hostName);
        inOrder.verify(docker, times(1)).executeInContainer(any(), anyVararg());
        inOrder.verify(docker).stopContainer(containerName);
        inOrder.verify(docker).deleteContainer(containerName);
        inOrder.verify(docker).startContainer(
                nodeSpec.wantedDockerImage.get(),
                nodeSpec.hostname,
                nodeSpec.containerName,
                nodeSpec.minCpuCores.get(),
                nodeSpec.minDiskAvailableGb.get(),
                nodeSpec.minMainMemoryAvailableGb.get());
        inOrder.verify(docker, times(1)).executeInContainer(any(), anyVararg());
        inOrder.verify(nodeRepository).updateNodeAttributes(hostName, wantedRestartGeneration, dockerImage, vespaVersion);
        inOrder.verify(orchestrator).resume(hostName);
    }

    @Test
    public void newDockerImageCausesRestart() throws Exception {
        final long restartGeneration = 1;
        final DockerImage currentDockerImage = new DockerImage("currentDockerImage");
        final DockerImage wantedDockerImage = new DockerImage("wantedDockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(wantedDockerImage),
                containerName,
                NodeState.ACTIVE,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final boolean isRunning = true;
        final Container existingContainer = new Container(hostName, currentDockerImage, containerName, isRunning);
        final String vespaVersion = "7.8.9";

        when(docker.imageIsDownloaded(wantedDockerImage)).thenReturn(true);
        when(docker.executeInContainer(eq(containerName), anyVararg())).thenReturn(NODE_PROGRAM_DOESNT_EXIST);;
        when(docker.getVespaVersion(containerName)).thenReturn(vespaVersion);
        when(orchestrator.suspend(any(HostName.class))).thenReturn(true);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, Optional.of(existingContainer));

        final InOrder inOrder = inOrder(orchestrator, docker, nodeRepository);
        inOrder.verify(orchestrator).suspend(hostName);
        inOrder.verify(docker).stopContainer(containerName);
        inOrder.verify(docker).deleteContainer(containerName);
        inOrder.verify(docker).startContainer(
                nodeSpec.wantedDockerImage.get(),
                nodeSpec.hostname,
                nodeSpec.containerName,
                nodeSpec.minCpuCores.get(),
                nodeSpec.minDiskAvailableGb.get(),
                nodeSpec.minMainMemoryAvailableGb.get());
        inOrder.verify(docker, times(1)).executeInContainer(any(), anyVararg());
        inOrder.verify(nodeRepository).updateNodeAttributes(hostName, restartGeneration, wantedDockerImage, vespaVersion);
        inOrder.verify(orchestrator).resume(hostName);
    }

    @Test
    public void containerIsNotStoppedIfNewImageMustBePulled() throws Exception {
        final ContainerName containerName = new ContainerName("container");
        final DockerImage oldDockerImage = new DockerImage("old-image");
        final DockerImage newDockerImage = new DockerImage("new-image");
        final long wantedRestartGeneration = 2;
        final long currentRestartGeneration = 1;
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(newDockerImage),
                containerName,
                NodeState.ACTIVE,
                Optional.of(wantedRestartGeneration),
                Optional.of(currentRestartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final Container existingContainer = new Container(hostName, oldDockerImage, containerName, true);

        when(docker.imageIsDownloaded(newDockerImage)).thenReturn(false);
        when(docker.pullImageAsync(newDockerImage)).thenReturn(new CompletableFuture<>());

        nodeAgent.synchronizeLocalContainerState(nodeSpec, Optional.of(existingContainer));

        verify(docker, never()).stopContainer(containerName);
        verify(docker).pullImageAsync(newDockerImage);
    }

    @Test
    public void stoppedContainerIsRestarted() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.ACTIVE,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final boolean isRunning = false;
        final Container existingContainer = new Container(hostName, dockerImage, containerName, isRunning);
        final String vespaVersion = "7.8.9";

        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);
        when(docker.executeInContainer(eq(containerName), anyVararg())).thenReturn(NODE_PROGRAM_DOESNT_EXIST);
        when(docker.getVespaVersion(containerName)).thenReturn(vespaVersion);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, Optional.of(existingContainer));

        verify(docker, never()).stopContainer(any(ContainerName.class));
        verify(orchestrator, never()).suspend(any(HostName.class));
        verify(docker, times(1)).executeInContainer(any(), anyVararg());
        final InOrder inOrder = inOrder(orchestrator, docker, nodeRepository);
        inOrder.verify(docker).deleteContainer(containerName);
        inOrder.verify(docker).startContainer(
                nodeSpec.wantedDockerImage.get(),
                nodeSpec.hostname,
                nodeSpec.containerName,
                nodeSpec.minCpuCores.get(),
                nodeSpec.minDiskAvailableGb.get(),
                nodeSpec.minMainMemoryAvailableGb.get());
        inOrder.verify(nodeRepository).updateNodeAttributes(hostName, restartGeneration, dockerImage, vespaVersion);
        inOrder.verify(orchestrator).resume(hostName);
    }

    @Test
    public void missingContainerIsStarted() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.ACTIVE,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final String vespaVersion = "7.8.9";

        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);
        when(docker.executeInContainer(eq(containerName), anyVararg())).thenReturn(NODE_PROGRAM_DOESNT_EXIST);
        when(docker.getVespaVersion(containerName)).thenReturn(vespaVersion);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, NO_CONTAINER);

        verify(docker, never()).stopContainer(any(ContainerName.class));
        verify(docker, never()).deleteContainer(any(ContainerName.class));
        verify(docker, times(1)).executeInContainer(any(), anyVararg());
        verify(orchestrator, never()).suspend(any(HostName.class));
        final InOrder inOrder = inOrder(orchestrator, docker, nodeRepository);
        inOrder.verify(docker).startContainer(
                nodeSpec.wantedDockerImage.get(),
                nodeSpec.hostname,
                nodeSpec.containerName,
                nodeSpec.minCpuCores.get(),
                nodeSpec.minDiskAvailableGb.get(),
                nodeSpec.minMainMemoryAvailableGb.get());
        inOrder.verify(nodeRepository).updateNodeAttributes(hostName, restartGeneration, dockerImage, vespaVersion);
        inOrder.verify(orchestrator).resume(hostName);
    }

    @Test
    public void noRestartIfOrchestratorSuspendFails() throws Exception {
        final long wantedRestartGeneration = 2;
        final long currentRestartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.ACTIVE,
                Optional.of(wantedRestartGeneration),
                Optional.of(currentRestartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final boolean isRunning = true;
        final Container existingContainer = new Container(hostName, dockerImage, containerName, isRunning);

        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);
        when(orchestrator.suspend(any(HostName.class))).thenReturn(false);

        try {
            nodeAgent.synchronizeLocalContainerState(nodeSpec, Optional.of(existingContainer));
            fail("permission to suspend should fail so we should never get here");
        } catch (OrchestratorException e) {
            // expected
        }

        verify(orchestrator).suspend(hostName);
        verify(docker, never()).stopContainer(any(ContainerName.class));
        verify(docker, never()).deleteContainer(any(ContainerName.class));
        verify(docker, never()).startContainer(
                any(DockerImage.class),
                any(HostName.class),
                any(ContainerName.class),
                anyDouble(),
                anyDouble(),
                anyDouble());
        verify(orchestrator, never()).resume(any(HostName.class));
        verify(nodeRepository, never()).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());
    }

    @Test
    public void failedNodeRunningContainerIsTakenDown() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.FAILED,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final boolean isRunning = true;
        final Container existingContainer = new Container(hostName, dockerImage, containerName, isRunning);

        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, Optional.of(existingContainer));

        verify(orchestrator, never()).suspend(any(HostName.class));
        final InOrder inOrder = inOrder(orchestrator, docker);
        inOrder.verify(docker).stopContainer(containerName);
        inOrder.verify(docker).deleteContainer(containerName);
        verify(docker, never()).startContainer(
                any(DockerImage.class),
                any(HostName.class),
                any(ContainerName.class),
                anyDouble(),
                anyDouble(),
                anyDouble());
        verify(docker, never()).deleteApplicationStorage(any(ContainerName.class));
        verify(orchestrator, never()).resume(any(HostName.class));
        verify(nodeRepository, never()).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());
    }

    @Test
    public void failedNodeStoppedContainerIsTakenDown() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.FAILED,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final boolean isRunning = false;
        final Container existingContainer = new Container(hostName, dockerImage, containerName, isRunning);

        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, Optional.of(existingContainer));

        verify(orchestrator, never()).suspend(any(HostName.class));
        verify(docker, never()).stopContainer(any(ContainerName.class));
        verify(docker).deleteContainer(containerName);
        verify(docker, never()).startContainer(
                any(DockerImage.class),
                any(HostName.class),
                any(ContainerName.class),
                anyDouble(),
                anyDouble(),
                anyDouble());
        verify(docker, never()).deleteApplicationStorage(any(ContainerName.class));
        verify(orchestrator, never()).resume(any(HostName.class));
        verify(nodeRepository, never()).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());
    }

    @Test
    public void failedNodeNoContainerNoActionTaken() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.FAILED,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);

        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, NO_CONTAINER);

        verify(orchestrator, never()).suspend(any(HostName.class));
        verify(docker, never()).stopContainer(any(ContainerName.class));
        verify(docker, never()).deleteContainer(containerName);
        verify(docker, never()).startContainer(
                any(DockerImage.class),
                any(HostName.class),
                any(ContainerName.class),
                anyDouble(),
                anyDouble(),
                anyDouble());
        verify(docker, never()).deleteApplicationStorage(any(ContainerName.class));
        verify(orchestrator, never()).resume(any(HostName.class));
        verify(nodeRepository, never()).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());
    }

    @Test
    public void inactiveNodeRunningContainerIsTakenDown() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.INACTIVE,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final boolean isRunning = true;
        final Container existingContainer = new Container(hostName, dockerImage, containerName, isRunning);

        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, Optional.of(existingContainer));

        verify(orchestrator, never()).suspend(any(HostName.class));
        final InOrder inOrder = inOrder(orchestrator, docker);
        inOrder.verify(docker).stopContainer(containerName);
        inOrder.verify(docker).deleteContainer(containerName);
        verify(docker, never()).startContainer(
                any(DockerImage.class),
                any(HostName.class),
                any(ContainerName.class),
                anyDouble(),
                anyDouble(),
                anyDouble());
        verify(docker, never()).deleteApplicationStorage(any(ContainerName.class));
        verify(orchestrator, never()).resume(any(HostName.class));
        verify(nodeRepository, never()).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());
    }

    @Test
    public void inactiveNodeStoppedContainerIsTakenDown() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.INACTIVE,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final boolean isRunning = false;
        final Container existingContainer = new Container(hostName, dockerImage, containerName, isRunning);

        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, Optional.of(existingContainer));

        verify(orchestrator, never()).suspend(any(HostName.class));
        verify(docker, never()).stopContainer(any(ContainerName.class));
        verify(docker).deleteContainer(containerName);
        verify(docker, never()).startContainer(
                any(DockerImage.class),
                any(HostName.class),
                any(ContainerName.class),
                anyDouble(),
                anyDouble(),
                anyDouble());
        verify(docker, never()).deleteApplicationStorage(any(ContainerName.class));
        verify(orchestrator, never()).resume(any(HostName.class));
        verify(nodeRepository, never()).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());
    }

    @Test
    public void inactiveNodeNoContainerNoActionTaken() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.INACTIVE,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);

        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, NO_CONTAINER);

        verify(orchestrator, never()).suspend(any(HostName.class));
        verify(docker, never()).stopContainer(any(ContainerName.class));
        verify(docker, never()).deleteContainer(any(ContainerName.class));
        verify(docker, never()).startContainer(
                any(DockerImage.class),
                any(HostName.class),
                any(ContainerName.class),
                anyDouble(),
                anyDouble(),
                anyDouble());
        verify(docker, never()).deleteApplicationStorage(any(ContainerName.class));
        verify(orchestrator, never()).resume(any(HostName.class));
        verify(nodeRepository, never()).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());
    }

    @Test
    public void dirtyNodeRunningContainerIsTakenDownAndCleanedAndRecycled() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.DIRTY,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final boolean isRunning = true;
        final Container existingContainer = new Container(hostName, dockerImage, containerName, isRunning);

        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, Optional.of(existingContainer));

        verify(orchestrator, never()).suspend(any(HostName.class));
        final InOrder inOrder = inOrder(orchestrator, docker, nodeRepository);
        inOrder.verify(docker).stopContainer(containerName);
        inOrder.verify(docker).deleteContainer(containerName);
        inOrder.verify(docker).deleteApplicationStorage(containerName);
        inOrder.verify(nodeRepository).markAsReady(hostName);
        verify(docker, never()).startContainer(
                any(DockerImage.class),
                any(HostName.class),
                any(ContainerName.class),
                anyDouble(),
                anyDouble(),
                anyDouble());
        verify(orchestrator, never()).resume(any(HostName.class));
        verify(nodeRepository, never()).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());
    }

    @Test
    public void dirtyNodeStoppedContainerIsTakenDownAndCleanedAndRecycled() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.DIRTY,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final boolean isRunning = false;
        final Container existingContainer = new Container(hostName, dockerImage, containerName, isRunning);

        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, Optional.of(existingContainer));

        verify(orchestrator, never()).suspend(any(HostName.class));
        verify(docker, never()).stopContainer(any(ContainerName.class));
        final InOrder inOrder = inOrder(orchestrator, docker, nodeRepository);
        inOrder.verify(docker).deleteContainer(containerName);
        inOrder.verify(docker).deleteApplicationStorage(containerName);
        inOrder.verify(nodeRepository).markAsReady(hostName);
        verify(docker, never()).startContainer(
                any(DockerImage.class),
                any(HostName.class),
                any(ContainerName.class),
                anyDouble(),
                anyDouble(),
                anyDouble());
        verify(orchestrator, never()).resume(any(HostName.class));
        verify(nodeRepository, never()).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());
    }

    @Test
    public void dirtyNodeWithNoContainerIsCleanedAndRecycled() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.DIRTY,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);

        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, NO_CONTAINER);

        verify(orchestrator, never()).suspend(any(HostName.class));
        verify(docker, never()).stopContainer(any(ContainerName.class));
        verify(docker, never()).deleteContainer(any(ContainerName.class));
        final InOrder inOrder = inOrder(docker, nodeRepository);
        inOrder.verify(docker).deleteApplicationStorage(containerName);
        inOrder.verify(nodeRepository).markAsReady(hostName);
        verify(docker, never()).startContainer(
                any(DockerImage.class),
                any(HostName.class),
                any(ContainerName.class),
                anyDouble(),
                anyDouble(),
                anyDouble());
        verify(orchestrator, never()).resume(any(HostName.class));
        verify(nodeRepository, never()).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());
    }

    @Test
    public void provisionedNodeWithNoContainerIsCleanedAndRecycled() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage = new DockerImage("dockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage),
                containerName,
                NodeState.PROVISIONED,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);

        when(docker.imageIsDownloaded(dockerImage)).thenReturn(true);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, NO_CONTAINER);

        verify(orchestrator, never()).suspend(any(HostName.class));
        verify(docker, never()).stopContainer(any(ContainerName.class));
        verify(docker, never()).deleteContainer(any(ContainerName.class));
        final InOrder inOrder = inOrder(docker, nodeRepository);
        inOrder.verify(docker).deleteApplicationStorage(containerName);
        inOrder.verify(nodeRepository).markAsReady(hostName);
        verify(docker, never()).startContainer(
                any(DockerImage.class),
                any(HostName.class),
                any(ContainerName.class),
                anyDouble(),
                anyDouble(),
                anyDouble());
        verify(orchestrator, never()).resume(any(HostName.class));
        verify(nodeRepository, never()).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());
    }

    @Test
    public void noRedundantNodeRepositoryCalls() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage1 = new DockerImage("dockerImage1");
        final DockerImage dockerImage2 = new DockerImage("dockerImage2");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec1 = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage1),
                containerName,
                NodeState.ACTIVE,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final ContainerNodeSpec nodeSpec2 = new ContainerNodeSpec(
                nodeSpec1.hostname,
                Optional.of(dockerImage2),
                nodeSpec1.containerName,
                nodeSpec1.nodeState,
                nodeSpec1.wantedRestartGeneration,
                nodeSpec1.currentRestartGeneration,
                nodeSpec1.minCpuCores,
                nodeSpec1.minMainMemoryAvailableGb,
                nodeSpec1.minDiskAvailableGb);
        final boolean isRunning = true;
        final Container existingContainer1 = new Container(hostName, dockerImage1, containerName, isRunning);
        final Container existingContainer2 = new Container(hostName, dockerImage2, containerName, isRunning);
        final String vespaVersion = "7.8.9";

        when(docker.imageIsDownloaded(any(DockerImage.class))).thenReturn(true);
        when(docker.executeInContainer(eq(containerName), anyVararg())).thenReturn(NODE_PROGRAM_DOESNT_EXIST);
        when(docker.getVespaVersion(containerName)).thenReturn(vespaVersion);
        when(orchestrator.suspend(any(HostName.class))).thenReturn(true);

        final InOrder inOrder = inOrder(nodeRepository, docker);

        nodeAgent.synchronizeLocalContainerState(nodeSpec1, Optional.of(existingContainer1));
        inOrder.verify(docker, times(1)).executeInContainer(any(), anyVararg());
        // Should get exactly one invocation.
        inOrder.verify(nodeRepository).updateNodeAttributes(hostName, restartGeneration, dockerImage1, vespaVersion);
        verify(nodeRepository, times(1)).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());

        nodeAgent.synchronizeLocalContainerState(nodeSpec1, Optional.of(existingContainer1));
        inOrder.verify(docker, never()).executeInContainer(any(), anyVararg());
        // No attributes have changed; no second invocation should take place.
        verify(nodeRepository, times(1)).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());

        nodeAgent.synchronizeLocalContainerState(nodeSpec2, Optional.of(existingContainer1));
        inOrder.verify(docker, times(2)).executeInContainer(any(), anyVararg());
        // One attribute has changed, should cause new invocation.
        inOrder.verify(nodeRepository).updateNodeAttributes(hostName, restartGeneration, dockerImage2, vespaVersion);
        verify(nodeRepository, times(2)).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());

        nodeAgent.synchronizeLocalContainerState(nodeSpec2, Optional.of(existingContainer2));
        inOrder.verify(docker, never()).executeInContainer(any(), anyVararg());
        // No attributes have changed; no new invocation should take place.
        verify(nodeRepository, times(2)).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());

        nodeAgent.synchronizeLocalContainerState(nodeSpec1, Optional.of(existingContainer2));
        inOrder.verify(docker, times(2)).executeInContainer(any(), anyVararg());
        // Back to previous node spec should also count as new data and cause a new invocation.
        inOrder.verify(nodeRepository).updateNodeAttributes(hostName, restartGeneration, dockerImage1, vespaVersion);
        verify(nodeRepository, times(3)).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());
    }

    @Test
    public void failedNodeRepositoryUpdateIsRetried() throws Exception {
        final long restartGeneration = 1;
        final DockerImage dockerImage1 = new DockerImage("dockerImage1");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec1 = new ContainerNodeSpec(
                hostName,
                Optional.of(dockerImage1),
                containerName,
                NodeState.ACTIVE,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final boolean isRunning = true;
        final Container existingContainer = new Container(hostName, dockerImage1, containerName, isRunning);
        final String vespaVersion = "7.8.9";

        when(docker.imageIsDownloaded(any(DockerImage.class))).thenReturn(true);
        when(docker.executeInContainer(eq(containerName), anyVararg())).thenReturn(NODE_PROGRAM_DOESNT_EXIST);
        when(docker.getVespaVersion(containerName)).thenReturn(vespaVersion);
        when(orchestrator.suspend(any(HostName.class))).thenReturn(true);
        doThrow(new IOException()).doNothing().when(nodeRepository).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());

        final InOrder inOrder = inOrder(nodeRepository);

        try {
            nodeAgent.synchronizeLocalContainerState(nodeSpec1, Optional.of(existingContainer));
            fail("Should throw exception");
        } catch (IOException e) {
            // As expected.
        }
        // Should get exactly one invocation.
        inOrder.verify(nodeRepository).updateNodeAttributes(hostName, restartGeneration, dockerImage1, vespaVersion);
        verify(nodeRepository, times(1)).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());

        nodeAgent.synchronizeLocalContainerState(nodeSpec1, Optional.of(existingContainer));
        // First attribute update failed, so it should be retried now.
        inOrder.verify(nodeRepository).updateNodeAttributes(hostName, restartGeneration, dockerImage1, vespaVersion);
        verify(nodeRepository, times(2)).updateNodeAttributes(
                any(HostName.class), anyLong(), any(DockerImage.class), anyString());
    }

    @Test
    public void resumeProgramRunsUntilSuccess() throws Exception {
        final long restartGeneration = 1;
        final HostName hostName = new HostName("hostname");
        final DockerImage wantedDockerImage = new DockerImage("wantedDockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(wantedDockerImage),
                containerName,
                NodeState.ACTIVE,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final String vespaVersion = "7.8.9";
        final boolean isRunning = true;
        final Optional<Container> uptodateContainer = Optional.of(
                new Container(hostName, wantedDockerImage, containerName, isRunning));

        when(docker.imageIsDownloaded(wantedDockerImage)).thenReturn(true);
        when(docker.executeInContainer(eq(containerName), anyVararg()))
                .thenReturn(new ProcessResult(0, "node program exists"))
                .thenReturn(new ProcessResult(1, "node program fails 1st time"))
                .thenReturn(new ProcessResult(0, "node program exists"))
                .thenReturn(new ProcessResult(1, "node program fails 2nd time"))
                .thenReturn(new ProcessResult(0, "node program exists"))
                .thenReturn(new ProcessResult(0, "node program succeeds 3rd time"));

        when(docker.getVespaVersion(containerName)).thenReturn(vespaVersion);

        final InOrder inOrder = inOrder(orchestrator, docker);

        // 1st try
        try {
            nodeAgent.synchronizeLocalContainerState(nodeSpec, NO_CONTAINER);
            fail("Should have been a failure");
        } catch (Exception e) {
            // expected
        }

        inOrder.verify(docker).startContainer(
                nodeSpec.wantedDockerImage.get(),
                nodeSpec.hostname,
                nodeSpec.containerName,
                nodeSpec.minCpuCores.get(),
                nodeSpec.minDiskAvailableGb.get(),
                nodeSpec.minMainMemoryAvailableGb.get());
        inOrder.verify(docker, times(2)).executeInContainer(any(), anyVararg());
        inOrder.verifyNoMoreInteractions();

        // 2nd try
        try {
            nodeAgent.synchronizeLocalContainerState(nodeSpec, NO_CONTAINER);
            fail("Should have been a failure");
        } catch (Exception e) {
            // expected
        }

        inOrder.verify(docker, times(2)).executeInContainer(any(), anyVararg());
        inOrder.verifyNoMoreInteractions();

        // 3rd try success
        nodeAgent.synchronizeLocalContainerState(nodeSpec, NO_CONTAINER);

        inOrder.verify(docker, times(2)).executeInContainer(any(), anyVararg());
        inOrder.verify(orchestrator).resume(hostName);
        inOrder.verifyNoMoreInteractions();

        // 4th and 5th times, already started, no calls to executeInContainer
        nodeAgent.synchronizeLocalContainerState(nodeSpec, uptodateContainer);

        inOrder.verify(docker, never()).executeInContainer(any(), anyVararg());
        inOrder.verify(orchestrator).resume(hostName);
        inOrder.verifyNoMoreInteractions();

        nodeAgent.synchronizeLocalContainerState(nodeSpec, uptodateContainer);

        inOrder.verify(docker, never()).executeInContainer(any(), anyVararg());
        inOrder.verify(orchestrator).resume(hostName);
        inOrder.verifyNoMoreInteractions();
    }

    // The suspend program can fail by returning non-zero exit code, or throw IOException.
    private enum NodeProgramFailureScenario {
        EXCEPTION, NODE_PROGRAM_FAILURE}

    private void failSuspendProgram(NodeProgramFailureScenario scenario) throws Exception {
        final long restartGeneration = 1;
        final HostName hostName = new HostName("hostname");
        final DockerImage currentDockerImage = new DockerImage("currentDockerImage");
        final DockerImage wantedDockerImage = new DockerImage("wantedDockerImage");
        final ContainerName containerName = new ContainerName("container-name");
        final ContainerNodeSpec nodeSpec = new ContainerNodeSpec(
                hostName,
                Optional.of(wantedDockerImage),
                containerName,
                NodeState.ACTIVE,
                Optional.of(restartGeneration),
                Optional.of(restartGeneration),
                MIN_CPU_CORES,
                MIN_MAIN_MEMORY_AVAILABLE_GB,
                MIN_DISK_AVAILABLE_GB);
        final boolean isRunning = true;
        final Container existingContainer = new Container(hostName, currentDockerImage, containerName, isRunning);
        final String vespaVersion = "7.8.9";

        when(docker.imageIsDownloaded(wantedDockerImage)).thenReturn(true);
        switch (scenario) {
            case EXCEPTION:
                when(docker.executeInContainer(eq(containerName), anyVararg()))
                        .thenThrow(new RuntimeException()) // suspending node
                        .thenReturn(new ProcessResult(1, "")); // resuming node, node program doesn't exist
                break;
            case NODE_PROGRAM_FAILURE:
                when(docker.executeInContainer(eq(containerName), anyVararg()))
                        .thenReturn(new ProcessResult(0, "")) // program exists
                        .thenReturn(new ProcessResult(1, "error")) // and program fails to suspend
                        .thenReturn(new ProcessResult(0, "")) // program exists
                        .thenReturn(new ProcessResult(0, "output")); // resuming succeeds
                break;
        }
        when(docker.getVespaVersion(containerName)).thenReturn(vespaVersion);
        when(orchestrator.suspend(any(HostName.class))).thenReturn(true);

        nodeAgent.synchronizeLocalContainerState(nodeSpec, Optional.of(existingContainer));

        final InOrder inOrder = inOrder(orchestrator, docker, nodeRepository);
        inOrder.verify(orchestrator).suspend(hostName);

        switch (scenario) {
            case EXCEPTION:
                inOrder.verify(docker, times(1)).executeInContainer(any(), anyVararg());
                break;
            case NODE_PROGRAM_FAILURE:
                inOrder.verify(docker, times(2)).executeInContainer(any(), anyVararg());
                break;
        }

        inOrder.verify(docker).stopContainer(containerName);
        inOrder.verify(docker).deleteContainer(containerName);
        inOrder.verify(docker).startContainer(
                nodeSpec.wantedDockerImage.get(),
                nodeSpec.hostname,
                nodeSpec.containerName,
                nodeSpec.minCpuCores.get(),
                nodeSpec.minDiskAvailableGb.get(),
                nodeSpec.minMainMemoryAvailableGb.get());

        switch (scenario) {
            case EXCEPTION:
                inOrder.verify(docker, times(1)).executeInContainer(any(), anyVararg());
                break;
            case NODE_PROGRAM_FAILURE:
                inOrder.verify(docker, times(2)).executeInContainer(any(), anyVararg());
                break;
        }

        inOrder.verify(nodeRepository).updateNodeAttributes(hostName, restartGeneration, wantedDockerImage, vespaVersion);
        inOrder.verify(orchestrator).resume(hostName);
        inOrder.verifyNoMoreInteractions();
    }

    @Test
    public void suspendExceptionIsIgnored() throws Exception {
        failSuspendProgram(NodeProgramFailureScenario.EXCEPTION);
    }

    @Test
    public void suspendFailureIsIgnored() throws Exception {
        failSuspendProgram(NodeProgramFailureScenario.NODE_PROGRAM_FAILURE);
    }

    @Test
    public void absenceOfNodeProgramIsSuccess() throws Exception {
        final ContainerName containerName = new ContainerName("container-name");
        final String programPath = "/bin/command";

        when(docker.executeInContainer(any(), anyVararg())).thenReturn(new ProcessResult(3, "output"));

        Optional<ProcessResult> result = NodeAgentImpl.executeOptionalProgram(
                docker,
                containerName,
                programPath,
                "arg1",
                "arg2");

        String[] nodeProgramExistsCommand = NodeAgentImpl.programExistsCommand(programPath);
        assertThat(nodeProgramExistsCommand.length, is(4));

        verify(docker, times(1)).executeInContainer(
                eq(containerName),
                // Mockito fails if we put the array here instead...
                eq(nodeProgramExistsCommand[0]),
                eq(nodeProgramExistsCommand[1]),
                eq(nodeProgramExistsCommand[2]),
                eq(nodeProgramExistsCommand[3]));
        assertThat(result.isPresent(), is(false));
    }

    @Test
    public void processResultFromNodeProgramWhenPresent() throws Exception {
        final ContainerName containerName = new ContainerName("container-name");
        final ProcessResult actualResult = new ProcessResult(3, "output");
        final String programPath = "/bin/command";
        final String[] command = new String[] {programPath, "arg"};

        when(docker.executeInContainer(any(), anyVararg()))
                .thenReturn(new ProcessResult(0, "")) // node program exists
                .thenReturn(actualResult); // output from node program

        Optional<ProcessResult> result = NodeAgentImpl.executeOptionalProgram(
                docker,
                containerName,
                command);

        String[] nodeProgramExistsCommand = NodeAgentImpl.programExistsCommand(programPath);
        assertThat(nodeProgramExistsCommand.length, is(4));

        final InOrder inOrder = inOrder(docker);
        inOrder.verify(docker, times(1)).executeInContainer(
                eq(containerName),
                // Mockito fails if we put the array here instead...
                eq(nodeProgramExistsCommand[0]),
                eq(nodeProgramExistsCommand[1]),
                eq(nodeProgramExistsCommand[2]),
                eq(nodeProgramExistsCommand[3]));
        inOrder.verify(docker, times(1)).executeInContainer(
                eq(containerName),
                eq(command[0]),
                eq(command[1]));

        assertThat(result.isPresent(), is(true));
        assertThat(result.get(), is(actualResult));
    }
}
