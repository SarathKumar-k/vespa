// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "searchable_doc_subdb_configurer.h"
#include "fast_access_feed_view.h"
#include "iattributeadapterfactory.h"
#include <vespa/searchcore/proton/reprocessing/i_reprocessing_initializer.h>

namespace proton {

/**
 * Class used to reconfig the feed view used in a fast-access sub database
 * when the set of fast-access attributes change.
 */
class FastAccessDocSubDBConfigurer
{
public:
    typedef vespalib::VarHolder<FastAccessFeedView::SP> FeedViewVarHolder;

private:
    FeedViewVarHolder           &_feedView;
    IAttributeAdapterFactory::UP _factory;
    vespalib::string             _subDbName;

    void reconfigureFeedView(const FastAccessFeedView::SP &curr,
                             const search::index::Schema::SP &schema,
                             const document::DocumentTypeRepo::SP &repo,
                             const IAttributeWriter::SP &attrWriter);

public:
    FastAccessDocSubDBConfigurer(FeedViewVarHolder &feedView,
                                 IAttributeAdapterFactory::UP factory,
                                 const vespalib::string &subDbName);

    IReprocessingInitializer::UP reconfigure(const DocumentDBConfig &newConfig,
                                             const DocumentDBConfig &oldConfig,
                                             const AttributeCollectionSpec &attrSpec);
};

} // namespace proton

