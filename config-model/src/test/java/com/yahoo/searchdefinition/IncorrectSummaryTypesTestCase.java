// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.searchdefinition;

import com.yahoo.searchdefinition.parser.ParseException;
import org.junit.Test;

import java.io.IOException;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;
/**
 * Tests importing a search definition with conflicting summary types
 *
 * @author <a href="bratseth@yahoo-inc.com">Jon Bratseth</a>
 */
public class IncorrectSummaryTypesTestCase extends SearchDefinitionTestCase {
    @Test
    public void testImportingIncorrect() throws IOException, ParseException {
        try {
            SearchBuilder.buildFromFile("src/test/examples/incorrectsummarytypes.sd");
            fail("processing should have failed");
        } catch (RuntimeException e) {
            assertEquals("'summary somestring type string' in 'destinations(default )' is inconsistent with 'summary somestring type int' in 'destinations(incorrect )': All declarations of the same summary field must have the same type", e.getMessage());
        }
    }

}
