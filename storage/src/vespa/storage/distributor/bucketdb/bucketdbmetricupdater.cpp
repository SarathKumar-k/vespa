// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/storage/distributor/bucketdb/bucketdbmetricupdater.h>

#include <vespa/storage/distributor/distributormetricsset.h>
#include <vespa/storage/distributor/idealstatemetricsset.h>

#include <algorithm>

namespace storage {
namespace distributor {

BucketDBMetricUpdater::BucketDBMetricUpdater()
    : _workingStats(),
      _lastCompleteStats(),
      _replicaCountingMode(ReplicaCountingMode::TRUSTED),
      _hasCompleteStats(false)
{
}

void
BucketDBMetricUpdater::resetStats()
{
    _workingStats = Stats();
}

void
BucketDBMetricUpdater::visit(const BucketDatabase::Entry& entry,
                             uint32_t redundancy)
{
    if (entry->getNodeCount() == 0) {
        // We used to have an assert on >0 but that caused some crashes, see
        // ticket 7275624. Why? Until that gets sorted out, we're disabling the
        // assert and return, which should be fine since it was the old
        // behavior.
        return;
    }

    ++_workingStats._totalBuckets;

    uint32_t docCount = 0;
    uint32_t byteCount = 0;
    uint32_t trustedCopies = 0;
        
    for (uint32_t i = 0; i < entry->getNodeCount(); i++) {
        if (entry->getNodeRef(i).trusted()) {
            if (trustedCopies == 0) {
                docCount = entry->getNodeRef(i).getDocumentCount();
                byteCount = entry->getNodeRef(i).getTotalDocumentSize();
            }
                
            trustedCopies++;
        }
    }
        
    // If there was no trusted, pick the largest one.
    if (trustedCopies == 0) {
        for (uint32_t i = 0; i < entry->getNodeCount(); i++) {
            uint32_t curr = entry->getNodeRef(i).getDocumentCount();
                
            if (curr > docCount) {
                docCount = curr;
                byteCount = entry->getNodeRef(i).getTotalDocumentSize();
            }
        }
    }
                
    _workingStats._docCount += docCount;
    _workingStats._byteCount += byteCount;

    if (trustedCopies < redundancy) {
        ++_workingStats._tooFewCopies;
    } else if (trustedCopies > redundancy) {
        ++_workingStats._tooManyCopies;
    }
    if (trustedCopies == 0) {
        ++_workingStats._noTrusted;
    }
    updateMinReplicationStats(entry, trustedCopies);
}

void
BucketDBMetricUpdater::updateMinReplicationStats(
        const BucketDatabase::Entry& entry,
        uint32_t trustedCopies)
{
    auto& minBucketReplica = _workingStats._minBucketReplica;
    for (uint32_t i = 0; i < entry->getNodeCount(); i++) {
        const uint16_t node = entry->getNodeRef(i).getNode();
        // Note: currently we assume there are only 2 counting modes.
        // Either we only count the trusted replicas, or we count any and all
        // available replicas without caring about whether or not they are in
        // sync across each other.
        // Regardless of counting mode we still have to take the minimum
        // replica count across all buckets present on any given node.
        const uint32_t countedReplicas(
                (_replicaCountingMode == ReplicaCountingMode::TRUSTED)
                 ? trustedCopies : entry->getNodeCount());
        auto it = minBucketReplica.find(node);
        if (it == minBucketReplica.end()) {
            minBucketReplica[node] = countedReplicas;
        } else {
            it->second = std::min(it->second, countedReplicas);
        }
    }
}

void
BucketDBMetricUpdater::completeRound(bool resetWorkingStats)
{
    _lastCompleteStats = _workingStats;
    _hasCompleteStats = true;
    if (resetWorkingStats) {
        resetStats();
    }
}

void
BucketDBMetricUpdater::Stats::propagateMetrics(
        IdealStateMetricSet& idealStateMetrics,
        DistributorMetricSet& distributorMetrics)
{
    distributorMetrics.docsStored.set(_docCount);
    distributorMetrics.bytesStored.set(_byteCount);

    idealStateMetrics.buckets_toofewcopies.set(_tooFewCopies);
    idealStateMetrics.buckets_toomanycopies.set(_tooManyCopies);
    idealStateMetrics.buckets_notrusted.set(_noTrusted);
    idealStateMetrics.buckets.set(_totalBuckets);
}

void
BucketDBMetricUpdater::reset()
{
    resetStats();
}

} // distributor
} // storage
