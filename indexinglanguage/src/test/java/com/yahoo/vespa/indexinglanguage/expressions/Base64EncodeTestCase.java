// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.indexinglanguage.expressions;

import com.yahoo.document.DataType;
import com.yahoo.document.datatypes.FieldValue;
import com.yahoo.document.datatypes.LongFieldValue;
import com.yahoo.document.datatypes.StringFieldValue;
import com.yahoo.vespa.indexinglanguage.SimpleTestAdapter;
import org.junit.Test;

import static com.yahoo.vespa.indexinglanguage.expressions.ExpressionAssert.assertVerify;
import static com.yahoo.vespa.indexinglanguage.expressions.ExpressionAssert.assertVerifyThrows;
import static org.junit.Assert.*;

/**
 * @author <a href="mailto:simon@yahoo-inc.com">Simon Thoresen</a>
 */
public class Base64EncodeTestCase {

    @Test
    public void requireThatHashCodeAndEqualsAreImplemented() {
        Expression exp = new Base64EncodeExpression();
        assertFalse(exp.equals(new Object()));
        assertEquals(exp, new Base64EncodeExpression());
        assertEquals(exp.hashCode(), new Base64EncodeExpression().hashCode());
    }

    @Test
    public void requireThatInputIsEncoded() {
        ExecutionContext ctx = new ExecutionContext(new SimpleTestAdapter());
        ctx.setValue(new LongFieldValue(489210573L));
        new Base64EncodeExpression().execute(ctx);

        FieldValue val = ctx.getValue();
        assertTrue(val instanceof StringFieldValue);
        assertEquals("zcIoHQAAAAA=", ((StringFieldValue)val).getString());
    }

    @Test
    public void requireThatExpressionCanBeVerified() {
        Expression exp = new Base64EncodeExpression();
        assertVerify(DataType.LONG, exp, DataType.STRING);
        assertVerifyThrows(null, exp, "Expected long input, got null.");
        assertVerifyThrows(DataType.STRING, exp, "Expected long input, got string.");
    }
}
