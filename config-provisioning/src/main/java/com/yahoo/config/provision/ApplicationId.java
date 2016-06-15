// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.config.provision;

import com.google.inject.Inject;
import com.yahoo.cloud.config.ApplicationIdConfig;

/**
 * A complete, immutable identification of an application instance.
 *
 * @author lulf
 * @author vegard
 * @author bratseth
 * @since 5.1
 */
public final class ApplicationId implements Comparable<ApplicationId> {

    private final TenantName tenant;
    private final ApplicationName application;
    private final InstanceName instance;

    private final String stringValue;
    private final String serializedForm;

    public static class Builder {

        private TenantName tenant;
        private ApplicationName application;
        private InstanceName instance;

        public Builder() {
            this.tenant = TenantName.defaultName();
            this.application = null;
            this.instance = InstanceName.defaultName();
        }

        public Builder tenant(TenantName ten) { this.tenant = ten; return this; }
        public Builder tenant(String ten) { return tenant(TenantName.from(ten)); }

        public Builder applicationName(ApplicationName nam) { this.application = nam; return this; }
        public Builder applicationName(String nam) { return applicationName(ApplicationName.from(nam)); }

        public Builder instanceName(InstanceName ins) { this.instance = ins; return this; }
        public Builder instanceName(String ins) { return instanceName(InstanceName.from(ins)); }

        public ApplicationId build() {
            if (application == null) {
                throw new IllegalArgumentException("must set application name in builder");
            }
            return ApplicationId.from(tenant, application, instance);
        }

    }

    public ApplicationId(ApplicationIdConfig config) {
        this(TenantName.from(config.tenant()), ApplicationName.from(config.application()), InstanceName.from(config.instance()));
    }

    private ApplicationId(TenantName tenant, ApplicationName applicationName, InstanceName instanceName) {
        this.tenant = tenant;
        this.application = applicationName;
        this.instance = instanceName;
        this.stringValue = toStringValue();
        this.serializedForm = toSerializedForm();
    }

    public static final TenantName HOSTED_VESPA_TENANT = TenantName.from("hosted-vespa");
    // TODO: Remove references to routing application, or rename them to zone
    // application, once everything (like Chef recipes) refers to the zone
    // application name.
    public static final ApplicationName ROUTING_APPLICATION = ApplicationName.from("routing");
    public static final ApplicationName ZONE_APPLICATION = ApplicationName.from("zone");
    public static final ApplicationId HOSTED_ZONE_APPLICATION_ID =
        new ApplicationId.Builder()
        .tenant(HOSTED_VESPA_TENANT)
        .applicationName(ROUTING_APPLICATION)
        .build();


    public boolean isHostedVespaRoutingApplication() {
        return HOSTED_VESPA_TENANT.equals(tenant) &&
	    (ROUTING_APPLICATION.equals(application) ||
	     ZONE_APPLICATION.equals(application));
    }

    public static ApplicationId from(TenantName tenant, ApplicationName application, InstanceName instanceName) {
        return new ApplicationId(tenant, application, instanceName);
    }

    /** Creates an application id from a string on the form application:environment:region:instance */
    public static ApplicationId fromSerializedForm(TenantName tenant, String idString) {
        String[] parts = idString.split(":");
        if (parts.length < 3) {
            throw new IllegalArgumentException("Illegal id string '" + idString + "'. Id string must consist of at least three parts separated by ':'");
        }

        // Legacy id from 5.50 and backwards
        if (parts.length < 4) {
            return new Builder()
                    .tenant(parts[0])
                    .applicationName(parts[1])
                    .instanceName(parts[2])
                    .build();

        } else {
            return new Builder()
                    .applicationName(parts[0])
                    .instanceName(parts[3])
                    .tenant(tenant)
                    .build();
        }
    }

    @Override
    public int hashCode() { return stringValue.hashCode(); }

    @Override
    public boolean equals(Object other) {
        if (this == other) return true;
        if (other == null || getClass() != other.getClass()) return false;

        ApplicationId rhs = (ApplicationId) other;
        return tenant.equals(rhs.tenant) &&
               application.equals(rhs.application) &&
               instance.equals(rhs.instance);
    }

    /** Returns a serialized form of the content of this: tenant:application:instance */
    public String serializedForm() { return serializedForm; }

    private String toStringValue() {
        return "tenant '" + tenant + "', application '" + application + "', instance '" + instance + "'";
    }

    private String toSerializedForm() {
        return tenant + ":" + application + ":" + instance;
    }

    @Override
    public String toString() { return stringValue; }

    public TenantName tenant() { return tenant; }
    public ApplicationName application() { return application; }
    public InstanceName instance() { return instance; }

    @Override
    public int compareTo(ApplicationId other) {
        int diff;

        diff = tenant.compareTo(other.tenant);
        if (diff != 0) { return diff; }

        diff = application.compareTo(other.application);
        if (diff != 0) { return diff; }

        diff = instance.compareTo(other.instance);
        if (diff != 0) { return diff; }

        return 0;
    }

    /** Returns an application id where all fields are "default" */
    public static ApplicationId defaultId() {
        return new ApplicationId(TenantName.defaultName(), ApplicationName.defaultName(), InstanceName.defaultName());
    }

    /** Returns an application id where all fields are "*" */
    public static ApplicationId global() { // TODO: Sukk ... get rid of this
        return new Builder().tenant("*")
                            .applicationName("*")
                            .instanceName("*")
                            .build();
    }

}
