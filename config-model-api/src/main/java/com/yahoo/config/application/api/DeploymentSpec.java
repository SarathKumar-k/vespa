// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.config.application.api;

import com.yahoo.config.provision.Environment;
import com.yahoo.config.provision.RegionName;
import com.yahoo.text.XML;
import org.w3c.dom.Element;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.Reader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Optional;

/**
 * Specifies the environments and regions to which an application should be deployed.
 * This may be used both for inspection as part of an application model and to answer
 * queries about deployment from the command line. A main method is included for the latter usage.
 *
 * @author bratseth
 */
public class DeploymentSpec {

    private final Optional<String> globalServiceId;
    private final List<DeclaredZone> zones;

    public DeploymentSpec(List<DeclaredZone> zones, Optional<String> globalServiceId) {
        this.zones = Collections.unmodifiableList(new ArrayList<>(zones));
        this.globalServiceId = globalServiceId;
    }

    /** Returns the zones this declares as a read-only list. */
    public List<DeclaredZone> zones() { return zones; }

    /** Returns whether this deployment spec specifies the given zone, either implicitly or explicitly */
    public boolean includes(Environment environment, Optional<RegionName> region) {
        for (DeclaredZone declaredZone : zones)
            if (declaredZone.matches(environment, region)) return true;
        return false;
    }

    /** Returns the ID of the service to expose through global routing, if present */
    public Optional<String> globalServiceId() {
        return globalServiceId;
    }

    /**
     * Creates a deployment spec from XML.
     *
     * @throws IllegalArgumentException if the XML is invalid
     */
    public static DeploymentSpec fromXml(Reader reader) {
        List<DeclaredZone> zones = new ArrayList<>();
        Element root = XML.getDocument(reader).getDocumentElement();
        Optional<String> globalServiceId = Optional.empty();
        for (Element environmentTag : XML.getChildren(root)) {
            Environment environment = Environment.from(environmentTag.getTagName());
            List<Element> regionTags = XML.getChildren(environmentTag, "region");
            if (regionTags.isEmpty()) {
                zones.add(new DeclaredZone(environment, Optional.empty(), false));
            }
            else {
                for (Element regionTag : regionTags) {
                    RegionName region = RegionName.from(XML.getValue(regionTag).trim());
                    boolean active = environment == Environment.prod && readActive(regionTag);
                    zones.add(new DeclaredZone(environment, Optional.of(region), active));
                }
            }

            if (Environment.prod.equals(environment)) {
                globalServiceId = readGlobalServiceId(environmentTag);
            } else if (readGlobalServiceId(environmentTag).isPresent()) {
                throw new IllegalArgumentException("Attribute 'global-service-id' is only valid on 'prod' tag.");
            }
        }
        return new DeploymentSpec(zones, globalServiceId);
    }

    private static Optional<String> readGlobalServiceId(Element environmentTag) {
        String globalServiceId = environmentTag.getAttribute("global-service-id");
        if (globalServiceId == null || globalServiceId.isEmpty()) {
            return Optional.empty();
        }
        else {
            return Optional.of(globalServiceId);
        }
    }

    private static boolean readActive(Element regionTag) {
        String activeValue = regionTag.getAttribute("active");
        if ("true".equals(activeValue)) return true;
        if ("false".equals(activeValue)) return false;
        throw new IllegalArgumentException("Region tags must have an 'active' attribute set to 'true' or 'false' " +
                                           "to control whether the region should receive production traffic");
    }

    public static String toMessageString(Throwable t) {
        StringBuilder b = new StringBuilder();
        String lastMessage = null;
        String message;
        for (; t != null; t = t.getCause()) {
            message = t.getMessage();
            if (message == null) continue;
            if (message.equals(lastMessage)) continue;
            if (b.length() > 0) {
                b.append(": ");
            }
            b.append(message);
            lastMessage = message;
        }
        return b.toString();
    }

    /** This may be invoked by a continuous build */
    public static void main (String[] args) {
        if (args.length != 2 && args.length != 3) {
            System.err.println("Usage: DeploymentSpec [file] [environment] [region]?" +
                               "Returns 0 if the specified zone matches the deployment spec, 1 otherwise");
            System.exit(1);
        }

        try (BufferedReader reader = new BufferedReader(new FileReader(args[0]))) {
            DeploymentSpec spec = DeploymentSpec.fromXml(reader);
            Environment environment = Environment.from(args[1]);
            Optional<RegionName> region = args.length == 3 ? Optional.of(RegionName.from(args[2])) : Optional.empty();
            if (spec.includes(environment, region))
                System.exit(0);
            else
                System.exit(1);
        }
        catch (Exception e) {
            System.err.println("Exception checking deployment spec: " + toMessageString(e));
            System.exit(1);
        }
    }

    public static class DeclaredZone {

        private final Environment environment;

        private Optional<RegionName> region;

        private final boolean active;

        public DeclaredZone(Environment environment, Optional<RegionName> region, boolean active) {
            this.environment = environment;
            this.region = region;
            this.active = active;
        }

        public Environment environment() { return environment; }

        /** The region name, or empty if not declared */
        public Optional<RegionName> region() { return region; }

        /** Returns whether this zone should receive production traffic */
        public boolean active() { return active; }

        public boolean matches(Environment environment, Optional<RegionName> region) {
            if (environment.equals(this.environment) && region.equals(this.region)) return true;
            if ( ! region.isPresent() && prerequisite(environment)) return true;
            return false;
        }

        /**
         * Returns whether deployment in the given environment is a prerequisite of deployment in this environment
         *
         * The required progression leading to prerequisites is test, staging, prod.
         */
        private boolean prerequisite(Environment environment) {
            if (this.environment == Environment.prod)
                return environment == Environment.staging || environment == Environment.test;
            if (this.environment == Environment.staging)
                return environment == Environment.test;
            return false;
        }

    }

}
