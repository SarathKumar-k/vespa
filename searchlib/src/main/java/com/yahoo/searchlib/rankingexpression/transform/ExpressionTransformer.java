// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.searchlib.rankingexpression.transform;

import com.yahoo.searchlib.rankingexpression.RankingExpression;
import com.yahoo.searchlib.rankingexpression.rule.CompositeNode;
import com.yahoo.searchlib.rankingexpression.rule.ExpressionNode;

import java.util.ArrayList;
import java.util.List;

/**
 * Superclass of expression transformers
 *
 * @author <a href="mailto:bratseth@yahoo-inc.com">Jon Bratseth</a>
 */
public abstract class ExpressionTransformer {

    public RankingExpression transform(RankingExpression expression) {
        return new RankingExpression(expression.getName(), transform(expression.getRoot()));
    }

    /** Transforms an expression node and returns the transformed node */
    public abstract ExpressionNode transform(ExpressionNode node);

    /**
     * Utility method which calls transform on each child of the given node and return the resulting transformed
     * composite
     */
    protected CompositeNode transformChildren(CompositeNode node) {
        List<ExpressionNode> children = node.children();
        List<ExpressionNode> transformedChildren = new ArrayList<>(children.size());
        for (ExpressionNode child : children)
            transformedChildren.add(transform(child));
        return node.setChildren(transformedChildren);
    }


}
