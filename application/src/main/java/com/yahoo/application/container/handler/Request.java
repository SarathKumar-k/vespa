// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.application.container.handler;

import com.google.common.annotations.Beta;
import net.jcip.annotations.Immutable;

/**
 * A request for use with {@link com.yahoo.application.container.JDisc#handleRequest(Request)}.
 *
 * @author <a href="mailto:einarmr@yahoo-inc.com">Einar M R Rosenvinge</a>
 * @since 5.1.15
 * @see Response
 */
@Immutable
@Beta
public class Request {
    private final Headers headers = new Headers();
    private final String uri;
    private final byte[] body;
    private final Method method;

    /**
     * Creates a Request with an empty body.
     *
     * @param uri the URI of the request
     */
    public Request(String uri) {
        this(uri, new byte[0]);
    }

    /**
     * Creates a Request with a message body.
     *
     * @param uri the URI of the request
     * @param body the body of the request
     */
    public Request(String uri, byte[] body) {
        this(uri, body, Method.GET);
    }


    /**
     * Creates a Request with a message body.
     *
     * @param uri the URI of the request
     * @param body the body of the request
     */
    public Request(String uri, byte[] body, Method method) {
        this.uri = uri;
        this.body = body;
        this.method = method;
    }

    /**
     * Returns a mutable multi-map of headers for this Request.
     *
     * @return a mutable multi-map of headers for this Request
     */
    public Headers getHeaders() {
        return headers;
    }

    /**
     * Returns the body of this Request.
     *
     * @return the body of this Request
     */
    public byte[] getBody() {
        return body;
    }

    /**
     * Returns the URI of this Request.
     *
     * @return the URI of this Request
     */
    public String getUri() {
        return uri;
    }

    @Override
    public String toString() {
        String bodyStr = (body == null || body.length == 0) ? "[empty]" : "[omitted]";
        return "Request to " + uri + ", headers: " + headers + ", body: " + bodyStr;
    }

    public Method getMethod() {
        return method;
    }

    public enum Method {
        OPTIONS,
        GET,
        HEAD,
        POST,
        PUT,
        PATCH,
        DELETE,
        TRACE,
        CONNECT
    }
}
