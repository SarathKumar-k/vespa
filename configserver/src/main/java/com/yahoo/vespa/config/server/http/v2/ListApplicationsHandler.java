// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.config.server.http.v2;

import com.google.common.base.Function;
import com.google.common.collect.Collections2;
import com.yahoo.config.provision.TenantName;
import com.yahoo.config.provision.Zone;
import com.yahoo.container.jdisc.HttpRequest;
import com.yahoo.container.jdisc.HttpResponse;
import com.yahoo.container.logging.AccessLog;
import com.yahoo.jdisc.Response;
import com.yahoo.vespa.config.server.Tenant;
import com.yahoo.vespa.config.server.Tenants;
import com.yahoo.config.provision.ApplicationId;
import com.yahoo.vespa.config.server.application.ApplicationRepo;
import com.yahoo.vespa.config.server.http.HttpHandler;
import com.yahoo.vespa.config.server.http.Utils;

import java.util.Collection;
import java.util.List;
import java.util.concurrent.Executor;

/**
 * Handler for listing currently active applications for a tenant.
 *
 * @author lulf
 * @since 5.1
 */
public class ListApplicationsHandler extends HttpHandler {
    private final Tenants tenants;
    private final Zone zone;
    public ListApplicationsHandler(Executor executor, AccessLog accessLog, Tenants tenants, Zone zone) {
        super(executor, accessLog);
        this.tenants = tenants;
        this.zone = zone;
    }

    @Override
    public HttpResponse handleGET(HttpRequest request) {
        TenantName tenantName = Utils.getTenantFromApplicationsRequest(request);
        final String urlBase = Utils.getUrlBase(request, "/application/v2/tenant/" + tenantName + "/application/");

        List<ApplicationId> applicationIds = listApplicationIds(tenantName);
        Collection<String> applicationUrls = Collections2.transform(applicationIds, new Function<ApplicationId, String>() {
            @Override
            public String apply(ApplicationId id) {
                return createUrlStringFromId(urlBase, id, zone);
            }
        });
        return new ListApplicationsResponse(Response.Status.OK, applicationUrls);
    }

    private List<ApplicationId> listApplicationIds(TenantName tenantName) {
        Tenant tenant = Utils.checkThatTenantExists(tenants, tenantName);
        ApplicationRepo applicationRepo = tenant.getApplicationRepo();
        return applicationRepo.listApplications();
    }

    private static String createUrlStringFromId(String urlBase, ApplicationId id, Zone zone) {
        StringBuilder sb = new StringBuilder();
        sb.append(urlBase).append(id.application().value());
        sb.append("/environment/").append(zone.environment().value());
        sb.append("/region/").append(zone.region().value());
        sb.append("/instance/").append(id.instance().value());
        return sb.toString();
    }
}
