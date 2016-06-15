// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.provision.node;

import com.yahoo.vespa.config.nodes.NodeRepositoryConfig;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;

import java.util.ArrayList;
import java.util.List;

import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.*;


public class NodeFlavorsTest {
    @Rule
    public final ExpectedException exception = ExpectedException.none();

    @Test
    public void testReplacesWithBadValue() {
        NodeRepositoryConfig.Builder builder = new NodeRepositoryConfig.Builder();
        List<NodeRepositoryConfig.Flavor.Builder> flavorBuilderList = new ArrayList<>();
        NodeRepositoryConfig.Flavor.Builder flavorBuilder = new NodeRepositoryConfig.Flavor.Builder();
        NodeRepositoryConfig.Flavor.Replaces.Builder flavorReplacesBuilder = new NodeRepositoryConfig.Flavor.Replaces.Builder();
        flavorReplacesBuilder.name("non-existing-config");
        flavorBuilder.name("strawberry").cost(2).replaces.add(flavorReplacesBuilder);
        flavorBuilderList.add(flavorBuilder);
        builder.flavor(flavorBuilderList);
        NodeRepositoryConfig config = new NodeRepositoryConfig(builder);
        exception.expect(IllegalStateException.class);
        exception.expectMessage("Replaces for strawberry pointing to a non existing flavor: non-existing-config");
        new NodeFlavors(config);
    }

    @Test
    public void testConfigParsing() {
        NodeRepositoryConfig.Builder builder = new NodeRepositoryConfig.Builder();
        List<NodeRepositoryConfig.Flavor.Builder> flavorBuilderList = new ArrayList<>();
        {
            NodeRepositoryConfig.Flavor.Builder flavorBuilder = new NodeRepositoryConfig.Flavor.Builder();
            NodeRepositoryConfig.Flavor.Replaces.Builder flavorReplacesBuilder = new NodeRepositoryConfig.Flavor.Replaces.Builder();
            flavorReplacesBuilder.name("banana");
            flavorBuilder.name("strawberry").cost(2).replaces.add(flavorReplacesBuilder);
            flavorBuilderList.add(flavorBuilder);
        }
        {
            NodeRepositoryConfig.Flavor.Builder flavorBuilder = new NodeRepositoryConfig.Flavor.Builder();
            flavorBuilder.name("banana").cost(3);
            flavorBuilderList.add(flavorBuilder);
        }
        builder.flavor(flavorBuilderList);
        NodeRepositoryConfig config = new NodeRepositoryConfig(builder);
        NodeFlavors nodeFlavors = new NodeFlavors(config);
        assertThat(nodeFlavors.getFlavor("banana").get().cost(), is(3));
    }
}