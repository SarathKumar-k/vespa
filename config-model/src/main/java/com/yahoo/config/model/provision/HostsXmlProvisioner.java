// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.config.model.provision;

import com.yahoo.config.model.api.HostProvisioner;
import com.yahoo.config.provision.*;
import com.yahoo.vespa.model.container.Container;

import java.io.Reader;
import java.util.List;

/**
 * A host provisioner based on a hosts.xml file.
 * No state in this provisioner, i.e it does not know anything about the active
 * application if one exists. Pre-condition: A valid hosts file.
 *
 * @author musum
 * @since 5.11
 */
public class HostsXmlProvisioner implements HostProvisioner {

    private final Hosts hosts;
    public static final String IMPLICIT_ADMIN_HOSTALIAS = "INTERNAL_VESPA_IMPLICIT_ADMIN";

    public HostsXmlProvisioner(Reader hosts) {
        this.hosts = Hosts.getHosts(hosts);
    }

    @Override
    public HostSpec allocateHost(String alias) {
        /**
         * Some special rules to allow no admin elements as well
         * as jdisc element without nodes.
         */
        if (alias.equals(IMPLICIT_ADMIN_HOSTALIAS)) {
            if (hosts.getHosts().size() > 1) {
                throw new IllegalArgumentException("More than 1 host specified (" + hosts.getHosts().size() + ") and <admin> not specified");
            } else {
                return host2HostSpec(getFirstHost());
            }
        } else if (alias.equals(Container.SINGLENODE_CONTAINER_SERVICESPEC)) {
            return host2HostSpec(getFirstHost());
        }
        for (Host host : hosts.getHosts()) {
            if (host.getHostAliases().contains(alias)) {
                return new HostSpec(host.getHostname(), host.getHostAliases());
            }
        }
        throw new IllegalArgumentException("Unable to find host for alias '" + alias + "'");
    }

    @Override
    public List<HostSpec> prepare(ClusterSpec cluster, Capacity quantity, int groups, ProvisionLogger logger) {
        throw new UnsupportedOperationException();
    }

    private HostSpec host2HostSpec(Host host) {
        return new HostSpec(host.getHostname(), host.getHostAliases());
    }

    private Host getFirstHost() {
        return hosts.getHosts().iterator().next();
    }

}
