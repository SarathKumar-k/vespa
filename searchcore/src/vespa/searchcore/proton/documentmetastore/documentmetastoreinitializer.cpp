// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include "documentmetastoreinitializer.h"
#include "documentmetastore.h"
#include <vespa/searchlib/common/indexmetainfo.h>
#include <vespa/searchcore/proton/common/eventlogger.h>
#include <vespa/vespalib/io/fileutil.h>

using search::GrowStrategy;
using search::IndexMetaInfo;
using vespalib::IllegalStateException;
using proton::initializer::InitializerTask;

namespace proton
{

namespace documentmetastore
{

DocumentMetaStoreInitializer::
DocumentMetaStoreInitializer(const vespalib::string baseDir,
                             const vespalib::string &subDbName,
                             const vespalib::string &docTypeName,
                             DocumentMetaStore::SP dms)
    : _baseDir(baseDir),
      _subDbName(subDbName),
      _docTypeName(docTypeName),
      _dms(dms)
{
}


void
DocumentMetaStoreInitializer::run()
{
    vespalib::string name = DocumentMetaStore::getFixedName();
    IndexMetaInfo info(_baseDir);
    if (info.load()) {
        IndexMetaInfo::Snapshot snap = info.getBestSnapshot();
        if (snap.valid) {
            vespalib::string attrFileName = _baseDir + "/" +
                                            snap.dirName + "/" +
                                            name;
            _dms->setBaseFileName(attrFileName);
            assert(_dms->hasLoadData());
            fastos::TimeStamp startTime = fastos::ClockSystem::now();
            EventLogger::loadDocumentMetaStoreStart(_subDbName);
            if (!_dms->load()) {
                throw IllegalStateException(vespalib::make_string(
                                                    "Failed to load"
                                                    " document meta store "
                                                    "for document "
                                                    "type '%s' from disk",
                                                    _docTypeName.c_str()));
            } else {
                _dms->commit(snap.syncToken,
                             snap.syncToken);
            }
            fastos::TimeStamp endTime = fastos::ClockSystem::now();
            int64_t elapsedTimeMs = (endTime - startTime).ms();
            EventLogger::loadDocumentMetaStoreComplete(_subDbName, elapsedTimeMs);
        }
    } else {
        vespalib::mkdir(_baseDir, false);
        info.save();
    }
}


} // namespace proton::documentmetastore

} // namespace proton
