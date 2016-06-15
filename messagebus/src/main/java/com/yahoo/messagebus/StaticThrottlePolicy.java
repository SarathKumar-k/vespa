// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.messagebus;

/**
 * This is an implementatin of the {@link ThrottlePolicy} that offers static limits to the amount of pending data a
 * {@link SourceSession} is allowed to have. You may choose to set a limit to the total number of pending messages (by
 * way of {@link #setMaxPendingCount(int)}), the total size of pending messages (by way of {@link
 * #setMaxPendingSize(long)}), or some combination thereof.
 *
 * <b>NOTE:</b> By context, "pending" is refering to the number of sent messages that have not been replied to yet.
 *
 * @author <a href="mailto:simon@yahoo-inc.com">Simon Thoresen</a>
 */
public class StaticThrottlePolicy implements ThrottlePolicy {

    private int maxPendingCount = 0;
    private long maxPendingSize = 0;
    private long pendingSize = 0;

    public boolean canSend(Message msg, int pendingCount) {
        if (maxPendingCount > 0 && pendingCount >= maxPendingCount) {
            return false;
        }
        if (maxPendingSize > 0 && pendingSize >= maxPendingSize) {
            return false;
        }
        return true;
    }

    public void processMessage(Message msg) {
        int size = msg.getApproxSize();
        msg.setContext(size);
        pendingSize += size;
    }

    public void processReply(Reply reply) {
        int size = (Integer)reply.getContext();
        pendingSize -= size;
    }

    /**
     * Returns the maximum number of pending messages allowed.
     *
     * @return The max limit.
     */
    public int getMaxPendingCount() {
        return maxPendingCount;
    }

    /**
     * Sets the maximum number of pending messages allowed.
     *
     * @param maxCount The max count.
     * @return This, to allow chaining.
     */
    public StaticThrottlePolicy setMaxPendingCount(int maxCount) {
        maxPendingCount = maxCount;
        return this;
    }

    /**
     * Returns the maximum total size of pending messages allowed.
     *
     * @return The max limit.
     */
    public long getMaxPendingSize() {
        return maxPendingSize;
    }

    /**
     * Sets the maximum total size of pending messages allowed. This size is relative to the value returned by {@link
     * com.yahoo.messagebus.Message#getApproxSize()}.
     *
     * @param maxSize The max size.
     * @return This, to allow chaining.
     */
    public StaticThrottlePolicy setMaxPendingSize(long maxSize) {
        maxPendingSize = maxSize;
        return this;
    }

    /**
     * Returns the total size of pending messages.
     *
     * @return The size.
     */
    public long getPendingSize() {
        return pendingSize;
    }

}
