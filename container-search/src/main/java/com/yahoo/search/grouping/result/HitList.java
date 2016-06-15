// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.search.grouping.result;

import com.yahoo.search.Result;
import com.yahoo.search.grouping.GroupingRequest;
import com.yahoo.search.result.Hit;

/**
 * <p>This class represents a labeled hit list in the grouping result model. It is contained in {@link Group}, and
 * contains one or more {@link Hit hits} itself, making this the parent of leaf nodes in the hierarchy of grouping
 * results. Use the {@link GroupingRequest#getResultGroup(Result)} to retrieve grouping results.</p>
 *
 * @author <a href="mailto:simon@yahoo-inc.com">Simon Thoresen</a>
 */
public class HitList extends AbstractList {

    /**
     * <p>Constructs a new instance of this class.</p>
     *
     * @param label The label to assign to this.
     */
    public HitList(String label) {
        super("hitlist", label);
    }
}
