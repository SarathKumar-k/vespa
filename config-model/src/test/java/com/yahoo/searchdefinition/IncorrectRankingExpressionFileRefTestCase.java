// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.searchdefinition;

import com.yahoo.searchdefinition.derived.DerivedConfiguration;
import com.yahoo.searchdefinition.parser.ParseException;
import org.junit.Test;

import java.io.IOException;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

/**
 * @author <a href="mailto:bratseth@yahoo-inc.com">Jon Bratseth</a>
 */
public class IncorrectRankingExpressionFileRefTestCase extends SearchDefinitionTestCase {

    @Test
    public void testIncorrectRef() throws IOException, ParseException {
        try {
            RankProfileRegistry registry = new RankProfileRegistry();
            Search search = SearchBuilder.buildFromFile("src/test/examples/incorrectrankingexpressionfileref.sd", registry);
            new DerivedConfiguration(search, registry); // rank profile parsing happens during deriving
            fail("parsing should have failed");
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
            assertTrue(e.getCause().getMessage().contains("Could not read ranking expression file"));
            assertTrue(e.getCause().getMessage().contains("wrongending.expr.expression"));
        }
    }

}
