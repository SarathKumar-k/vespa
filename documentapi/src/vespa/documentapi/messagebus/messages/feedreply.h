// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/document/util/bytebuffer.h>
#include <vespa/documentapi/messagebus/messages/documentreply.h>
#include <vespa/documentapi/messagebus/messages/feedanswer.h>

namespace documentapi {

class FeedReply : public DocumentReply {
private:
    std::vector<FeedAnswer> _feedAnswers;

public:
    /**
     * Convenience typedef.
     */
    typedef std::unique_ptr<FeedReply> UP;
    typedef std::shared_ptr<FeedReply> SP;

    /**
     * Constructs a new reply for deserialization.
     *
     * @param type The type to assign to this.
     */
    FeedReply(uint32_t type);

    /**
     * Constructs a new feed reply.
     *
     * @param type        The type to assign to this.
     * @param feedAnswers The list of answers given by the search nodes.
     */
    FeedReply(uint32_t type, const std::vector<FeedAnswer> &feedAnswers);

    /**
     * Returns the list of answers given by the search nodes.
     *
     * @return The list of answers.
     */
    std::vector<FeedAnswer> &getFeedAnswers() { return _feedAnswers; }

    /**
     * Returns the list of answers given by the search nodes.
     *
     * @return The list of answers.
     */
    const std::vector<FeedAnswer> &getFeedAnswers() const { return _feedAnswers; }
};

}

