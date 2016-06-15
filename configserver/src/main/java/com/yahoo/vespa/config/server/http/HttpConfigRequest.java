// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.config.server.http;

import java.util.Collections;
import java.util.Optional;
import java.util.Set;

import javax.annotation.Nullable;

import com.google.common.base.Predicate;
import com.google.common.collect.Iterables;
import com.yahoo.collections.Tuple2;
import com.yahoo.config.provision.ApplicationName;
import com.yahoo.config.provision.TenantName;
import com.yahoo.container.jdisc.HttpRequest;
import com.yahoo.jdisc.application.BindingMatch;
import com.yahoo.vespa.config.ConfigKey;
import com.yahoo.vespa.config.GetConfigRequest;
import com.yahoo.vespa.config.protocol.DefContent;
import com.yahoo.vespa.config.protocol.VespaVersion;
import com.yahoo.vespa.config.server.RequestHandler;
import com.yahoo.config.provision.ApplicationId;
import com.yahoo.vespa.config.server.http.v2.HttpConfigRequests;
import com.yahoo.vespa.config.server.http.v2.TenantRequest;
import com.yahoo.vespa.config.util.ConfigUtils;

/**
 * A request to get config, bound to tenant and app id. Used by both v1 and v2 of the config REST API.
 *
 * @author lulf
 * @since 5.1
 */
public class HttpConfigRequest implements GetConfigRequest, TenantRequest {
    private static final String DEFAULT_TENANT = "default";
    private static final String HTTP_PROPERTY_NOCACHE = "noCache";
    private final ConfigKey<?> key;
    private final ApplicationId appId;
    private final boolean noCache;

    private HttpConfigRequest(ConfigKey<?> key, ApplicationId appId, boolean noCache) {
        this.key = key;
        this.appId = appId;
        this.noCache = noCache;
    }

    private static ConfigKey<?> fromRequestV1(HttpRequest req) {
        BindingMatch<?> bm = Utils.getBindingMatch(req, "http://*/config/v1/*/*"); // see jdisc-bindings.cfg
        String conf = bm.group(2); // The port number is implicitly 1, it seems
        String cId;
        String cName;
        String cNamespace;
        if (bm.groupCount() >= 4) {
            cId = bm.group(3);
        } else {
            cId = "";
        }
        Tuple2<String, String> nns = nameAndNamespace(conf);
        cName = nns.first;
        cNamespace = nns.second;
        return new ConfigKey<>(cName, cId, cNamespace);
    }

    public static HttpConfigRequest createFromRequestV1(HttpRequest req) {
        return new HttpConfigRequest(fromRequestV1(req), ApplicationId.defaultId(), req.getBooleanProperty(HTTP_PROPERTY_NOCACHE));
    }

    public static HttpConfigRequest createFromRequestV2(HttpRequest req) {
        // Four bindings for this: with full app id or only name, with and without config id (like v1)
        BindingMatch<?> bm = HttpConfigRequests.getBindingMatch(req,
                "http://*/config/v2/tenant/*/application/*/environment/*/region/*/instance/*/*/*",
                "http://*/config/v2/tenant/*/application/*/*/*");
        if (bm.groupCount() > 6) return createFromRequestV2FullAppId(req, bm);
        return createFromRequestV2SimpleAppId(req, bm);
    }

    // The URL pattern with only tenant and application given
    private static HttpConfigRequest createFromRequestV2SimpleAppId(HttpRequest req, BindingMatch<?> bm) {
        String cId;
        String cName;
        String cNamespace;
        TenantName tenant = TenantName.from(bm.group(2));
        ApplicationName application = ApplicationName.from(bm.group(3));
        String conf = bm.group(4);
        if (bm.groupCount() >= 6) {
            cId = bm.group(5);
        } else {
            cId = "";
        }
        Tuple2<String, String> nns = nameAndNamespace(conf);
        cName = nns.first;
        cNamespace = nns.second;
        return new HttpConfigRequest(new ConfigKey<>(cName, cId, cNamespace),
                                     new ApplicationId.Builder().applicationName(application).tenant(tenant).build(),
                                     req.getBooleanProperty(HTTP_PROPERTY_NOCACHE));
    }

    // The URL pattern with full app id given
    private static HttpConfigRequest createFromRequestV2FullAppId(HttpRequest req, BindingMatch<?> bm) {
        String cId;
        String cName;
        String cNamespace;
        String tenant = bm.group(2);
        String application = bm.group(3);
        String environment = bm.group(4);
        String region = bm.group(5);
        String instance = bm.group(6);
        String conf = bm.group(7);
        if (bm.groupCount() >= 9) {
            cId = bm.group(8);
        } else {
            cId = "";
        }
        Tuple2<String, String> nns = nameAndNamespace(conf);
        cName = nns.first;
        cNamespace = nns.second;

        ApplicationId appId = new ApplicationId.Builder()
                              .tenant(tenant)
                              .applicationName(application)
                              .instanceName(instance)
                              .build();
        return new HttpConfigRequest(new ConfigKey<>(cName, cId, cNamespace), appId, req.getBooleanProperty(HTTP_PROPERTY_NOCACHE));
    }

    /**
     * Throws an exception if bad config or config id
     *
     * @param requestKey     a {@link com.yahoo.vespa.config.ConfigKey}
     * @param requestHandler a {@link RequestHandler}
     * @param appId appId
     */
    public static void validateRequestKey(ConfigKey<?> requestKey, RequestHandler requestHandler, ApplicationId appId) {
        Set<ConfigKey<?>> allConfigsProduced = requestHandler.allConfigsProduced(appId, Optional.empty());
        if (allConfigsProduced.isEmpty()) {
            // This will happen if the configserver is starting up, but has not built a config model
            throwModelNotReady();
        }
        if (configNameNotFound(requestKey, allConfigsProduced)) {
            throw new NotFoundException("No such config: " + requestKey.getNamespace() + "." + requestKey.getName());
        }
        if (configIdNotFound(requestHandler, requestKey, appId)) {
            throw new NotFoundException("No such config id: " + requestKey.getConfigId());
        }
    }    
    
    public static void throwModelNotReady() {
        throw new NotFoundException("Config not available, verify that an application package has been deployed and activated.");
    }
    
    /**
     * If the given config is produced by the model at all
     *
     * @return ok or not
     */
    private static boolean configNameNotFound(final ConfigKey<?> requestKey, Set<ConfigKey<?>> allConfigsProduced) {
        return !Iterables.any(allConfigsProduced, new Predicate<ConfigKey<?>>() {
            @Override
            public boolean apply(@Nullable ConfigKey<?> k) {
                return k.getName().equals(requestKey.getName()) && k.getNamespace().equals(requestKey.getNamespace());
            }
        });
    }

    private static boolean configIdNotFound(RequestHandler requestHandler, ConfigKey<?> requestKey, ApplicationId appId) {
      return !requestHandler.allConfigIds(appId, Optional.empty()).contains(requestKey.getConfigId());
    }
    
    public static Tuple2<String, String> nameAndNamespace(String nsDotName) {
        Tuple2<String, String> ret = ConfigUtils.getNameAndNamespaceFromString(nsDotName);
        if ("".equals(ret.second)) throw new IllegalArgumentException("Illegal config, must be of form namespace.name.");
        return ret;
    }

    @Override
    public ConfigKey<?> getConfigKey() {
        return key;
    }

    @Override
    public DefContent getDefContent() {
        return DefContent.fromList(Collections.<String>emptyList());
    }

    @Override
    public Optional<VespaVersion> getVespaVersion() {
        return Optional.empty();
    }

    @Override
    public ApplicationId getApplicationId() {
        return appId;
    }

    public boolean noCache() {
        return noCache;
    }
}
