// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.provision.node;

import com.yahoo.component.Version;

import javax.annotation.concurrent.Immutable;
import java.util.Optional;

/**
 * Information about current status of a node
 *
 * @author bratseth
 */
@Immutable
public class Status {

    private final Generation reboot;
    private final Optional<Version> vespaVersion;
    private final Optional<Version> hostedVersion;
    private final Optional<String> stateVersion;
    private final Optional<String> dockerImage;
    private final int failCount;
    private final boolean hardwareFailure;

    public Status(Generation generation,
                  Optional<Version> vespaVersion,
                  Optional<Version> hostedVersion,
                  Optional<String> stateVersion,
                  Optional<String> dockerImage,
                  int failCount,
                  boolean hardwareFailure) {
        this.reboot = generation;
        this.vespaVersion = vespaVersion;
        this.hostedVersion = hostedVersion;
        this.stateVersion = stateVersion;
        this.dockerImage = dockerImage;
        this.failCount = failCount;
        this.hardwareFailure = hardwareFailure;
    }

    /** Returns a copy of this with the reboot generation changed */
    public Status setReboot(Generation reboot) { return new Status(reboot, vespaVersion, hostedVersion, stateVersion, dockerImage, failCount, hardwareFailure); }

    /** Returns the reboot generation of this node */
    public Generation reboot() { return reboot; }

    /** Returns a copy of this with the vespa version changed */
    public Status setVespaVersion(Version version) { return new Status(reboot, Optional.of(version), hostedVersion, stateVersion, dockerImage, failCount, hardwareFailure); }

    /** Returns the Vespa version installed on the node, if known */
    public Optional<Version> vespaVersion() { return vespaVersion; }

    /** Returns a copy of this with the hosted version changed */
    public Status setHostedVersion(Version version) { return new Status(reboot, vespaVersion, Optional.of(version), stateVersion, dockerImage, failCount, hardwareFailure); }

    /** Returns the hosted version installed on the node, if known */
    public Optional<Version> hostedVersion() { return hostedVersion; }

    /** Returns a copy of this with the state version changed */
    public Status setStateVersion(String version) { return new Status(reboot, vespaVersion, hostedVersion, Optional.of(version), dockerImage, failCount, hardwareFailure); }

    /**
     * Returns the state version the node last successfully converged with.
     * The state version contains the version-specific parts in identifying state
     * files on dist, and is of the form VESPAVERSION-HOSTEDVERSION in CI, or otherwise HOSTEDVERSION.
     * It's also used to uniquely identify a hosted Vespa release.
     */
    public Optional<String> stateVersion() { return stateVersion; }

    /** Returns a copy of this with the docker image changed */
    public Status setDockerImage(String dockerImage) { return new Status(reboot, vespaVersion, hostedVersion, stateVersion, Optional.of(dockerImage), failCount, hardwareFailure); }

    /** Returns the current docker image the node is running, if known. */
    public Optional<String> dockerImage() { return dockerImage; }

    public Status increaseFailCount() { return new Status(reboot, vespaVersion, hostedVersion, stateVersion, dockerImage, failCount+1, hardwareFailure); }

    public Status decreaseFailCount() { return new Status(reboot, vespaVersion, hostedVersion, stateVersion, dockerImage, failCount-1, hardwareFailure); }

    public Status setFailCount(Integer value) { return new Status(reboot, vespaVersion, hostedVersion, stateVersion, dockerImage, value, hardwareFailure); }

    /** Returns how many times this node has been moved to the failed state. */
    public int failCount() { return failCount; }

    /** Returns whether a hardware failure has been detected on this node */
    public boolean hardwareFailure() { return hardwareFailure; }

    public Status setHardwareFailure(boolean hardwareFailure) { return new Status(reboot, vespaVersion, hostedVersion, stateVersion, dockerImage, failCount, hardwareFailure); }

    /** Returns the initial status of a newly provisioned node */
    public static Status initial() { return new Status(Generation.inital(), Optional.empty(), Optional.empty(), Optional.empty(), Optional.empty(), 0, false); }

}
