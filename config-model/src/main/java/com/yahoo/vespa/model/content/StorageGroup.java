// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.content;

import com.yahoo.config.provision.ClusterMembership;
import com.yahoo.config.provision.ClusterSpec;
import com.yahoo.config.application.api.DeployLogger;
import com.yahoo.log.LogLevel;
import com.yahoo.vespa.config.content.StorDistributionConfig;
import com.yahoo.vespa.model.HostResource;
import com.yahoo.vespa.model.builder.xml.dom.ModelElement;
import com.yahoo.vespa.model.builder.xml.dom.NodesSpecification;
import com.yahoo.vespa.model.builder.xml.dom.VespaDomBuilder;
import com.yahoo.vespa.model.content.cluster.ContentCluster;
import com.yahoo.vespa.model.content.engines.PersistenceEngine;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

/**
 * A group of storage nodes/distributors.
 *
 * @author unknown, probably thomasg
 * @author bratseth has done things here recently
 */
public class StorageGroup {

    private final boolean useCpuSocketAffinity;
    private final String index;
    private Optional<String> partitions;
    String name;
    private final ContentCluster owner;
    private final Optional<Long> mmapNoCoreLimit;

    private final List<StorageGroup> subgroups = new ArrayList<>();
    private final List<StorageNode> nodes = new ArrayList<>();

    /**
     * Creates a storage group
     *
     * @param owner the cluster this group belongs to
     * @param name the name of this group
     * @param index the distribution-key index og this group
     * @param partitions the distribution strategy to use to distribute content to subgroups or empty
     *        (meaning that the "*" distribution will be used) only if this is a leaf group
     *        (having nodes, not subgroups as children).
     * @param useCpuSocketAffinity whether processes should be started with socket affinity
     */
    private StorageGroup(ContentCluster owner, String name, String index, Optional<String> partitions,
                         boolean useCpuSocketAffinity, Optional<Long> mmapNoCoreLimit)
    {
        this.owner = owner;
        this.index = index;
        this.name = name;
        this.partitions = partitions;
        this.useCpuSocketAffinity = useCpuSocketAffinity;
        this.mmapNoCoreLimit = mmapNoCoreLimit;
    }

    /** Returns the name of this group, or null if it is the root group */
    public String getName() { return name; }

    /** Returns the subgroups of this, or an empty list if it is a leaf group */
    public List<StorageGroup> getSubgroups() { return subgroups; }

    /** Returns the nodes of this, or an empty list of it is not a leaf group */
    public List<StorageNode> getNodes() { return nodes; }

    public ContentCluster getOwner() { return owner; }

    /** Returns the index of this group, or null if it is the root group */
    public String getIndex() { return index; }

    public Optional<String> getPartitions() { return partitions; }
    public boolean useCpuSocketAffinity() { return useCpuSocketAffinity; }
    public Optional<Long> getMmapNoCoreLimit() { return mmapNoCoreLimit; }

    /** Returns all the nodes below this group */
    public List<StorageNode> recursiveGetNodes() {
        if ( ! nodes.isEmpty()) return nodes;
        List<StorageNode> nodes = new ArrayList<>();
        for (StorageGroup subgroup : subgroups)
            nodes.addAll(subgroup.recursiveGetNodes());
        return nodes;
    }

    public Collection<StorDistributionConfig.Group.Builder> getGroupStructureConfig() {
        List<StorDistributionConfig.Group.Builder> groups = new ArrayList<>();

        StorDistributionConfig.Group.Builder myGroup = new StorDistributionConfig.Group.Builder();
        getConfig(myGroup);
        groups.add(myGroup);

        for (StorageGroup g : subgroups) {
            groups.addAll(g.getGroupStructureConfig());
        }

        return groups;
    }

    public void getConfig(StorDistributionConfig.Group.Builder builder) {
        builder.index(index == null ? "invalid" : index);
        builder.name(name == null ? "invalid" : name);
        if (partitions.isPresent())
            builder.partitions(partitions.get());
        for (StorageNode node : nodes) {
            StorDistributionConfig.Group.Nodes.Builder nb = new StorDistributionConfig.Group.Nodes.Builder();
            nb.index(node.getDistributionKey());
            nb.retired(node.isRetired());
            builder.nodes.add(nb);
        }
        builder.capacity(getCapacity());
    }

    public int getNumberOfLeafGroups() {
        int count = subgroups.isEmpty() ? 1 : 0;
        for (StorageGroup g : subgroups) {
            count += g.getNumberOfLeafGroups();
        }
        return count;
    }

    public double getCapacity() {
        double capacity = 0;
        for (StorageNode node : nodes) {
            capacity += node.getCapacity();
        }
        for (StorageGroup group : subgroups) {
            capacity += group.getCapacity();
        }
        return capacity;
    }

    /** Returns the total number of nodes below this group */
    public int countNodes() {
        int nodeCount = nodes.size();
        for (StorageGroup group : subgroups)
            nodeCount += group.countNodes();
        return nodeCount;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof StorageGroup) {
            StorageGroup rhs = (StorageGroup)obj;
            return this.index.equals(rhs.index) &&
                    this.name.equals(rhs.name) &&
                    this.partitions.equals(rhs.partitions);
        }
        return false;
    }

    public static class Builder {

        private final ModelElement clusterElement;
        private final ContentCluster owner;
        private final DeployLogger deployLogger;

        public Builder(ModelElement clusterElement, ContentCluster owner, DeployLogger deployLogger) {
            this.clusterElement = clusterElement;
            this.owner = owner;
            this.deployLogger = deployLogger;
        }

        public StorageGroup buildRootGroup() {
            Optional<ModelElement> group = Optional.ofNullable(clusterElement.getChild("group"));
            Optional<ModelElement> nodes = getNodes(clusterElement);

            if (group.isPresent() && nodes.isPresent())
                throw new IllegalStateException("Both group and nodes exists, only one of these tags is legal");
            if (group.isPresent() && (group.get().getStringAttribute("name") != null || group.get().getIntegerAttribute("distribution-key") != null))
                    owner.deployLogger().log(LogLevel.INFO, "'distribution-key' attribute on a content cluster's root group is ignored");

            GroupBuilder groupBuilder = collectGroup(group, nodes, null, null);
            if (owner.isHostedVespa()) {
                return groupBuilder.buildHosted(owner, Optional.empty());
            } else {
                return groupBuilder.buildNonHosted(owner, Optional.empty());
            }
        }

        /**
         * Represents a storage group and can build storage nodes in both hosted and non-hosted environments.
         */
        private static class GroupBuilder {

            private final StorageGroup storageGroup;

            /* The explicitly defined subgroups of this */
            private final List<GroupBuilder> subGroups;
            private final List<XmlNodeBuilder> nodeBuilders;

            /** The nodes explicitly specified as a nodes tag in this group, or empty if none */
            private final Optional<NodesSpecification> nodeRequirement;

            private final DeployLogger deployLogger;

            private GroupBuilder(StorageGroup storageGroup, List<GroupBuilder> subGroups, List<XmlNodeBuilder> nodeBuilders,
                                 Optional<NodesSpecification> nodeRequirement, DeployLogger deployLogger) {
                this.storageGroup = storageGroup;
                this.subGroups = subGroups;
                this.nodeBuilders = nodeBuilders;
                this.nodeRequirement = nodeRequirement;
                this.deployLogger = deployLogger;
            }

            /**
             * Builds a storage group for a nonhosted environment
             *
             * @param owner the cluster owning this
             * @param parent the parent storage group, or empty if this is the root group
             * @return the storage group build by this
             */
            public StorageGroup buildNonHosted(ContentCluster owner, Optional<GroupBuilder> parent) {
                for (GroupBuilder subGroup : subGroups) {
                    storageGroup.subgroups.add(subGroup.buildNonHosted(owner, Optional.of(this)));
                }
                for (XmlNodeBuilder nodeBuilder : nodeBuilders) {
                    storageGroup.nodes.add(nodeBuilder.build(owner, storageGroup));
                }

                if ( ! parent.isPresent())
                    owner.redundancy().setTotalNodes(storageGroup.countNodes());

                return storageGroup;
            }

            /**
             * Builds a storage group for a hosted environment
             *
             * @param owner the cluster owning this
             * @param parent the parent storage group, or empty if this is the root group
             * @return the storage group build by this
             */
            public StorageGroup buildHosted(ContentCluster owner, Optional<GroupBuilder> parent) {
                Map<HostResource, ClusterMembership> hostMapping = nodeRequirement.isPresent() ? allocateHosts(owner) : Collections.emptyMap();

                Map<Optional<ClusterSpec.Group>, Map<HostResource, ClusterMembership>> hostGroups = collectAllocatedSubgroups(hostMapping);
                if (hostGroups.size() > 1) {
                    if (parent.isPresent())
                        throw new IllegalArgumentException("Cannot specify groups using the groups attribute in nested content groups");
                    owner.redundancy().setTotalNodes(hostMapping.size());

                    // Switch redundancy settings to meaning "per group"
                    owner.redundancy().setImplicitGroups(hostGroups.size());

                    // Compute partitions expression
                    int redundancyPerGroup = (int)Math.floor(owner.redundancy().effectiveFinalRedundancy() / hostGroups.size());
                    storageGroup.partitions = Optional.of(computePartitions(redundancyPerGroup, hostGroups.size()));

                    // create subgroups as returned from allocation
                    for (Map.Entry<Optional<ClusterSpec.Group>, Map<HostResource, ClusterMembership>> hostGroup : hostGroups.entrySet()) {
                        String groupIndex = hostGroup.getKey().get().value();
                        StorageGroup subgroup = new StorageGroup(owner, groupIndex, groupIndex, Optional.empty(), false, Optional.empty());
                        for (Map.Entry<HostResource, ClusterMembership> host : hostGroup.getValue().entrySet()) {
                            subgroup.nodes.add(createStorageNode(owner, host.getKey(), subgroup, host.getValue()));
                        }
                        storageGroup.subgroups.add(subgroup);
                    }
                }
                else { // or otherwise just create the nodes directly on this group, or the explicitly enumerated subgroups
                    for (Map.Entry<HostResource, ClusterMembership> host : hostMapping.entrySet()) {
                        storageGroup.nodes.add(createStorageNode(owner, host.getKey(), storageGroup, host.getValue()));
                    }
                    for (GroupBuilder subGroup : subGroups) {
                        storageGroup.subgroups.add(subGroup.buildHosted(owner, Optional.of(this)));
                    }
                    if ( ! parent.isPresent())
                        owner.redundancy().setTotalNodes(storageGroup.countNodes());
                }
                return storageGroup;
            }

            /** This returns a partition string which specifies equal distribution between all groups */
            // TODO: Make a partitions object
            private String computePartitions(int redundancyPerGroup, int numGroups) {
                StringBuilder sb = new StringBuilder();
                for (int i = 0; i < numGroups - 1; ++i) {
                    sb.append(redundancyPerGroup);
                    sb.append("|");
                }
                sb.append("*");
                return sb.toString();
            }

            private Map<HostResource, ClusterMembership> allocateHosts(ContentCluster parent) {
                ClusterSpec.Id clusterId = ClusterSpec.Id.from(parent.getStorageNodes().getClusterName());
                Optional<ClusterSpec.Group> groupId = storageGroup.getIndex() == null ? Optional.empty() : Optional.of(ClusterSpec.Group.from(storageGroup.getIndex()));
                return nodeRequirement.get().provision(parent.getRoot().getHostSystem(), ClusterSpec.Type.content, clusterId, groupId, deployLogger);
            }

            /** Collect hosts per group */
            private Map<Optional<ClusterSpec.Group>, Map<HostResource, ClusterMembership>> collectAllocatedSubgroups(Map<HostResource, ClusterMembership> hostMapping) {
                Map<Optional<ClusterSpec.Group>, Map<HostResource, ClusterMembership>> hostsPerGroup = new LinkedHashMap<>();
                for (Map.Entry<HostResource, ClusterMembership> entry : hostMapping.entrySet()) {
                    Optional<ClusterSpec.Group> group = entry.getValue().cluster().group();
                    Map<HostResource, ClusterMembership> hostsInGroup = hostsPerGroup.get(group);
                    if (hostsInGroup == null) {
                        hostsInGroup = new LinkedHashMap<>();
                        hostsPerGroup.put(group, hostsInGroup);
                    }
                    hostsInGroup.put(entry.getKey(), entry.getValue());
                }
                return hostsPerGroup;
            }

        }

        private static class XmlNodeBuilder {
            private final ModelElement clusterElement;
            private final ModelElement element;

            private XmlNodeBuilder(ModelElement clusterElement, ModelElement element) {
                this.clusterElement = clusterElement;
                this.element = element;
            }

            public StorageNode build(ContentCluster parent, StorageGroup storageGroup) {
                StorageNode sNode = new StorageNode.Builder().build(parent.getStorageNodes(), element.getXml());
                PersistenceEngine provider = parent.getPersistence().create(sNode, storageGroup, element);
                new Distributor.Builder(clusterElement, provider).build(parent.getDistributorNodes(), element.getXml());
                return sNode;
            }
        }

        /**
         * Creates a content group builder from a group and/or nodes element.
         * These are the possibilities:
         * <ul>
         * <li>group and nodes is present: This is a leaf group specifying a set of nodes</li>
         * <li>only group is present: This is a nonleaf group</li>
         * <li>only nodes is present: This is the implicitly specified toplevel leaf group, or a set of groups
         *                            specified using a group count attrbute.
         * </ul>
         */
        private GroupBuilder collectGroup(Optional<ModelElement> groupElement, Optional<ModelElement> nodesElement, String name, String index) {
            StorageGroup group = new StorageGroup(owner, name, index,
                                                  childAsString(groupElement, "distribution.partitions"),
                                                  booleanAttributeOr(groupElement, VespaDomBuilder.CPU_SOCKET_AFFINITY_ATTRIB_NAME, false),
                                                  childAsLong(groupElement, VespaDomBuilder.MMAP_NOCORE_LIMIT));

            List<GroupBuilder> subGroups = groupElement.isPresent() ? collectSubGroups(group, groupElement.get()) : Collections.emptyList();

            List<XmlNodeBuilder> explicitNodes = new ArrayList<>();
            explicitNodes.addAll(collectExplicitNodes(groupElement));
            explicitNodes.addAll(collectExplicitNodes(nodesElement));

            if (subGroups.size() > 0 && explicitNodes.size() > 0)
                throw new IllegalArgumentException("A group can contain either nodes or groups, but not both.");

            Optional<NodesSpecification> nodeRequirement =
                nodesElement.isPresent() && nodesElement.get().getStringAttribute("count") != null ? Optional.of(NodesSpecification.from(nodesElement.get())) : Optional.empty();
            if (nodeRequirement.isPresent() && subGroups.size() > 0)
                throw new IllegalArgumentException("A group can contain either explicit subgroups or a nodes specification, but not both.");
            return new GroupBuilder(group, subGroups, explicitNodes, nodeRequirement, deployLogger);
        }

        private Optional<String> childAsString(Optional<ModelElement> element, String childTagName) {
            if ( ! element.isPresent()) return Optional.empty();
            return Optional.ofNullable(element.get().childAsString(childTagName));
        }
        private Optional<Long> childAsLong(Optional<ModelElement> element, String childTagName) {
            if ( ! element.isPresent()) return Optional.empty();
            return Optional.ofNullable(element.get().childAsLong(childTagName));
        }

        private boolean booleanAttributeOr(Optional<ModelElement> element, String attributeName, boolean defaultValue) {
            if ( ! element.isPresent()) return defaultValue;
            return element.get().getBooleanAttribute(attributeName, defaultValue);
        }

        private Optional<ModelElement> getNodes(ModelElement groupOrNodesElement) {
            if (groupOrNodesElement.getXml().getTagName().equals("nodes")) return Optional.of(groupOrNodesElement);
            return Optional.ofNullable(groupOrNodesElement.getChild("nodes"));
        }

        private List<XmlNodeBuilder> collectExplicitNodes(Optional<ModelElement> groupOrNodesElement) {
            if ( ! groupOrNodesElement.isPresent()) return Collections.emptyList();
            List<XmlNodeBuilder> nodes = new ArrayList<>();
            for (ModelElement n : groupOrNodesElement.get().subElements("node"))
                nodes.add(new XmlNodeBuilder(clusterElement, n));
            return nodes;
        }

        private List<GroupBuilder> collectSubGroups(StorageGroup parentGroup, ModelElement parentGroupElement) {
            List<ModelElement> subGroupElements = parentGroupElement.subElements("group");
            if (subGroupElements.size() > 1 &&  ! parentGroup.getPartitions().isPresent())
                throw new IllegalArgumentException("'distribution' attribute is required with multiple subgroups");

            List<GroupBuilder> subGroups = new ArrayList<>();
            String indexPrefix = "";
            if (parentGroup.index != null) {
                indexPrefix = parentGroup.index + ".";
            }
            for (ModelElement g : subGroupElements) {
                subGroups.add(collectGroup(Optional.of(g), Optional.ofNullable(g.getChild("nodes")), g.getStringAttribute("name"),
                                           indexPrefix + g.getIntegerAttribute("distribution-key")));
            }
            return subGroups;
        }

        private static StorageNode createStorageNode(ContentCluster parent, HostResource hostResource, StorageGroup parentGroup, ClusterMembership clusterMembership) {
            StorageNode sNode = new StorageNode(parent.getStorageNodes(), null, clusterMembership.index(), clusterMembership.retired());
            sNode.setHostResource(hostResource);
            sNode.initService();

            // TODO: Supplying null as XML is not very nice
            PersistenceEngine provider = parent.getPersistence().create(sNode, parentGroup, null);
            Distributor d = new Distributor(parent.getDistributorNodes(), clusterMembership.index(), null, provider);
            d.setHostResource(sNode.getHostResource());
            d.initService();
            return sNode;
        }
    }

}
