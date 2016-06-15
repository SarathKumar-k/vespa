// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.filedistribution.fileacquirer;

/**
 * @author tonytv
 */
public class FileReferenceRemovedException extends RuntimeException {
    public final String fileReference;

    FileReferenceRemovedException(String fileReference) {
        super("The file with file reference '" + fileReference + "' was removed while waiting.");
        this.fileReference = fileReference;
    }
}
