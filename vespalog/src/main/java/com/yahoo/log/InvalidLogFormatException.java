// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.log;

/**
 * This (checked) exception is used to flag invalid log messages,
 * primarily for use in the factory methods of LogMessage.
 *
 * @author  <a href="mailto:borud@yahoo-inc.com">Bjorn Borud</a>
 */
public class InvalidLogFormatException extends Exception
{
    public InvalidLogFormatException (String msg) {
        super(msg);
    }

    public InvalidLogFormatException () {
    }
}
