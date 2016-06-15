// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.prelude.fastsearch;

import com.yahoo.search.result.StructuredData;
import com.yahoo.data.access.Inspector;
import com.yahoo.data.access.Type;
import com.yahoo.container.search.LegacyEmulationConfig;
import com.yahoo.prelude.hitfield.JSONString;

/**
 * Class representing a XML rendered structured data field in the result set
 */
public class StructDataField extends JSONField {

    public StructDataField(String name) {
        super(name);
    }

    @Override
    public String toString() {
        return "field " + getName() + " type StructDataField";
    }

    public Object convert(Inspector value) {
        if (getEmulConfig().stringBackedStructuredData() ||
            value.type() == Type.STRING)
        {
            return super.convert(value);
        }
        return new StructuredData(value);
    }

}
