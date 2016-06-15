// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.yolean.trace;

/**
 * <p>This class is an abstract visitor of {@link TraceNode}. See {@link TraceNode#accept(TraceVisitor)}.</p>
 *
 * @author <a href="mailto:bratseth@yahoo-inc.com">Jon Bratseth</a>
 * @since 5.1.15
 */
public abstract class TraceVisitor {

    /**
     * <p>Visits a {@link TraceNode}. Called by {@link TraceNode#accept(TraceVisitor)}, before visiting its
     * children.</p>
     *
     * @param node the <tt>TraceNode</tt> being visited
     * @see TraceNode#accept(TraceVisitor)
     */
    public abstract void visit(TraceNode node);

    /**
     * <p>Enters a {@link TraceNode}. This method is called after {@link #visit(TraceNode)}, but before visiting its
     * children. Note that this method is NOT called if a <tt>TraceNode</tt> has zero children.</p>
     * <p>The default implementation of this method does nothing.</p>
     *
     * @param node the <tt>TraceNode</tt> being entered
     * @see #leaving(TraceNode)
     */
    @SuppressWarnings("UnusedParameters")
    public void entering(TraceNode node) {
        // empty
    }

    /**
     * <p>Leaves a {@link TraceNode}. This method is called after {@link #entering(TraceNode)}, and after visiting its
     * children. Note that this method is NOT called if a <tt>TraceNode</tt> has zero children.</p>
     * <p>The default implementation of this method does nothing.</p>
     *
     * @param node the <tt>TraceNode</tt> being left
     */
    @SuppressWarnings("UnusedParameters")
    public void leaving(TraceNode node) {
        // empty
    }
}
