// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.searchdefinition.derived;

import com.yahoo.document.*;
import com.yahoo.searchdefinition.Search;
import com.yahoo.searchdefinition.document.Attribute;
import com.yahoo.searchdefinition.document.Ranking;
import com.yahoo.searchdefinition.document.SDField;
import com.yahoo.searchdefinition.document.Sorting;
import com.yahoo.vespa.config.search.AttributesConfig;
import com.yahoo.config.subscription.ConfigInstanceUtil;
import com.yahoo.vespa.indexinglanguage.expressions.ToPositionExpression;

import java.util.*;

/**
 * The set of all attribute fields defined by a search definition
 *
 * @author <a href="mailto:bratseth@overture.com">Jon S Bratseth</a>
 */
public class AttributeFields extends Derived implements AttributesConfig.Producer {
    private Map<String, Attribute> attributes = new java.util.LinkedHashMap<>();

    /**
     * Flag indicating if a position-attribute has been found
     */
    private boolean hasPosition = false;

    public AttributeFields(Search search) {
        derive(search);
    }

    /**
     * Derives everything from a field
     */
    protected void derive(SDField field, Search search) {
        if (field.usesStructOrMap() &&
            !field.getDataType().equals(PositionDataType.INSTANCE) &&
            !field.getDataType().equals(DataType.getArray(PositionDataType.INSTANCE)))
        {
            return; // Ignore struct fields for indexed search (only implemented for streaming search)
        }
        deriveAttributes(field);
    }

    /**
     * Return an attribute by name, or null if it doesn't exist
     */
    public Attribute getAttribute(String attributeName) {
        return attributes.get(attributeName);
    }

    public boolean containsAttribute(String attributeName) {
        return getAttribute(attributeName) != null;
    }

    /**
     * Derives one attribute. TODO: Support non-default named attributes
     */
    private void deriveAttributes(SDField field) {
        for (Attribute fieldAttribute : field.getAttributes().values()) {
            Attribute attribute = getAttribute(fieldAttribute.getName());
            if (attribute == null) {
                attributes.put(fieldAttribute.getName(), fieldAttribute);
                attribute = getAttribute(fieldAttribute.getName());
            }
            Ranking ranking = field.getRanking();
            if (ranking != null && ranking.isFilter()) {
                attribute.setEnableBitVectors(true);
                attribute.setEnableOnlyBitVector(true);
            }
        }

        if (field.containsExpression(ToPositionExpression.class)) {
            // TODO: Move this check to processing and remove this
            if (hasPosition) {
                throw new IllegalArgumentException("Can not specify more than one " +
                                                   "set of position attributes per " + "field: " + field.getName());
            }
            hasPosition = true;
        }
    }

    /**
     * Returns a read only attribute iterator
     */
    public Iterator attributeIterator() {
        return attributes().iterator();
    }

    public Collection<Attribute> attributes() {
        return Collections.unmodifiableCollection(attributes.values());
    }

    public String toString() {
        return "attributes " + getName();
    }

    protected String getDerivedName() {
        return "attributes";
    }

    private Map<String, AttributesConfig.Attribute.Builder> toMap(List<AttributesConfig.Attribute.Builder> ls) {
        Map<String, AttributesConfig.Attribute.Builder> ret = new LinkedHashMap<>();
        for (AttributesConfig.Attribute.Builder builder : ls) {
            ret.put((String) ConfigInstanceUtil.getField(builder, "name"), builder);
        }
        return ret;
    }

    @Override
    public void getConfig(AttributesConfig.Builder builder) {
        for (Attribute attribute : attributes.values()) {
            AttributesConfig.Attribute.Builder aaB = new AttributesConfig.Attribute.Builder()
                .name(attribute.getName())
                .datatype(AttributesConfig.Attribute.Datatype.Enum.valueOf(attribute.getType().getExportAttributeTypeName()))
                .collectiontype(AttributesConfig.Attribute.Collectiontype.Enum.valueOf(attribute.getCollectionType().getName()));
            if (attribute.isRemoveIfZero()) {
                aaB.removeifzero(true);
            }
            if (attribute.isCreateIfNonExistent()) {
                aaB.createifnonexistent(true);
            }
            aaB.enablebitvectors(attribute.isEnabledBitVectors());
            aaB.enableonlybitvector(attribute.isEnabledOnlyBitVector());
            if (attribute.isFastSearch()) {
                aaB.fastsearch(true);
            }
            if (attribute.isFastAccess()) {
                aaB.fastaccess(true);
            }
            if (attribute.isHuge()) {
                aaB.huge(true);
            }
            if (attribute.getSorting().isDescending()) {
                aaB.sortascending(false);
            }
            if (attribute.getSorting().getFunction() != Sorting.Function.UCA) {
                aaB.sortfunction(AttributesConfig.Attribute.Sortfunction.Enum.valueOf(attribute.getSorting().getFunction().toString()));
            }
            if (attribute.getSorting().getStrength() != Sorting.Strength.PRIMARY) {
                aaB.sortstrength(AttributesConfig.Attribute.Sortstrength.Enum.valueOf(attribute.getSorting().getStrength().toString()));
            }
            if (!attribute.getSorting().getLocale().isEmpty()) {
                aaB.sortlocale(attribute.getSorting().getLocale());
            }
            aaB.arity(attribute.arity());
            aaB.lowerbound(attribute.lowerBound());
            aaB.upperbound(attribute.upperBound());
            aaB.densepostinglistthreshold(attribute.densePostingListThreshold());
            if (attribute.tensorType().isPresent()) {
                aaB.tensortype(attribute.tensorType().get().toString());
            }
            builder.attribute(aaB);
        }
    }
}
