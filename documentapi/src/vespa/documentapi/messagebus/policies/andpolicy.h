// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vector>
#include <vespa/messagebus/routing/hop.h>
#include <vespa/messagebus/routing/iroutingpolicy.h>
#include <vespa/documentapi/common.h>

namespace documentapi {

/**
 * An AND policy is a routing policy that can be used to write simple routes that split a message between multiple other
 * destinations. It can either be configured in a routing config, which will then produce a policy that always selects
 * all configured recipients, or it can be configured using the policy parameter (i.e. a string following the name of
 * the policy). Note that configured recipients take precedence over recipients configured in the parameter string.
 *
 * @author <a href="mailto:simon@yahoo-inc.com">Simon Thoresen</a>
 * @version $Id$
 */
class ANDPolicy : public mbus::IRoutingPolicy {
public:
    /**
     * Constructs a new AND policy that requires all recipients to be ok for it to merge their replies to an ok reply.
     * I.e. all errors in all child replies are copied into the merged reply.
     *
     * @param param A string of recipients to select unless recipients have been configured.
     */
    ANDPolicy(const string& param);

    /**
     * Destructor.
     *
     * Frees all allocated resources.
     */
    virtual ~ANDPolicy();

    // Inherit doc from IRoutingPolicy.
    virtual void select(mbus::RoutingContext &context);

    // Inherit doc from IRoutingPolicy.
    virtual void merge(mbus::RoutingContext &context);

private:
    ANDPolicy(const ANDPolicy &);            // hide
    ANDPolicy &operator=(const ANDPolicy &); // hide

private:
    std::vector<mbus::Hop> _hops;
};

}

