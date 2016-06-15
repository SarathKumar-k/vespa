// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.config.server;

import com.yahoo.cloud.config.LbServicesConfig;
import com.yahoo.cloud.config.ElkConfig;
import com.yahoo.config.model.application.provider.FilesApplicationPackage;
import com.yahoo.config.provision.*;
import com.yahoo.jrt.Request;
import com.yahoo.vespa.config.ConfigKey;
import com.yahoo.vespa.config.GetConfigRequest;
import com.yahoo.cloud.config.LbServicesConfig.Tenants.Applications;
import com.yahoo.vespa.config.protocol.CompressionType;
import com.yahoo.vespa.config.protocol.ConfigResponse;
import com.yahoo.vespa.config.protocol.DefContent;
import com.yahoo.vespa.config.protocol.JRTClientConfigRequestV3;
import com.yahoo.vespa.config.protocol.JRTServerConfigRequestV3;
import com.yahoo.vespa.config.protocol.Trace;
import com.yahoo.vespa.config.protocol.VespaVersion;
import com.yahoo.vespa.config.server.application.Application;
import com.yahoo.vespa.config.server.model.SuperModel;
import com.yahoo.vespa.config.server.monitoring.MetricUpdater;
import com.yahoo.vespa.model.VespaModel;

import org.junit.Before;
import org.junit.Test;
import org.xml.sax.SAXException;

import java.io.File;
import java.io.IOException;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Optional;

import com.yahoo.cloud.config.ElkConfig.Logstash;

import com.yahoo.vespa.config.server.model.ElkProducer;
import static org.hamcrest.core.Is.is;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;

/**
 * @author lulf
 * @since 5.9
 */
public class SuperModelRequestHandlerTest {

    private SuperModelRequestHandler handler;

    @Before
    public void setupHandler() throws IOException, SAXException {
        Map<TenantName, Map<ApplicationId, Application>> models = new LinkedHashMap<>();
        models.put(TenantName.from("a"), new LinkedHashMap<>());
        File testApp = new File("src/test/resources/deploy/app");
        ApplicationId app = ApplicationId.from(TenantName.from("a"),
                                               ApplicationName.from("foo"), InstanceName.defaultName());
        models.get(app.tenant()).put(app, new Application(new VespaModel(FilesApplicationPackage.fromFile(testApp)), new ServerCache(), 4l, Version.fromIntValues(1, 2, 3), MetricUpdater.createTestUpdater(), app));
        handler = new SuperModelRequestHandler(new SuperModel(models, new ElkConfig(new ElkConfig.Builder()), Zone.defaultZone()), new TestConfigDefinitionRepo(), 2, new UncompressedConfigResponseFactory());
    }
    
    @Test
    public void test_super_model_resolve_elk() {
        ConfigResponse response = handler.resolveConfig(new GetConfigRequest() {
            @Override
            public ConfigKey<?> getConfigKey() {
                return new ConfigKey<>(ElkConfig.class, "dontcare");
            }

            @Override
            public DefContent getDefContent() {
                return DefContent.fromClass(ElkConfig.class);
            }

            @Override
            public Optional<VespaVersion> getVespaVersion() {
                return Optional.empty();
            }

            @Override
            public boolean noCache() {
                return false;
            }
        });
        assertThat(response.getGeneration(), is(2l));
    }
    
    @Test
    public void test_lb_config_simple() {
        LbServicesConfig.Builder lb = new LbServicesConfig.Builder();
        handler.getSuperModel().getConfig(lb);
        LbServicesConfig lbc = new LbServicesConfig(lb);
        assertThat(lbc.tenants().size(), is(1));
        assertThat(lbc.tenants("a").applications().size(), is(1));
        Applications app = lbc.tenants("a").applications("foo:prod:default:default");
        assertTrue(app.hosts().size() > 0);
    }


    @Test(expected = UnknownConfigDefinitionException.class)
    public void test_unknown_config_definition() {
        String md5 = "asdfasf";
        Request request = JRTClientConfigRequestV3.createWithParams(new ConfigKey<>("foo", "id", "bar", md5, null), DefContent.fromList(Collections.emptyList()),
                                                                    "fromHost", md5, 1, 1, Trace.createDummy(), CompressionType.UNCOMPRESSED,
                                                                    Optional.empty())
                                                  .getRequest();
        JRTServerConfigRequestV3 v3Request = JRTServerConfigRequestV3.createFromRequest(request);
        handler.resolveConfig(v3Request);
    }

    @Test
    public void test_lb_config_multiple_apps() throws IOException, SAXException {
        Map<TenantName, Map<ApplicationId, Application>> models = new LinkedHashMap<>();
        models.put(TenantName.from("t1"), new LinkedHashMap<>());
        models.put(TenantName.from("t2"), new LinkedHashMap<>());
        File testApp1 = new File("src/test/resources/deploy/app");
        File testApp2 = new File("src/test/resources/deploy/advancedapp");
        File testApp3 = new File("src/test/resources/deploy/advancedapp");
        // TODO must fix equals, hashCode on Tenant
        Version vespaVersion = Version.fromIntValues(1, 2, 3);
        models.get(TenantName.from("t1")).put(applicationId("mysimpleapp"),
                new Application(new VespaModel(FilesApplicationPackage.fromFile(testApp1)), new ServerCache(), 4l, vespaVersion, MetricUpdater.createTestUpdater(), applicationId("mysimpleapp")));
        models.get(TenantName.from("t1")).put(applicationId("myadvancedapp"),
                new Application(new VespaModel(FilesApplicationPackage.fromFile(testApp2)), new ServerCache(), 4l, vespaVersion, MetricUpdater.createTestUpdater(), applicationId("myadvancedapp")));
        models.get(TenantName.from("t2")).put(applicationId("minetooadvancedapp"),
                new Application(new VespaModel(FilesApplicationPackage.fromFile(testApp3)), new ServerCache(), 4l, vespaVersion, MetricUpdater.createTestUpdater(), applicationId("minetooadvancedapp")));

        SuperModelRequestHandler han = new SuperModelRequestHandler(new SuperModel(models, new ElkConfig(new ElkConfig.Builder()), Zone.defaultZone()), new TestConfigDefinitionRepo(), 2, new UncompressedConfigResponseFactory());
        LbServicesConfig.Builder lb = new LbServicesConfig.Builder();
        han.getSuperModel().getConfig(lb);
        LbServicesConfig lbc = new LbServicesConfig(lb);
        assertThat(lbc.tenants().size(), is(2));
        assertThat(lbc.tenants("t1").applications().size(), is(2));
        assertThat(lbc.tenants("t2").applications().size(), is(1));
        assertThat(lbc.tenants("t2").applications("minetooadvancedapp:prod:default:default").hosts().size(), is(1));
        assertQrServer(lbc.tenants("t2").applications("minetooadvancedapp:prod:default:default"));
    }

    private ApplicationId applicationId(String applicationName) {
        return ApplicationId.from(TenantName.defaultName(),
                                  ApplicationName.from(applicationName), InstanceName.defaultName());
    }

    private void assertQrServer(Applications app) {
        String host = app.hosts().keySet().iterator().next();
        Applications.Hosts hosts = app.hosts(host);
        assertThat(hosts.hostname(), is(host));
        for (Map.Entry<String, Applications.Hosts.Services> e : app.hosts(host).services().entrySet()) {
            System.out.println(e);
            if ("qrserver".equals(e.getKey())) {
                Applications.Hosts.Services s = e.getValue();
                assertThat(s.type(), is("qrserver"));
                assertThat(s.ports().size(), is(4));
                assertThat(s.index(), is(0));
                return;
            }
        }
        org.junit.Assert.fail("No qrserver service in config");
    }

    @Test
    public void testElkConfig() {
        ElkConfig ec = new ElkConfig(new ElkConfig.Builder().elasticsearch(new ElkConfig.Elasticsearch.Builder().host("es1").port(99)).
                        logstash(new ElkConfig.Logstash.Builder().
                                        config_file("/cfgfile").
                                        source_field("srcfield").
                                        spool_size(345).
                                        network(new Logstash.Network.Builder().
                                                        servers(new Logstash.Network.Servers.Builder().
                                                                        host("ls1").
                                                                        port(999)).
                                                        servers(new Logstash.Network.Servers.Builder().
                                                                        host("ls2").
                                                                        port(998)).          
                                                        timeout(78)).
                                        files(new ElkConfig.Logstash.Files.Builder().
                                                        paths("path1").
                                                        paths("path2").
                                                        fields("field1", "f1val").
                                                        fields("field2", "f2val"))));
        ElkProducer ep = new ElkProducer(ec);
        ElkConfig.Builder newBuilder = new ElkConfig.Builder();
        ep.getConfig(newBuilder);
        ElkConfig elkConfig = new ElkConfig(newBuilder);
        assertThat(elkConfig.elasticsearch(0).host(), is("es1"));
        assertThat(elkConfig.elasticsearch(0).port(), is(99));        
        assertThat(elkConfig.logstash().network().servers(0).host(), is("ls1"));
        assertThat(elkConfig.logstash().network().servers(0).port(), is(999));
        assertThat(elkConfig.logstash().network().servers(1).host(), is("ls2"));
        assertThat(elkConfig.logstash().network().servers(1).port(), is(998));
        assertThat(elkConfig.logstash().network().timeout(), is(78));
        assertThat(elkConfig.logstash().config_file(), is("/cfgfile"));
        assertThat(elkConfig.logstash().source_field(), is("srcfield"));
        assertThat(elkConfig.logstash().spool_size(), is(345));
        assertThat(elkConfig.logstash().files().size(), is(1));
        assertThat(elkConfig.logstash().files(0).paths(0), is("path1"));
        assertThat(elkConfig.logstash().files(0).paths(1), is("path2"));
        assertThat(elkConfig.logstash().files(0).fields("field1"), is("f1val"));
        assertThat(elkConfig.logstash().files(0).fields("field2"), is("f2val"));
    }
 }



