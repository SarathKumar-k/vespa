// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.prelude.semantics.test;

import com.yahoo.search.Query;

/**
 * Tests blending rules
 *
 * @author <a href="bratseth@yahoo-inc.com">Jon Bratseth</a>
 */
public class BlendingTestCase extends RuleBaseAbstractTestCase {

    public BlendingTestCase(String name) {
        super(name,"blending.sr");
    }

    /** Tests parameter literal matching */
    public void testLiteralEquals() {
        assertParameterSemantics("AND a sun came cd","a sun came cd","search","[music]");
        assertParameterSemantics("AND driving audi","driving audi","search","[cars]");
        //assertParameterSemantics("AND audi music quality","audi music quality","search","carstereos",1);
    }

    private void assertParameterSemantics(String producedQuery,String inputQuery,
                                          String producedParameterName,String producedParameterValue) {
        assertParameterSemantics(producedQuery,inputQuery,producedParameterName,producedParameterValue,0);
    }

    private void assertParameterSemantics(String producedQuery,String inputQuery,
                                          String producedParameterName,String producedParameterValue,int tracing) {
        Query query=assertSemantics(producedQuery,inputQuery,tracing);
        assertEquals(producedParameterValue, query.properties().getString(producedParameterName));
    }

}
