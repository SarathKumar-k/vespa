// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".proton.server.metricsengine");
#include "metrics_engine.h"
#include <vespa/metrics/jsonwriter.h>

namespace proton {

MetricsEngine::MetricsEngine()
    : _root(),
      _legacyRoot(),
      _manager(),
      _metrics_producer(_manager)
{
}

MetricsEngine::~MetricsEngine()
{
}

void
MetricsEngine::start(const config::ConfigUri &)
{
    {
        metrics::MetricLockGuard guard(_manager.getMetricLock());
        _manager.registerMetric(guard, _root);
        _manager.registerMetric(guard, _legacyRoot);
    }

    // Storage doesnt snapshot unset metrics to save memory. Currently
    // feature seems a bit bugged. Disabling this optimalization for search.
    // Can enable it later when it is confirmed to be working well.
    _manager.snapshotUnsetMetrics(true);

    // Currently, when injecting a metric manager into the content layer,
    // the content layer require to be the one initializing and starting it.
    // Thus not calling init here, but further out in the application when
    // one knows whether we are running in row/column mode or not
}

void
MetricsEngine::addMetricsHook(metrics::MetricManager::UpdateHook &hook)
{
    _manager.addMetricUpdateHook(hook, 5);
}

void
MetricsEngine::removeMetricsHook(metrics::MetricManager::UpdateHook &hook)
{
    _manager.removeMetricUpdateHook(hook);
}

void
MetricsEngine::addExternalMetrics(metrics::Metric &child)
{
    metrics::MetricLockGuard guard(_manager.getMetricLock());
    _legacyRoot.registerMetric(child);
}

void
MetricsEngine::removeExternalMetrics(metrics::Metric &child)
{
    metrics::MetricLockGuard guard(_manager.getMetricLock());
    _legacyRoot.unregisterMetric(child);
}

namespace {

void
addLegacyDocumentDBMetrics(LegacyProtonMetrics &legacyRoot,
                           LegacyDocumentDBMetrics &metrics)
{
    legacyRoot.docTypes.registerMetric(metrics);
    // cannot use sum of sum due to metric clone issues
    legacyRoot.memoryUsage.addMetricToSum(metrics.index.memoryUsage);
    legacyRoot.memoryUsage.addMetricToSum(metrics.attributes.memoryUsage);
    legacyRoot.memoryUsage.addMetricToSum(metrics.docstore.memoryUsage);
    legacyRoot.diskUsage.addMetricToSum(metrics.index.diskUsage);
    legacyRoot.docsInMemory.addMetricToSum(metrics.index.docsInMemory);
    legacyRoot.numDocs.addMetricToSum(metrics.numDocs);
    legacyRoot.numActiveDocs.addMetricToSum(metrics.numActiveDocs);
    legacyRoot.numIndexedDocs.addMetricToSum(metrics.numIndexedDocs);
    legacyRoot.numStoredDocs.addMetricToSum(metrics.numStoredDocs);
    legacyRoot.numRemovedDocs.addMetricToSum(metrics.numRemovedDocs);
}

void
removeLegacyDocumentDBMetrics(LegacyProtonMetrics &legacyRoot,
                              LegacyDocumentDBMetrics &metrics)
{
    legacyRoot.docTypes.unregisterMetric(metrics);
    // cannot use sum of sum due to metric clone issues
    legacyRoot.memoryUsage.removeMetricFromSum(metrics.index.memoryUsage);
    legacyRoot.memoryUsage.removeMetricFromSum(metrics.attributes.memoryUsage);
    legacyRoot.memoryUsage.removeMetricFromSum(metrics.docstore.memoryUsage);
    legacyRoot.diskUsage.removeMetricFromSum(metrics.index.diskUsage);
    legacyRoot.docsInMemory.removeMetricFromSum(metrics.index.docsInMemory);
    legacyRoot.numDocs.removeMetricFromSum(metrics.numDocs);
    legacyRoot.numActiveDocs.removeMetricFromSum(metrics.numActiveDocs);
    legacyRoot.numIndexedDocs.removeMetricFromSum(metrics.numIndexedDocs);
    legacyRoot.numStoredDocs.removeMetricFromSum(metrics.numStoredDocs);
    legacyRoot.numRemovedDocs.removeMetricFromSum(metrics.numRemovedDocs);
}

}

void
MetricsEngine::addDocumentDBMetrics(DocumentDBMetricsCollection &child)
{
    metrics::MetricLockGuard guard(_manager.getMetricLock());
    addLegacyDocumentDBMetrics(_legacyRoot, child.getMetrics());

    _root.registerMetric(child.getTaggedMetrics());
}

void
MetricsEngine::removeDocumentDBMetrics(DocumentDBMetricsCollection &child)
{
    metrics::MetricLockGuard guard(_manager.getMetricLock());
    removeLegacyDocumentDBMetrics(_legacyRoot, child.getMetrics());

    _root.unregisterMetric(child.getTaggedMetrics());
}

namespace {

void
doAddAttribute(AttributeMetrics &attributes,
               const std::string &name)
{
    AttributeMetrics::List::Entry::LP entry = attributes.list.add(name);
    if (entry.get() != 0) {
        LOG(debug, "doAddAttribute(): name='%s', attributes=%p",
                name.c_str(), (void*)&attributes);
        attributes.list.registerMetric(*entry);
    } else {
        LOG(warning, "multiple attributes have the same name: '%s'", name.c_str());
    }
}

void
doRemoveAttribute(AttributeMetrics &attributes,
                  const std::string &name)
{
    AttributeMetrics::List::Entry::LP entry = attributes.list.remove(name);
    if (entry.get() != 0) {
        LOG(debug, "doRemoveAttribute(): name='%s', attributes=%p",
                name.c_str(), (void*)&attributes);
        attributes.list.unregisterMetric(*entry);
    } else {
        LOG(debug, "Could not remove attribute with name '%s', not found", name.c_str());
    }
}

void
doCleanAttributes(AttributeMetrics &attributes)
{
    std::vector<AttributeMetrics::List::Entry::LP> entries = attributes.list.release();
    for (size_t i = 0; i < entries.size(); ++i) {
        attributes.list.unregisterMetric(*entries[i]);
    }
}

}

void
MetricsEngine::addAttribute(AttributeMetrics &subAttributes,
                            AttributeMetrics *totalAttributes,
                            const std::string &name)
{
    metrics::MetricLockGuard guard(_manager.getMetricLock());
    doAddAttribute(subAttributes, name);
    if (totalAttributes != NULL) {
        doAddAttribute(*totalAttributes, name);
    }
}

void
MetricsEngine::removeAttribute(AttributeMetrics &subAttributes,
                               AttributeMetrics *totalAttributes,
                               const std::string &name)
{
    metrics::MetricLockGuard guard(_manager.getMetricLock());
    doRemoveAttribute(subAttributes, name);
    if (totalAttributes != NULL) {
        doRemoveAttribute(*totalAttributes, name);
    }
}

void
MetricsEngine::cleanAttributes(AttributeMetrics &subAttributes,
                               AttributeMetrics *totalAttributes)
{
    metrics::MetricLockGuard guard(_manager.getMetricLock());
    doCleanAttributes(subAttributes);
    if (totalAttributes != NULL) {
        doCleanAttributes(*totalAttributes);
    }
}

void MetricsEngine::addRankProfile(LegacyDocumentDBMetrics &owner,
                                   const std::string &name,
                                   size_t numDocIdPartitions) {
    metrics::MetricLockGuard guard(_manager.getMetricLock());
    LegacyDocumentDBMetrics::MatchingMetrics::RankProfileMetrics::LP &entry =
        owner.matching.rank_profiles[name];
    if (entry.get()) {
        LOG(warning, "Two rank profiles have the same name: %s", name.c_str());
    } else {
        owner.matching.rank_profiles[name].reset(
                new LegacyDocumentDBMetrics::MatchingMetrics::RankProfileMetrics(
                        name, std::min(numDocIdPartitions, owner._maxNumThreads), &owner.matching));
    }
}

void MetricsEngine::cleanRankProfiles(LegacyDocumentDBMetrics &owner) {
    metrics::MetricLockGuard guard(_manager.getMetricLock());
    LegacyDocumentDBMetrics::MatchingMetrics::RankProfileMap metrics;
    owner.matching.rank_profiles.swap(metrics);
    for (LegacyDocumentDBMetrics::MatchingMetrics::RankProfileMap::const_iterator
             it = metrics.begin(); it != metrics.end(); ++it) {
        owner.matching.unregisterMetric(*it->second);
    }
}

void
MetricsEngine::stop()
{
    _manager.stop();
}

} // namespace proton
