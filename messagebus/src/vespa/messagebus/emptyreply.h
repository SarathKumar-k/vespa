// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "reply.h"

namespace mbus {

/**
 * A concrete reply that contains no protocol-specific data. This is needed to
 * enable messagebus to reply to messages that result in an error. It may also
 * be used by the application for ack type replies. Objects of this class will
 * identify as type 0, which is reserved for this use. Also note that whenever a
 * protocol-specific reply encodes to an empty blob it will be decoded to an
 * EmptyReply at its network peer.
 */
class EmptyReply : public Reply {
public:
    /**
     * Constructs a new instance of this class.
     */
    EmptyReply();

    /**
     * This method returns the empty string to signal that it does not belong to
     * a protocol.
     *
     * @return ""
     */
    virtual const string & getProtocol() const;

    /**
     * This method returns the message type id reserved for empty replies: 0
     *
     * @return 0
     */
    virtual uint32_t getType() const;

    /**
     * Encodes this reply into an empty blob.
     *
     * @return empty blob
     */
    virtual Blob encode() const;

    uint8_t priority() const { return 8; }
};

} // namespace mbus

