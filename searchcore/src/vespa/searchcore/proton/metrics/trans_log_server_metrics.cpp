// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include "trans_log_server_metrics.h"

using search::transactionlog::DomainInfo;
using search::transactionlog::DomainStats;

namespace proton {

TransLogServerMetrics::DomainMetrics::DomainMetrics(metrics::MetricSet *parent,
                                                    const vespalib::string &documentType)
    : metrics::MetricSet("transactionlog", {{"documenttype", documentType}},
            "Transaction log metrics for a document type", parent),
      entries("entries", "", "The current number of entries in the transaction log", this)
{
}

void
TransLogServerMetrics::DomainMetrics::update(const DomainInfo &stats)
{
    entries.set(stats.count);
}

void
TransLogServerMetrics::considerAddDomains(const DomainStats &stats)
{
    for (const auto &elem : stats) {
        const vespalib::string &documentType = elem.first;
        if (_domainMetrics.find(documentType) == _domainMetrics.end()) {
            _domainMetrics[documentType] = DomainMetrics::UP(new DomainMetrics(_parent, documentType));
        }
    }
}

void
TransLogServerMetrics::considerRemoveDomains(const DomainStats &stats)
{
    for (auto itr = _domainMetrics.begin(); itr != _domainMetrics.end(); ) {
        const vespalib::string &documentType = itr->first;
        if (stats.find(documentType) == stats.end()) {
            itr = _domainMetrics.erase(itr);
        } else {
            ++itr;
        }
    }
}

void
TransLogServerMetrics::updateDomainMetrics(const DomainStats &stats)
{
    for (const auto &elem : stats) {
        const vespalib::string &documentType = elem.first;
        _domainMetrics.find(documentType)->second->update(elem.second);
    }
}

TransLogServerMetrics::TransLogServerMetrics(metrics::MetricSet *parent)
    : _parent(parent)
{
}

void
TransLogServerMetrics::update(const DomainStats &stats)
{
    considerAddDomains(stats);
    considerRemoveDomains(stats);
    updateDomainMetrics(stats);
}

} // namespace proton
