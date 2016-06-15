// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.searchdefinition.derived;

import com.yahoo.searchdefinition.parser.ParseException;
import org.junit.Test;

import java.io.IOException;

/**
 * @author <a href="mailto:jon@zenior.no">Jon Bratseth</a>
 */
public class IntegerAttributeToStringIndexTestCase extends AbstractExportingTestCase {
    @Test
    public void testIt() throws IOException, ParseException {
        assertCorrectDeriving("integerattributetostringindex");
    }
}
