// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.searchlib.expression;

/**
 * This function is an instruction to concatenate the bits of all arguments in order.
 *
 * @author <a href="mailto:balder@yahoo-inc.com">Henning Baldersheim</a>
 * @author <a href="mailto:simon@yahoo-inc.com">Simon Thoresen</a>
 */
public class StrCatFunctionNode extends MultiArgFunctionNode {

    public static final int classId = registerClass(0x4000 + 133, StrCatFunctionNode.class);

    @Override
    protected int onGetClassId() {
        return classId;
    }

    @Override
    protected boolean equalsMultiArgFunction(MultiArgFunctionNode obj) {
        return true;
    }

    @Override
    protected void onPrepareResult() {
        setResult(new StringResultNode());
    }

    @Override
    protected void onPrepare() {
        super.onPrepare();
    }

    @Override
    protected boolean onExecute() {
        for (int i = 0; i < getNumArgs(); i++) {
            getArg(i).execute();
            ((StringResultNode)getResult()).append(getArg(i).getResult());
        }
        return true;
    }
}
