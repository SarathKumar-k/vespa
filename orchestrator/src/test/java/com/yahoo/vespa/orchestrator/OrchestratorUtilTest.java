// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.orchestrator;

import com.yahoo.config.provision.ApplicationId;
import com.yahoo.config.provision.ApplicationName;
import com.yahoo.config.provision.InstanceName;
import com.yahoo.config.provision.TenantName;
import com.yahoo.vespa.applicationmodel.ApplicationInstanceId;
import com.yahoo.vespa.applicationmodel.ApplicationInstanceReference;
import com.yahoo.vespa.applicationmodel.TenantId;
import org.junit.Assert;
import org.junit.Test;

/**
 * @author <a href="mailto:smorgrav@yahoo-inc.com">Toby</a>
 */
public class OrchestratorUtilTest {

    private static final ApplicationId APPID_1 = ApplicationId.from(
            TenantName.from("mediasearch"),
            ApplicationName.from("tumblr-search"),
            InstanceName.defaultName());

    private static final ApplicationInstanceReference APPREF_1 = new ApplicationInstanceReference(
            new TenantId("test-tenant"),
            new ApplicationInstanceId("test-application:test-environment:test-region:test-instance-key"));

    /**
     * Here we don't care how the internal of the different application
     * id/reference look like as long as we get back to exactly where we
     * started from a round trip. I.e I'm not testing validity of the
     * different representations.
     */
    @Test
    public void applicationid_conversion_are_symmetric() throws Exception {

        // From appId to appRef and back
        ApplicationInstanceReference appRef = OrchestratorUtil.toApplicationInstanceReference(APPID_1);
        ApplicationId appIdRoundTrip = OrchestratorUtil.toApplicationId(appRef);

        Assert.assertEquals(APPID_1, appIdRoundTrip);

        // From appRef to appId and back
        ApplicationId appId = OrchestratorUtil.toApplicationId(APPREF_1);
        ApplicationInstanceReference appRefRoundTrip = OrchestratorUtil.toApplicationInstanceReference(appId);

        Assert.assertEquals(APPREF_1, appRefRoundTrip);
    }
}