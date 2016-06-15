// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.searchlib.expression;

/**
 * This abstract expression node represents a document whose content is accessed depending on the subclass
 * implementation of this.
 *
 * @author <a href="mailto:balder@yahoo-inc.com">Henning Baldersheim</a>
 * @author <a href="mailto:simon@yahoo-inc.com">Simon Thoresen</a>
 */
public abstract class DocumentAccessorNode extends ExpressionNode {

    public static final int classId = registerClass(0x4000 + 48, FunctionNode.class);

    @Override
    protected int onGetClassId() {
        return classId;
    }
}
