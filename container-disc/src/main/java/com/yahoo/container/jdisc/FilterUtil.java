// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.container.jdisc;

import com.yahoo.component.ComponentId;
import com.yahoo.component.ComponentSpecification;
import com.yahoo.component.provider.ComponentRegistry;
import com.yahoo.container.http.filter.FilterChainRepository;
import com.yahoo.jdisc.application.BindingRepository;
import com.yahoo.jdisc.application.UriPattern;
import com.yahoo.jdisc.http.filter.RequestFilter;
import com.yahoo.jdisc.http.filter.ResponseFilter;
import com.yahoo.jdisc.http.filter.SecurityRequestFilter;
import com.yahoo.jdisc.http.filter.SecurityRequestFilterChain;

import java.util.List;

/**
 * Helper class to set up filter binding repositories based on config.
 *
 * @author <a href="mailto:bakksjo@yahoo-inc.com">Oyvind Bakksjo</a>
 */
class FilterUtil {
    private static final ComponentId SEARCH_SERVER_COMPONENT_ID = ComponentId.fromString("SearchServer");

    private final BindingRepository<RequestFilter> requestFilters;
    private final BindingRepository<ResponseFilter> responseFilters;

    private FilterUtil(
            final BindingRepository<RequestFilter> requestFilters,
            final BindingRepository<ResponseFilter> responseFilters) {
        this.requestFilters = requestFilters;
        this.responseFilters = responseFilters;
    }

    private void configureFilters(
            final List<FilterSpec> filtersConfig,
            final FilterChainRepository filterChainRepository) {
        for (final FilterSpec filterConfig : filtersConfig) {
            final Object filter = filterChainRepository.getFilter(ComponentSpecification.fromString(filterConfig.getId()));
            if (filter == null) {
                throw new RuntimeException("No http filter with id " + filterConfig.getId());
            }
            addFilter(filter, filterConfig.getBinding());
        }
    }

    private void addFilter(final Object filter, final String binding) {
        if (filter instanceof RequestFilter && filter instanceof ResponseFilter) {
            throw new RuntimeException("The filter " + filter.getClass().getName() + " is unsupported since it's both a RequestFilter and a ResponseFilter.");
        } else if (filter instanceof RequestFilter) {
            requestFilters.put(new UriPattern(binding), (RequestFilter) filter);
        } else if (filter instanceof ResponseFilter) {
            responseFilters.put(new UriPattern(binding), (ResponseFilter) filter);
        } else {
            throw new RuntimeException("Unknown filter type " + filter.getClass().getName());
        }
    }

    //TVT: remove
    private void configureLegacyFilters(
            final ComponentId id,
            final ComponentRegistry<SecurityRequestFilter> legacyRequestFilters) {
        final ComponentId serverName = id.getNamespace();
        if (SEARCH_SERVER_COMPONENT_ID.equals(serverName) && !legacyRequestFilters.allComponents().isEmpty()) {
            requestFilters.bind(
                    "http://*/*",
                    SecurityRequestFilterChain.newInstance(legacyRequestFilters.allComponents()));
        }
    }

    /**
     * Populates binding repositories with filters based on config.
     *
     * @param requestFilters output argument that will be mutated
     * @param responseFilters output argument that will be mutated
     */
    public static void setupFilters(
            final ComponentId componentId,
            final ComponentRegistry<SecurityRequestFilter> legacyRequestFilters,
            final List<FilterSpec> filtersConfig,
            final FilterChainRepository filterChainRepository,
            final BindingRepository<RequestFilter> requestFilters,
            final BindingRepository<ResponseFilter> responseFilters) {
        final FilterUtil filterUtil = new FilterUtil(requestFilters, responseFilters);

        // TODO: remove
        filterUtil.configureLegacyFilters(componentId, legacyRequestFilters);

        filterUtil.configureFilters(filtersConfig, filterChainRepository);
    }

    public static class FilterSpec {
        private final String id;
        private final String binding;

        public FilterSpec(String id, String binding) {
            this.id = id;
            this.binding = binding;
        }

        public String getId() {
            return id;
        }

        public String getBinding() {
            return binding;
        }
    }
}
