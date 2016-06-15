// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.indexinglanguage.expressions;

/**
 * @author <a href="mailto:simon@yahoo-inc.com">Simon Thoresen</a>
 */
public class IndexExpression extends OutputExpression {

    public IndexExpression(String fieldName) {
        super("index", fieldName);
    }

    @Override
    public boolean equals(Object obj) {
        return super.equals(obj) && obj instanceof IndexExpression;
    }
}
