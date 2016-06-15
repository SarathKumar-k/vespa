// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.prelude.query;

import com.yahoo.compress.IntegerCompressor;
import com.yahoo.prelude.query.textualrepresentation.Discloser;

import java.nio.ByteBuffer;


/**
 * <p>A set of terms which must be near each other to match.</p>
 *
 * @author bratseth
 * @author havardpe
 */
public class NearItem extends CompositeItem {

    protected int distance;

    /** The default distance used if none is specified: 2 */
    public static final int defaultDistance=2;

    /** Creates a NEAR item with distance 2 */
    public NearItem() {
        setDistance(defaultDistance);
    }

    /**
     * Creates a <i>near</i> item with a limit to the distance
     * between the words.
     *
     * @param distance the number of word position which may separate
     *        the words for this near item to match
     */
    public NearItem(int distance) {
        setDistance(distance);
    }

    public void setDistance(int distance) {
        if (distance < 0) {
            throw new IllegalArgumentException("Can not use negative distance '" + distance + "'.");
        }
        this.distance = distance;
    }

    public int getDistance() {
        return distance;
    }

    public ItemType getItemType() {
        return ItemType.NEAR;
    }

    public String getName() {
        return "NEAR";
    }

    protected void encodeThis(ByteBuffer buffer) {
        super.encodeThis(buffer);
        IntegerCompressor.putCompressedPositiveNumber(distance, buffer);
    }

    @Override
    public void disclose(Discloser discloser) {
        super.disclose(discloser);
        discloser.addProperty("limit", distance);
    }

    /** Appends the heading of this string - <code>[getName()]([limit]) </code> */
    protected void appendHeadingString(StringBuilder buffer) {
        buffer.append(getName());
        buffer.append("(");
        buffer.append(distance);
        buffer.append(")");
        buffer.append(" ");
    }

    public int hashCode() {
        return super.hashCode() + 23* distance;
    }

    /**
     * Returns whether this item is of the same class and
     * contains the same state as the given item
     */
    public boolean equals(Object object) {
        if (!super.equals(object)) return false;
        NearItem other = (NearItem) object; // Ensured by superclass
        if (this.distance !=other.distance) return false;
        return true;
    }

}
