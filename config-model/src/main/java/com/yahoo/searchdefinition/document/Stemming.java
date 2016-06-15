// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.searchdefinition.document;

import com.yahoo.language.process.StemMode;

import java.util.logging.Logger;

/**
 * <p>The stemming setting of a field. This describes how the search engine
 * should transform content of this field into base forms (stems) to increase
 * recall (find "car" when you search for "cars" etc.).</p>
 *
 * @author bratseth
 */
public enum Stemming {

     /** No stemming */
    NONE("none"),

    /** Stem as much as possible */
    ALL("all"),

    /** select shortest possible stem */
    SHORTEST("shortest"),

    /** index (and query?) multiple stems */
    MULTIPLE("multiple");

    private static Logger log=Logger.getLogger(Stemming.class.getName());

    private final String name;

    /**
     * Returns the stemming object for the given string.
     * The legal stemming names are the stemming constants in any capitalization.
     *
     * @throws IllegalArgumentException if there is no stemming type with the given name
     */
    public static Stemming get(String stemmingName) {
        try {
            Stemming stemming = Stemming.valueOf(stemmingName.toUpperCase());
            if (stemming.equals(ALL)) {
                log.warning("note: stemming ALL is the same as stemming mode SHORTEST");
                stemming = SHORTEST;
            }
            return stemming;
        } catch (IllegalArgumentException e) {
            throw new IllegalArgumentException("'" + stemmingName + "' is not a valid stemming setting");
        }
    }

    private Stemming(String name) {
        this.name = name;
    }

    public String getName() { return name; }

    public String toString() {
        return "stemming " + name;
    }

    public StemMode toStemMode() {
        if (this == Stemming.SHORTEST) {
            return StemMode.SHORTEST;
        }
        if (this == Stemming.MULTIPLE) {
            return StemMode.ALL;
        }
        return StemMode.NONE;
    }

}
