// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.jdisc.http.server;

import com.yahoo.jdisc.application.BindingRepository;
import com.yahoo.jdisc.http.filter.RequestFilter;
import com.yahoo.jdisc.http.filter.ResponseFilter;

/**
 * @author <a href="mailto:bakksjo@yahoo-inc.com">Oyvind Bakksjo</a>
 */
public class FilterBindings {

    private final BindingRepository<RequestFilter> requestFilters;
    private final BindingRepository<ResponseFilter> responseFilters;

    public FilterBindings(BindingRepository<RequestFilter> requestFilters,
                          BindingRepository<ResponseFilter> responseFilters) {
        this.requestFilters = requestFilters;
        this.responseFilters = responseFilters;
    }

    public BindingRepository<RequestFilter> getRequestFilters() {
        return requestFilters;
    }

    public BindingRepository<ResponseFilter> getResponseFilters() {
        return responseFilters;
    }

}
