// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "ithreadingservice.h"
#include <vespa/searchcorespi/index/iindexmanager.h>
#include <vespa/searchlib/common/tunefileinfo.h>
#include <vespa/searchlib/common/fileheadercontext.h>
#include <vespa/vespalib/util/threadexecutor.h>

namespace searchcorespi {
namespace index {

/**
 * Class that keeps the long-lived context used by an index maintainer.
 */
class IndexMaintainerContext {
private:
    IThreadingService &_threadingService;
    searchcorespi::IIndexManager::Reconfigurer &_reconfigurer;
    const search::common::FileHeaderContext &_fileHeaderContext;
    vespalib::ThreadExecutor & _warmupExecutor;

public:
    IndexMaintainerContext(IThreadingService &threadingService,
                           searchcorespi::IIndexManager::Reconfigurer &reconfigurer,
                           const search::common::FileHeaderContext &fileHeaderContext,
                           vespalib::ThreadExecutor & warmupExecutor);

    /**
     * Returns the treading service that encapsulates the thread model used for writing.
     */
    IThreadingService &getThreadingService() const {
        return _threadingService;
    }

    /**
     * Returns the reconfigurer used to signal when the index maintainer has changed.
     */
    searchcorespi::IIndexManager::Reconfigurer &getReconfigurer() const {
        return _reconfigurer;
    }

    /**
     * Returns the context used to insert extra tags into file headers before writing them.
     */
    const search::common::FileHeaderContext &getFileHeaderContext() const {
        return _fileHeaderContext;
    }

    /**
     * @return The executor that should be used for warmup.
     */
    vespalib::ThreadExecutor & getWarmupExecutor() const {  return _warmupExecutor; }
};

} // namespace index
} // namespace searchcorespi


