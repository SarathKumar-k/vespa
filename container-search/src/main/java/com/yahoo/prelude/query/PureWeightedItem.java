// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.prelude.query;

import java.nio.ByteBuffer;

/**
 * Created with IntelliJ IDEA.
 * User: balder
 * Date: 07.12.12
 * Time: 13:24
 * To change this template use File | Settings | File Templates.
 */
// TODO: Fix javadoc
public abstract class PureWeightedItem extends Item {

    public PureWeightedItem(int weight) {
        setWeight(weight);
    }
    @Override
    public void setIndexName(String index) {
        // No index
    }

    @Override
    public String getName() {
        return getItemType().name();  //To change body of implemented methods use File | Settings | File Templates.
    }

    @Override
    public int encode(ByteBuffer buffer) {
        encodeThis(buffer);
        return 1;
    }

    @Override
    protected void appendBodyString(StringBuilder buffer) {
        buffer.append(':').append(getWeight());
    }
}
