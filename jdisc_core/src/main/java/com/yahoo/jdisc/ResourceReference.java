// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.jdisc;

/**
 * <p>Represents a live reference to a {@link SharedResource}. Only provides the ability to release the reference.</p>
 *
 * <p>Implements {@link AutoCloseable} so that it can be used in try-with-resources statements. Example</p>
 * <pre>
 *     void doSomethingWithRequest(final Request request) {
 *         try (final ResourceReference ref = request.refer()) {
 *             // Do something with request
 *         }
 *         // ref.close() will be called automatically on exit from the try block, releasing the reference on 'request'.
 *     }
 * </pre>
 *
 * @author <a href="mailto:bakksjo@yahoo-inc.com">Oyvind Bakksjo</a>
 */
public interface ResourceReference extends AutoCloseable {

    /**
     * <p>Decrements the reference count of the referenced resource.
     * You call this method once you are done using an object
     * that you have previously {@link SharedResource#refer() referred}.</p>
     *
     * <p>Note that this method is NOT idempotent; you must call it exactly once.</p>
     *
     * @see SharedResource#refer()
     */
    @Override
    void close();

}
