// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.provision.testutils;

import com.yahoo.vespa.config.nodes.NodeRepositoryConfig;
import com.yahoo.vespa.hosted.provision.node.NodeFlavors;

/**
 * A mock repository prepopulated with flavors, to avoid having config.
 * Instantiated by DI from application package above.
 */
public class MockNodeFlavors extends NodeFlavors {

    public MockNodeFlavors() {
        super(createConfig());
    }

    private static NodeRepositoryConfig createConfig() {
        FlavorConfigBuilder b = new FlavorConfigBuilder();
        b.addFlavor("default", 2., 16., 400, "env");
        b.addFlavor("medium-disk", 6., 12., 56, "foo");
        b.addFlavor("large", 4., 32., 1600, "env");
        b.addFlavor("docker", 0.2, 0.5, 100, "docker");
        NodeRepositoryConfig.Flavor.Builder largeVariant = b.addFlavor("large-variant", 64, 128, 2000, "env");
        b.addReplaces("large", largeVariant);
        NodeRepositoryConfig.Flavor.Builder expensiveFlavor = b.addFlavor("expensive", 0, 0, 0, "");
        b.addReplaces("default", expensiveFlavor);
        b.addCost(200, expensiveFlavor);

        return b.build();
    }

}
