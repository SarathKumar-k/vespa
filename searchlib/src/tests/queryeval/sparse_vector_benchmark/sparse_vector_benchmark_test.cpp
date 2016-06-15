// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/vespalib/testkit/test_kit.h>

#include "../weak_and/rise_wand.h"
#include "../weak_and/rise_wand.hpp"
#include <vespa/searchlib/fef/matchdatalayout.h>
#include <vespa/searchlib/fef/termfieldmatchdata.h>
#include <vespa/searchlib/fef/termfieldmatchdataarray.h>
#include <vespa/searchlib/queryeval/andnotsearch.h>
#include <vespa/searchlib/queryeval/andsearch.h>
#include <vespa/searchlib/queryeval/dot_product_search.h>
#include <vespa/searchlib/queryeval/fake_search.h>
#include <vespa/searchlib/util/rand48.h>
#include <vespa/searchlib/queryeval/orsearch.h>
#include <vespa/searchlib/queryeval/simpleresult.h>
#include <vespa/searchlib/queryeval/wand/weak_and_search.h>
#include <vespa/searchlib/queryeval/weighted_set_term_search.h>
#include <vespa/vespalib/util/box.h>
#include <vespa/vespalib/util/stringfmt.h>

using namespace search::fef;
using namespace search::queryeval;
using namespace vespalib;

namespace {

//-----------------------------------------------------------------------------

struct Writer {
    FILE *file;
    Writer(const std::string &file_name) {
        file = fopen(file_name.c_str(), "w");
        assert(file != 0);
    }
    void write(const char *data, size_t size) const {
        fwrite(data, 1, size, file);
    }
    void fmt(const char *format, ...) const
#ifdef __GNUC__
        __attribute__ ((format (printf,2,3)))
#endif
    {
        va_list ap;
        va_start(ap, format);
        vfprintf(file, format, ap);
        va_end(ap);
    }
    ~Writer() { fclose(file); }
};

//-----------------------------------------------------------------------------

// top-level html report (global, used by plots and graphs directly)
class Report
{
private:
    Writer _html;

public:
    Report(const std::string &file) : _html(file) {
        _html.fmt("<html>\n");
        _html.fmt("<head><title>Sparse Vector Search Benchmark Report</title></head>\n");
        _html.fmt("<body>\n");
        _html.fmt("<h1>Sparse Vector Search Benchmark Report</h1>\n");
    }
    void addPlot(const std::string &title, const std::string &png_file) {
        _html.fmt("<h3>%s</h3>\n", title.c_str());
        _html.fmt("<img src=\"%s\">\n", png_file.c_str());
    }
    ~Report() {
        _html.fmt("<h2>Test Log with Numbers</h2>\n");
        _html.fmt("<pre>\n");
        // html file needs external termination
    }
};

Report report("report.head");

//-----------------------------------------------------------------------------

// a single graph within a plot
class Graph
{
private:
    Writer _writer;

public:
    typedef std::unique_ptr<Graph> UP;
    Graph(const std::string &file) : _writer(file) {}
    void addValue(double x, double y) { _writer.fmt("%g %g\n", x, y); }
};

// a plot possibly containing multiple graphs
class Plot
{
private:
    std::string _name;
    int         _graphs;
    Writer      _writer;
    static int  _plots;

public:
    typedef std::unique_ptr<Plot> UP;

    Plot(const std::string &title) : _name(vespalib::make_string("plot.%d", _plots++)), _graphs(0),
                                     _writer(vespalib::make_string("%s.gnuplot", _name.c_str())) {
        std::string png_file = vespalib::make_string("%s.png", _name.c_str());
        _writer.fmt("set term png size 1200,800\n");
        _writer.fmt("set output '%s'\n", png_file.c_str());
        _writer.fmt("set title '%s'\n", title.c_str());
        _writer.fmt("set xlabel 'term count'\n");
        _writer.fmt("set ylabel 'time (ms)'\n");
        report.addPlot(title, png_file);
    }

    ~Plot() {
        _writer.fmt("\n");
    }

    Graph::UP createGraph(const std::string &legend) {
        std::string file = vespalib::make_string("%s.graph.%d", _name.c_str(), _graphs);
        _writer.fmt("%s '%s' using 1:2 title '%s' w lines",
                    (_graphs == 0) ? "plot " : ",", file.c_str(), legend.c_str());
        ++_graphs;
        return Graph::UP(new Graph(file));
    }

    static UP createPlot(const std::string &title) { return UP(new Plot(title)); }
};

int Plot::_plots = 0;

//-----------------------------------------------------------------------------

uint32_t default_weight = 100;
double max_time = 1000000.0;

//-----------------------------------------------------------------------------

struct ChildFactory {
    ChildFactory() {}
    virtual std::string name() const = 0;
    virtual SearchIterator *createChild(uint32_t idx, uint32_t limit) const = 0;
    virtual ~ChildFactory() {}
};

struct SparseVectorFactory {
    virtual std::string name() const = 0;
    virtual SearchIterator *createSparseVector(ChildFactory &childFactory, uint32_t childCnt, uint32_t limit) const = 0;
    virtual ~SparseVectorFactory() {}
};

struct FilterStrategy {
    virtual std::string name() const = 0;
    virtual SearchIterator *createRoot(SparseVectorFactory &vectorFactory, ChildFactory &childFactory, uint32_t childCnt, uint32_t limit) const = 0;
    virtual ~FilterStrategy() {}
};

//-----------------------------------------------------------------------------

struct ModSearch : SearchIterator {
    uint32_t step;
    uint32_t limit;
    ModSearch(uint32_t step_in, uint32_t limit_in) : step(step_in), limit(limit_in) { setDocId(step); }
    virtual void doSeek(uint32_t docid) {
        assert(docid > getDocId());
        uint32_t hit = (docid / step) * step;
        if (hit < docid) {
            hit += step;
        }
        if (hit < limit) {
            assert(hit >= docid);
            setDocId(hit);
        } else {
            setAtEnd();
        }
    }
    virtual void doUnpack(uint32_t) {}
};

struct ModSearchFactory : ChildFactory {
    uint32_t bias;
    ModSearchFactory() : bias(1) {}
    explicit ModSearchFactory(int b) : bias(b) {}
    virtual std::string name() const {
        return vespalib::make_string("ModSearch(%u)", bias);
    }
    virtual SearchIterator *createChild(uint32_t idx, uint32_t limit) const {
        return new ModSearch(bias + idx, limit);
    }
};

//-----------------------------------------------------------------------------

struct VespaWandFactory : SparseVectorFactory {
    uint32_t n;
    VespaWandFactory(uint32_t n_in) : n(n_in) {}
    virtual std::string name() const {
        return vespalib::make_string("VespaWand(%u)", n);
    }
    virtual SearchIterator *createSparseVector(ChildFactory &childFactory, uint32_t childCnt, uint32_t limit) const {
        wand::Terms terms;
        for (size_t i = 0; i < childCnt; ++i) {
            terms.push_back(wand::Term(childFactory.createChild(i, limit), default_weight, limit / (i + 1)));
        }
        return WeakAndSearch::create(terms, n, true);
    }
};

struct RiseWandFactory : SparseVectorFactory {
    uint32_t n;
    RiseWandFactory(uint32_t n_in) : n(n_in) {}
    virtual std::string name() const {
        return vespalib::make_string("RiseWand(%u)", n);
    }
    virtual SearchIterator *createSparseVector(ChildFactory &childFactory, uint32_t childCnt, uint32_t limit) const {
        wand::Terms terms;
        for (size_t i = 0; i < childCnt; ++i) {
            terms.push_back(wand::Term(childFactory.createChild(i, limit), default_weight, limit / (i + 1)));
        }
        return new rise::TermFrequencyRiseWand(terms, n);
    }
};

struct WeightedSetFactory : SparseVectorFactory {
    mutable TermFieldMatchData tfmd;
    virtual std::string name() const {
        return vespalib::make_string("WeightedSet");
    }
    virtual SearchIterator *createSparseVector(ChildFactory &childFactory, uint32_t childCnt, uint32_t limit) const {
        std::vector<SearchIterator*> terms;
        std::vector<int32_t> weights;
        for (size_t i = 0; i < childCnt; ++i) {
            terms.push_back(childFactory.createChild(i, limit));
            weights.push_back(default_weight);
        }
        return WeightedSetTermSearch::create(terms, tfmd, weights);
    }
};

struct DotProductFactory : SparseVectorFactory {
    mutable TermFieldMatchData tfmd;
    virtual std::string name() const {
        return vespalib::make_string("DotProduct");
    }
    virtual SearchIterator *createSparseVector(ChildFactory &childFactory, uint32_t childCnt, uint32_t limit) const {
        MatchDataLayout layout;
        std::vector<TermFieldHandle> handles;
        for (size_t i = 0; i < childCnt; ++i) {
            handles.push_back(layout.allocTermField(0));
        }
        std::vector<SearchIterator*> terms;
        std::vector<TermFieldMatchData*> childMatch;
        std::vector<int32_t> weights;
        MatchData::UP md = layout.createMatchData();
        for (size_t i = 0; i < childCnt; ++i) {
            terms.push_back(childFactory.createChild(i, limit));
            childMatch.push_back(md->resolveTermField(handles[i]));
            weights.push_back(default_weight);
        }
        return DotProductSearch::create(terms, tfmd, childMatch, weights, std::move(md)).release();
    }
};

struct OrFactory : SparseVectorFactory {
    virtual std::string name() const {
        return vespalib::make_string("Or");
    }
    virtual SearchIterator *createSparseVector(ChildFactory &childFactory, uint32_t childCnt, uint32_t limit) const {
        OrSearch::Children children;
        for (size_t i = 0; i < childCnt; ++i) {
            children.push_back(childFactory.createChild(i, limit));
        }
        return OrSearch::create(children, true);
    }
};

//-----------------------------------------------------------------------------

struct NoFilterStrategy : FilterStrategy {
    virtual std::string name() const {
        return vespalib::make_string("NoFilter");
    }
    virtual SearchIterator *createRoot(SparseVectorFactory &vectorFactory, ChildFactory &childFactory, uint32_t childCnt, uint32_t limit) const {
        return vectorFactory.createSparseVector(childFactory, childCnt, limit);
    }
};

struct PositiveFilterBeforeStrategy : FilterStrategy {
    virtual std::string name() const {
        return vespalib::make_string("PositiveBefore");
    }
    virtual SearchIterator *createRoot(SparseVectorFactory &vectorFactory, ChildFactory &childFactory, uint32_t childCnt, uint32_t limit) const {
        AndSearch::Children children;
        children.push_back(new ModSearch(2, limit)); // <- 50% hits (hardcoded)
        children.push_back(vectorFactory.createSparseVector(childFactory, childCnt, limit));
        return AndSearch::create(children, true);
    }
};

struct NegativeFilterAfterStrategy : FilterStrategy {
    virtual std::string name() const {
        return vespalib::make_string("NegativeAfter");
    }
    virtual SearchIterator *createRoot(SparseVectorFactory &vectorFactory, ChildFactory &childFactory, uint32_t childCnt, uint32_t limit) const {
        AndNotSearch::Children children;
        children.push_back(vectorFactory.createSparseVector(childFactory, childCnt, limit));
        children.push_back(new ModSearch(2, limit)); // <- 50% hits (hardcoded)
        return AndNotSearch::create(children, true);
    }
};

//-----------------------------------------------------------------------------

struct Result {
    double time_ms;
    uint32_t num_hits;
    Result() : time_ms(max_time), num_hits(0) {}
    Result(double t, uint32_t n) : time_ms(t), num_hits(n) {}
    void combine(const Result &r) {
        if (time_ms == max_time) {
            *this = r;
        } else {
            assert(num_hits == r.num_hits);
            time_ms = std::min(time_ms, r.time_ms);
        }
    }
    std::string toString() const {
        return vespalib::make_string("%u hits, %g ms", num_hits, time_ms);
    }
};

Result run_single_benchmark(FilterStrategy &filterStrategy, SparseVectorFactory &vectorFactory, ChildFactory &childFactory, uint32_t childCnt, uint32_t limit) {
    SearchIterator::UP search(filterStrategy.createRoot(vectorFactory, childFactory, childCnt, limit));
    SearchIterator &sb = *search;
    uint32_t num_hits = 0;
    FastOS_Time timer;
    timer.SetNow();
    for (sb.seek(1); !sb.isAtEnd(); sb.seek(sb.getDocId() + 1)) {
        ++num_hits;
        sb.unpack(sb.getDocId());
    }
    return Result(timer.MilliSecsToNow(), num_hits);
}

//-----------------------------------------------------------------------------

// one setup is used to produce all graphs in a single plot
class Setup
{
private:
    FilterStrategy &_filterStrategy;
    ChildFactory &_childFactory;
    uint32_t _limit;
    Plot::UP _plot;

    std::string make_title() const {
        return vespalib::make_string("%u docs, filter:%s, terms:%s", _limit, _filterStrategy.name().c_str(), _childFactory.name().c_str());
    }

public:
    Setup(FilterStrategy &fs, ChildFactory &cf, uint32_t lim) : _filterStrategy(fs), _childFactory(cf), _limit(lim) {
        _plot = Plot::createPlot(make_title());
        fprintf(stderr, "benchmark setup: %s\n", make_title().c_str());
    }

    void benchmark(SparseVectorFactory &svf, const std::vector<uint32_t> &child_counts) {
        Graph::UP graph = _plot->createGraph(svf.name());
        fprintf(stderr, "  search operator: %s\n", svf.name().c_str());
        for (size_t i = 0; i < child_counts.size(); ++i) {
            uint32_t childCnt = child_counts[i];
            Result result;
            for (int j = 0; j < 5; ++j) {
                result.combine(run_single_benchmark(_filterStrategy, svf, _childFactory, childCnt, _limit));
            }
            graph->addValue(childCnt, result.time_ms);
            fprintf(stderr, "    %u children => %s\n", childCnt, result.toString().c_str());
        }
    }
};

//-----------------------------------------------------------------------------

void benchmark_all_operators(Setup &setup, const std::vector<uint32_t> &child_counts) {
    VespaWandFactory       vespaWand256(256);
    RiseWandFactory        riseWand256(256);
    WeightedSetFactory     weightedSet;
    DotProductFactory      dotProduct;
    OrFactory              plain_or;
    setup.benchmark(vespaWand256, child_counts);
    setup.benchmark(riseWand256, child_counts);
    setup.benchmark(weightedSet, child_counts);
    setup.benchmark(dotProduct, child_counts);
    setup.benchmark(plain_or, child_counts);
}

//-----------------------------------------------------------------------------

Box<uint32_t> make_full_child_counts() {
    return Box<uint32_t>()
        .add(10).add(20).add(30).add(40).add(50).add(60).add(70).add(80).add(90)
        .add(100).add(125).add(150).add(175)
        .add(200).add(250).add(300).add(350).add(400).add(450)
        .add(500).add(600).add(700).add(800).add(900)
        .add(1000).add(1200).add(1400).add(1600).add(1800)
        .add(2000);
}

//-----------------------------------------------------------------------------

} // namespace <unnamed>

TEST_FFF("benchmark", NoFilterStrategy(), ModSearchFactory(), Setup(f1, f2, 5000000)) {
    benchmark_all_operators(f3, make_full_child_counts());
}

TEST_FFF("benchmark", NoFilterStrategy(), ModSearchFactory(8), Setup(f1, f2, 5000000)) {
    benchmark_all_operators(f3, make_full_child_counts());
}

TEST_FFF("benchmark", PositiveFilterBeforeStrategy(), ModSearchFactory(), Setup(f1, f2, 5000000)) {
    benchmark_all_operators(f3, make_full_child_counts());
}

TEST_FFF("benchmark", NegativeFilterAfterStrategy(), ModSearchFactory(), Setup(f1, f2, 5000000)) {
    benchmark_all_operators(f3, make_full_child_counts());
}

TEST_MAIN() { TEST_RUN_ALL(); }
