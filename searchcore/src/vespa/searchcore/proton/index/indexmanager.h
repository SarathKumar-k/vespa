// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/searchcorespi/index/iindexmaintaineroperations.h>
#include <vespa/searchcorespi/index/iindexmanager.h>
#include <vespa/searchcorespi/index/indexmaintainer.h>
#include <vespa/searchcorespi/index/ithreadingservice.h>

namespace proton {

using searchcorespi::IFlushTarget;
using searchcorespi::IIndexManager;

/**
 * The IndexManager provides a holistic view of a set of disk and
 * memory indexes. It allows updating the active index, enables search
 * across all indexes, and manages the set of indexes through flushing
 * of memory indexes and fusion of disk indexes.
 */
class IndexManager : public boost::noncopyable, public IIndexManager
{
public:
    class MaintainerOperations : public searchcorespi::index::IIndexMaintainerOperations {
    private:
        const size_t _cacheSize;
        const search::common::FileHeaderContext &_fileHeaderContext;
        const search::TuneFileIndexing _tuneFileIndexing;
        const search::TuneFileSearch _tuneFileSearch;
        searchcorespi::index::IThreadingService &_threadingService;

    public:
        MaintainerOperations(const search::common::FileHeaderContext &fileHeaderContext,
                             const search::TuneFileIndexManager &tuneFileIndexManager,
                             size_t cacheSize,
                             searchcorespi::index::IThreadingService &
                             threadingService);

        virtual searchcorespi::index::IMemoryIndex::SP
            createMemoryIndex(const search::index::Schema &schema);
        virtual searchcorespi::index::IDiskIndex::SP
            loadDiskIndex(const vespalib::string &indexDir);
        virtual searchcorespi::index::IDiskIndex::SP
            reloadDiskIndex(const searchcorespi::index::IDiskIndex &oldIndex);
        virtual bool runFusion(const search::index::Schema &schema,
                               const vespalib::string &outputDir,
                               const std::vector<vespalib::string> &sources,
                               const search::diskindex::SelectorArray &docIdSelector,
                               search::SerialNum lastSerialNum);
    };

private:
    MaintainerOperations     _operations;
    searchcorespi::index::IndexMaintainer _maintainer;

public:
    IndexManager(const vespalib::string &baseDir,
                 double diskIndexWarmupTime,
                 size_t maxFlushed,
                 size_t cacheSize,
                 const Schema &schema,
                 const Schema &fusionSchema,
                 Reconfigurer &reconfigurer,
                 searchcorespi::index::IThreadingService &threadingService,
                 vespalib::ThreadExecutor & warmupExecutor,
                 const search::TuneFileIndexManager &tuneFileIndexManager,
                 const search::TuneFileAttributes &tuneFileAttributes,
                 const search::common::FileHeaderContext &fileHeaderContext);
    ~IndexManager();

    searchcorespi::index::IndexMaintainer &getMaintainer() {
        return _maintainer;
    }

    /**
     * Implements searchcorespi::IIndexManager
     **/
    virtual void putDocument(uint32_t lid, const Document &doc,
                             SerialNum serialNum) override {
        _maintainer.putDocument(lid, doc, serialNum);
    }

    virtual void removeDocument(uint32_t lid, SerialNum serialNum) override {
        _maintainer.removeDocument(lid, serialNum);
    }

    virtual void commit(SerialNum serialNum,
                        OnWriteDoneType onWriteDone) override {
        _maintainer.commit(serialNum, onWriteDone);
    }

    virtual void heartBeat(SerialNum serialNum) {
        _maintainer.heartBeat(serialNum);
    }

    virtual SerialNum getCurrentSerialNum() const {
        return _maintainer.getCurrentSerialNum();
    }

    virtual SerialNum getFlushedSerialNum() const {
        return _maintainer.getFlushedSerialNum();
    }

    virtual searchcorespi::IndexSearchable::SP getSearchable() const {
        return _maintainer.getSearchable();
    }

    virtual search::SearchableStats getSearchableStats() const {
        return _maintainer.getSearchableStats();
    }

    virtual IFlushTarget::List getFlushTargets() {
        return _maintainer.getFlushTargets();
    }

    virtual void setSchema(const Schema &schema, const Schema &fusionSchema) {
        _maintainer.setSchema(schema, fusionSchema);
    }

    virtual void wipeHistory(SerialNum wipeSerial, const Schema &historyFields) {
        _maintainer.wipeHistory(wipeSerial, historyFields);
    }
};

} // namespace proton

