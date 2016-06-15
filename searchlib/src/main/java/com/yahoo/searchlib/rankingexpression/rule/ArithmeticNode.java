// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.searchlib.rankingexpression.rule;

import com.google.common.collect.ImmutableList;
import com.yahoo.searchlib.rankingexpression.evaluation.Context;
import com.yahoo.searchlib.rankingexpression.evaluation.Value;

import java.util.*;

/**
 * A binary mathematical operation
 *
 * @author bratseth
 */
public final class ArithmeticNode extends CompositeNode {

    private final ImmutableList<ExpressionNode> children;
    private final ImmutableList<ArithmeticOperator> operators;

    public ArithmeticNode(List<ExpressionNode> children, List<ArithmeticOperator> operators) {
        this.children = ImmutableList.copyOf(children);
        this.operators = ImmutableList.copyOf(operators);
    }

    public ArithmeticNode(ExpressionNode leftExpression, ArithmeticOperator operator, ExpressionNode rightExpression) {
        this.children = ImmutableList.of(leftExpression, rightExpression);
        this.operators = ImmutableList.of(operator);
    }

    public List<ArithmeticOperator> operators() { return operators; }

    @Override
    public List<ExpressionNode> children() { return children; }

    @Override
    public String toString(SerializationContext context, Deque<String> path, CompositeNode parent) {
        StringBuilder string = new StringBuilder();

        boolean nonDefaultPrecedence = nonDefaultPrecedence(parent);
        if (nonDefaultPrecedence)
            string.append("(");

        Iterator<ExpressionNode> child = children.iterator();
        string.append(child.next().toString(context, path, this)).append(" ");
        for (Iterator<ArithmeticOperator> op = operators.iterator(); op.hasNext() && child.hasNext();) {
            string.append(op.next().toString()).append(" ");
            string.append(child.next().toString(context, path, this));
            if (op.hasNext())
                string.append(" ");
        }
        if (nonDefaultPrecedence)
            string.append(")");
        string.append(" ");

        return string.toString().trim();
    }

    /**
     * Returns true if this node has lower precedence than the parent
     * (even though by virtue of being a node it will be calculated before the parent).
     */
    private boolean nonDefaultPrecedence(CompositeNode parent) {
        if ( parent==null) return false;
        if ( ! (parent instanceof ArithmeticNode)) return false;

        return ((ArithmeticNode)parent).operators.get(0).hasPrecedenceOver(this.operators.get(0));
    }

    @Override
    public Value evaluate(Context context) {
        Iterator<ExpressionNode> child = children.iterator();

        Deque<ValueItem> stack = new ArrayDeque<>();
        stack.push(new ValueItem(ArithmeticOperator.PLUS, child.next().evaluate(context)));
        for (Iterator<ArithmeticOperator> it = operators.iterator(); it.hasNext() && child.hasNext();) {
            ArithmeticOperator op = it.next();
            if (!stack.isEmpty()) {
                while (stack.peek().op.hasPrecedenceOver(op)) {
                    popStack(stack);
                }
            }
            stack.push(new ValueItem(op, child.next().evaluate(context)));
        }
        while (stack.size() > 1) {
            popStack(stack);
        }
        return stack.getFirst().value;
    }

    private void popStack(Deque<ValueItem> stack) {
        ValueItem rhs = stack.pop();
        ValueItem lhs = stack.peek();
        lhs.value = rhs.op.evaluate(lhs.value, rhs.value);
    }

    public static ArithmeticNode resolve(ExpressionNode left, ArithmeticOperator op, ExpressionNode right) {
        if ( ! (left instanceof ArithmeticNode)) return new ArithmeticNode(left, op, right);

        ArithmeticNode leftArithmetic = (ArithmeticNode)left;

        List<ExpressionNode> newChildren = new ArrayList<>(leftArithmetic.children());
        newChildren.add(right);

        List<ArithmeticOperator> newOperators = new ArrayList<>(leftArithmetic.operators());
        newOperators.add(op);

        return new ArithmeticNode(newChildren, newOperators);
    }

    private static class ValueItem {

        final ArithmeticOperator op;
        Value value;

        public ValueItem(ArithmeticOperator op, Value value) {
            this.op = op;
            this.value = value;
        }
    }

    @Override
    public CompositeNode setChildren(List<ExpressionNode> newChildren) {
        if (children.size() != newChildren.size())
            throw new IllegalArgumentException("Expected " + children.size() + " children but got " + newChildren.size());
        return new ArithmeticNode(newChildren, operators);
    }

}

