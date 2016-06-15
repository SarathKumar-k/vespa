// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include "singleenumattributesaver.h"
#include <vespa/searchlib/util/bufferwriter.h>

using vespalib::GenerationHandler;

namespace search {

SingleValueEnumAttributeSaver::
SingleValueEnumAttributeSaver(GenerationHandler::Guard &&guard,
                              const IAttributeSaveTarget::Config &cfg,
                              EnumIndexCopyVector &&indices,
                              const EnumStoreBase &enumStore)
    : AttributeSaver(std::move(guard), cfg),
      _indices(std::move(indices)),
      _enumSaver(enumStore, false)
{
}


SingleValueEnumAttributeSaver::~SingleValueEnumAttributeSaver()
{
}


bool
SingleValueEnumAttributeSaver::onSave(IAttributeSaveTarget &saveTarget)
{
    _enumSaver.writeUdat(saveTarget);
    const EnumStoreBase &enumStore = _enumSaver.getEnumStore();
    std::unique_ptr<search::BufferWriter> datWriter(saveTarget.datWriter().
                                                    allocBufferWriter());
    if (saveTarget.getEnumerated()) {
        enumStore.writeEnumValues(*datWriter,
                                  &_indices[0], _indices.size());
    } else {
        enumStore.writeValues(*datWriter,
                              &_indices[0], _indices.size());
    }
    datWriter->flush();
    _enumSaver.enableReEnumerate();
    return true;
}


}  // namespace search
