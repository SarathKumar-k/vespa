// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/metrics/metric.h>
#include <vespa/vespalib/util/jsonstream.h>

namespace metrics {

class JsonWriter : public MetricVisitor, public vespalib::JsonStreamTypes {
    vespalib::JsonStream& _stream;
    enum Flag { NOT_STARTED, SNAPSHOT_STARTED, METRICS_WRITTEN };
    Flag _flag;
    // Not a "true" set, but dimensions are guaranteed to be unique for any
    // given metric set.
    using DimensionSet = Metric::Tags;

    std::vector<DimensionSet> _dimensionStack;
    uint64_t _period;

public:
    JsonWriter(vespalib::JsonStream&);

private:
    virtual bool visitSnapshot(const MetricSnapshot&);
    virtual void doneVisitingSnapshot(const MetricSnapshot&);
    virtual bool visitMetricSet(const MetricSet&, bool autoGenerated);
    virtual void doneVisitingMetricSet(const MetricSet&);
    virtual bool visitCountMetric(const AbstractCountMetric&,
                                  bool autoGenerated);
    virtual bool visitValueMetric(const AbstractValueMetric&,
                                  bool autoGenerated);
    virtual void doneVisiting();

    void checkIfArrayNeedsToBeStarted();
    void writeCommonPrefix(const Metric& m);
    void writeCommonPostfix(const Metric& m);

    void pushLegacyDimensionFromSet(const MetricSet&);
    void pushAllDimensionsFromSet(const MetricSet&);
    void writeDimensions(const DimensionSet&);
    void writeInheritedDimensions();
    void writeMetricSpecificDimensions(const Metric&);
    
    bool isLeafMetric(const Metric& m) const { return !m.isMetricSet(); }
};

}

