// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP(".proton.docsummary.document_store_explorer");
#include "document_store_explorer.h"

using vespalib::slime::Cursor;
using vespalib::slime::Inserter;
using search::DataStoreFileChunkStats;
using search::DataStoreStorageStats;

namespace proton {

DocumentStoreExplorer::DocumentStoreExplorer(ISummaryManager::SP mgr)
    : _mgr(std::move(mgr))
{
}

void
DocumentStoreExplorer::get_state(const Inserter &inserter, bool full) const
{
    Cursor &object = inserter.insertObject();
    search::IDocumentStore &store = _mgr->getBackingStore();
    DataStoreStorageStats storageStats(store.getStorageStats());
    object.setLong("diskUsage", storageStats.diskUsage());
    object.setLong("diskBloat", storageStats.diskBloat());
    object.setDouble("maxBucketSpread", storageStats.maxBucketSpread());
    object.setLong("lastFlushedSerialNum", storageStats.lastFlushedSerialNum());
    object.setLong("lastSerialNum", storageStats.lastSerialNum());
    if (full) {
        const vespalib::string &baseDir = store.getBaseDir();
        std::vector<DataStoreFileChunkStats> chunks;
        chunks = store.getFileChunkStats();
        Cursor &fileChunksArrayCursor = object.setArray("fileChunks");
        for (const auto &chunk : chunks) {
            Cursor &chunkCursor = fileChunksArrayCursor.addObject();
            chunkCursor.setLong("diskUsage", chunk.diskUsage());
            chunkCursor.setLong("diskBloat", chunk.diskBloat());
            chunkCursor.setDouble("bucketSpread", chunk.maxBucketSpread());
            chunkCursor.setLong("lastFlushedSerialNum",
                                chunk.lastFlushedSerialNum());
            chunkCursor.setLong("lastSerialNum", chunk.lastSerialNum());
            chunkCursor.setLong("nameid", chunk.nameId());
            chunkCursor.setString("name", chunk.createName(baseDir));
        }
    }
}

} // namespace proton
