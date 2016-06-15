// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/vespalib/testkit/test_kit.h>
#include <vespa/searchcore/proton/matching/docid_range_scheduler.h>
#include <vespa/vespalib/util/benchmark_timer.h>
#include <vespa/vespalib/util/stringfmt.h>

using namespace proton::matching;
using namespace vespalib;

//-----------------------------------------------------------------------------

size_t do_work(size_t cost) __attribute__((noinline));
size_t do_work(size_t cost) {
    size_t result = 0;
    size_t loop_cnt = 42;
    for (size_t n = 0; n < cost; ++n) {
        result += (cost * n);
        for (size_t i = 0; i < loop_cnt; ++i) {
            result += (cost * n * i);
            for (size_t j = 0; j < loop_cnt; ++j) {
                result += (cost * n * i * j);
                for (size_t k = 0; k < loop_cnt; ++k) {
                    result += (cost * n * i * j * k);
                }
            }
        }
    }
    return result;
}

//-----------------------------------------------------------------------------

TEST("measure do_work overhead for different cost inputs") {
    for (size_t cost: {0, 1, 10, 100, 1000}) {
        BenchmarkTimer timer(1.0);
        while (timer.has_budget()) {
            timer.before();
            (void) do_work(cost);
            timer.after();
        }
        double min_time_s = timer.min_time();
        fprintf(stderr, "const %zu: %g us\n", cost, min_time_s * 1000.0 * 1000.0);
    }
}

//-----------------------------------------------------------------------------

struct Work {
    typedef std::unique_ptr<Work> UP;
    virtual vespalib::string desc() const = 0;
    virtual void perform(uint32_t docid) const = 0;
    virtual ~Work() {}
};

struct UniformWork : public Work {
    size_t cost;
    UniformWork(size_t cost_in) : cost(cost_in) {}
    vespalib::string desc() const override { return make_string("uniform(%zu)", cost); }
    void perform(uint32_t) const override { (void) do_work(cost); }
};

struct TriangleWork : public Work {
    size_t div;
    TriangleWork(size_t div_in) : div(div_in) {}
    vespalib::string desc() const override { return make_string("triangle(docid/%zu)", div); }
    void perform(uint32_t docid) const override { (void) do_work(docid/div); }
};

struct SpikeWork : public Work {
    uint32_t begin;
    uint32_t end;
    size_t cost;
    SpikeWork(uint32_t begin_in, uint32_t end_in, size_t cost_in)
        : begin(begin_in), end(end_in), cost(cost_in) {}
    vespalib::string desc() const override { return make_string("spike(%u,%u,%zu)", begin, end, cost); }
    void perform(uint32_t docid) const override {
        if ((docid >= begin) && (docid < end)) {
            (void) do_work(cost);
        }
    }
};

struct WorkList {
    std::vector<Work::UP> work_list;
    WorkList() : work_list() {
        work_list.push_back(std::make_unique<UniformWork>(10));
        work_list.push_back(std::make_unique<TriangleWork>(4878));
        work_list.push_back(std::make_unique<SpikeWork>(1, 10001, 100));
        work_list.push_back(std::make_unique<SpikeWork>(1, 1001, 1000));
        work_list.push_back(std::make_unique<SpikeWork>(1, 101, 10000));
        work_list.push_back(std::make_unique<SpikeWork>(1, 11, 100000));
        work_list.push_back(std::make_unique<SpikeWork>(90001, 100001, 100));
        work_list.push_back(std::make_unique<SpikeWork>(99001, 100001, 1000));
        work_list.push_back(std::make_unique<SpikeWork>(99901, 100001, 10000));
        work_list.push_back(std::make_unique<SpikeWork>(99991, 100001, 100000));
    }
};

//-----------------------------------------------------------------------------

struct SchedulerFactory {
    typedef std::unique_ptr<SchedulerFactory> UP;
    virtual vespalib::string desc() const = 0;    
    virtual DocidRangeScheduler::UP create(uint32_t docid_limit) const = 0;
    virtual ~SchedulerFactory() {}
};

struct PartitionSchedulerFactory : public SchedulerFactory {
    size_t num_threads;
    PartitionSchedulerFactory(size_t num_threads_in) : num_threads(num_threads_in) {}
    vespalib::string desc() const override { return make_string("partition(threads:%zu)", num_threads); }
    DocidRangeScheduler::UP create(uint32_t docid_limit) const override {
        return std::make_unique<PartitionDocidRangeScheduler>(num_threads, docid_limit);
    }
};

struct TaskSchedulerFactory : public SchedulerFactory {
    size_t num_threads;
    size_t num_tasks;
    TaskSchedulerFactory(size_t num_threads_in, size_t num_tasks_in)
        : num_threads(num_threads_in), num_tasks(num_tasks_in) {}
    vespalib::string desc() const override { return make_string("task(threads:%zu,num_tasks:%zu)", num_threads, num_tasks); }
    DocidRangeScheduler::UP create(uint32_t docid_limit) const override {
        return std::make_unique<TaskDocidRangeScheduler>(num_threads, num_tasks, docid_limit);
    }
};

struct AdaptiveSchedulerFactory : public SchedulerFactory {
    size_t num_threads;
    size_t min_task;
    AdaptiveSchedulerFactory(size_t num_threads_in, size_t min_task_in)
        : num_threads(num_threads_in), min_task(min_task_in) {}
    vespalib::string desc() const override { return make_string("adaptive(threads:%zu,min_task:%zu)", num_threads, min_task); }
    DocidRangeScheduler::UP create(uint32_t docid_limit) const override {
        return std::make_unique<AdaptiveDocidRangeScheduler>(num_threads, min_task, docid_limit);
    }
};

struct SchedulerList {
    std::vector<SchedulerFactory::UP> factory_list;
    SchedulerList(size_t num_threads) : factory_list() {
        factory_list.push_back(std::make_unique<PartitionSchedulerFactory>(num_threads));
        factory_list.push_back(std::make_unique<TaskSchedulerFactory>(num_threads, num_threads));
        factory_list.push_back(std::make_unique<TaskSchedulerFactory>(num_threads, 64));
        factory_list.push_back(std::make_unique<TaskSchedulerFactory>(num_threads, 256));
        factory_list.push_back(std::make_unique<TaskSchedulerFactory>(num_threads, 1024));
        factory_list.push_back(std::make_unique<TaskSchedulerFactory>(num_threads, 4096));
        factory_list.push_back(std::make_unique<AdaptiveSchedulerFactory>(num_threads, 1000));
        factory_list.push_back(std::make_unique<AdaptiveSchedulerFactory>(num_threads, 100));
        factory_list.push_back(std::make_unique<AdaptiveSchedulerFactory>(num_threads, 10));
        factory_list.push_back(std::make_unique<AdaptiveSchedulerFactory>(num_threads, 1));
    }
};

//-----------------------------------------------------------------------------

void worker(DocidRangeScheduler &scheduler, const Work &work, size_t thread_id) {
    IdleObserver observer = scheduler.make_idle_observer();
    if (observer.is_always_zero()) {
        for (DocidRange range = scheduler.first_range(thread_id);
             !range.empty();
             range = scheduler.next_range(thread_id))
        {
            do_work(10); // represents init-range cost
            for (uint32_t docid = range.begin; docid < range.end; ++docid) {
                work.perform(docid);
            }
        }
    } else {
        for (DocidRange range = scheduler.first_range(thread_id);
             !range.empty();
             range = scheduler.next_range(thread_id))
        {
            do_work(10); // represents init-range cost
            for (uint32_t docid = range.begin; docid < range.end; ++docid) {
                work.perform(docid);
                if (observer.get() > 0) {
                    range = scheduler.share_range(thread_id, DocidRange(docid, range.end));
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------

TEST_MT_FFF("benchmark different combinations of schedulers and work loads", 8,
            DocidRangeScheduler::UP(), SchedulerList(num_threads), WorkList())
{
    if (thread_id == 0) {
        fprintf(stderr, "Benchmarking with %zu threads:\n", num_threads);
    }
    for (size_t scheduler = 0; scheduler < f2.factory_list.size(); ++scheduler) {
        for (size_t work = 0; work < f3.work_list.size(); ++work) {
            if (thread_id == 0) {
                fprintf(stderr, "  scheduler: %s, work load: %s ",
                        f2.factory_list[scheduler]->desc().c_str(),
                        f3.work_list[work]->desc().c_str());
            }
            BenchmarkTimer timer(1.0);
            for (size_t i = 0; i < 5; ++i) {
                TEST_BARRIER();
                if (thread_id == 0) {
                    f1 = f2.factory_list[scheduler]->create(100001);
                }
                TEST_BARRIER();
                timer.before();
                worker(*f1, *f3.work_list[work], thread_id);
                TEST_BARRIER();
                timer.after();
                if (thread_id == 0) {
                    fprintf(stderr, ".");
                }
            }
            if (thread_id == 0) {
                fprintf(stderr, " real time: %g ms\n", timer.min_time() * 1000.0);
            }
        }
    }
}

//-----------------------------------------------------------------------------

TEST_MAIN() { TEST_RUN_ALL(); }
