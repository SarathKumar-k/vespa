// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.messagebus.shared;

import com.yahoo.jdisc.SharedResource;
import com.yahoo.messagebus.MessageHandler;
import com.yahoo.messagebus.Reply;

/**
 * @author <a href="mailto:simon@yahoo-inc.com">Simon Thoresen</a>
 */
public interface ServerSession extends SharedResource {

    public MessageHandler getMessageHandler();

    public void setMessageHandler(MessageHandler msgHandler);

    public void sendReply(Reply reply);

    public String connectionSpec();

    public String name();
}
