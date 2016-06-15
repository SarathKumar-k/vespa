// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "datastore.h"

namespace search
{

namespace btree
{


template <typename RefT>
DataStoreT<RefT>::DataStoreT(void)
    : DataStoreBase(RefType::numBuffers(),
                    RefType::offsetSize() / RefType::align(1))
{
}


template <typename RefT>
DataStoreT<RefT>::~DataStoreT(void)
{
}


template <typename RefT>
void
DataStoreT<RefT>::freeElem(EntryRef ref, uint64_t len)
{
    RefType intRef(ref);
    BufferState &state = _states[intRef.bufferId()];
    if (state._state == BufferState::ACTIVE) {
        if (state._freeListList != NULL && len == state.getClusterSize()) {
            if (state._freeList.empty())
                state.addToFreeListList();
            state._freeList.push_back(ref);
        }
    } else {
        assert(state._state == BufferState::HOLD);
    }
    state._deadElems += len;
    state.cleanHold(_buffers[intRef.bufferId()],
                    (intRef.offset() / RefType::align(1)) *
                    state.getClusterSize(), len);
}


template <typename RefT>
void
DataStoreT<RefT>::holdElem(EntryRef ref, uint64_t len)
{
    RefType intRef(ref);
    uint64_t alignedLen = RefType::align(len);
    BufferState &state = _states[intRef.bufferId()];
    assert(state._state == BufferState::ACTIVE);
    if (state._disableElemHoldList) {
        state._deadElems += alignedLen;
        return;
    }
    _elemHold1List.push_back(ElemHold1ListElem(ref, alignedLen));
    state._holdElems += alignedLen;
}


template <typename RefT>
void
DataStoreT<RefT>::trimElemHoldList(generation_t usedGen)
{
    ElemHold2List &elemHold2List = _elemHold2List;

    ElemHold2List::iterator it(elemHold2List.begin());
    ElemHold2List::iterator ite(elemHold2List.end());
    uint32_t freed = 0;
    for (; it != ite; ++it) {
        if (static_cast<sgeneration_t>(it->_generation - usedGen) >= 0)
            break;
        RefType intRef(it->_ref);
        BufferState &state = _states[intRef.bufferId()];
        freeElem(it->_ref, it->_len);
        assert(state._holdElems >= it->_len);
        state._holdElems -= it->_len;
        ++freed;
    }
    if (freed != 0) {
        elemHold2List.erase(elemHold2List.begin(), it);
    }
}


template <typename RefT>
void
DataStoreT<RefT>::clearElemHoldList(void)
{
    ElemHold2List &elemHold2List = _elemHold2List;

    ElemHold2List::iterator it(elemHold2List.begin());
    ElemHold2List::iterator ite(elemHold2List.end());
    for (; it != ite; ++it) {
        RefType intRef(it->_ref);
        BufferState &state = _states[intRef.bufferId()];
        freeElem(it->_ref, it->_len);
        assert(state._holdElems >= it->_len);
        state._holdElems -= it->_len;
    }
    elemHold2List.clear();
}


template <typename RefT>
template <typename EntryType>
std::pair<RefT, EntryType *>
DataStoreT<RefT>::allocNewEntry(uint32_t typeId)
{
    ensureBufferCapacity(typeId, 1);
    uint32_t activeBufferId = getActiveBufferId(typeId);
    BufferState &state = getBufferState(activeBufferId);
    assert(state._state == BufferState::ACTIVE);
    size_t oldSize = state.size();
    EntryType *entry = getBufferEntry<EntryType>(activeBufferId, oldSize);
    new (static_cast<void *>(entry)) EntryType();
    state.pushed_back(1);
    return std::make_pair(RefType(oldSize, activeBufferId), entry);
}


template <typename RefT>
template <typename EntryType, typename Reclaimer>
std::pair<RefT, EntryType *>
DataStoreT<RefT>::allocEntry(uint32_t typeId)
{
    BufferState::FreeListList &freeListList = getFreeList(typeId);
    if (freeListList._head == NULL) {
        return allocNewEntry<EntryType>(typeId);
    }
    BufferState &state = *freeListList._head;
    assert(state._state == BufferState::ACTIVE);
    RefType ref(state.popFreeList());
    EntryType *entry =
        getBufferEntry<EntryType>(ref.bufferId(), ref.offset());
    Reclaimer::reclaim(entry);
    return std::make_pair(ref, entry);
}


template <typename RefT>
template <typename EntryType>
std::pair<RefT, EntryType *>
DataStoreT<RefT>::allocNewEntryCopy(uint32_t typeId, const EntryType &rhs)
{
    ensureBufferCapacity(typeId, 1);
    uint32_t activeBufferId = getActiveBufferId(typeId);
    BufferState &state = getBufferState(activeBufferId);
    assert(state._state == BufferState::ACTIVE);
    size_t oldSize = state.size();
    EntryType *entry = getBufferEntry<EntryType>(activeBufferId, oldSize);
    new (static_cast<void *>(entry)) EntryType(rhs);
    state.pushed_back(1);
    return std::make_pair(RefType(oldSize, activeBufferId), entry);
}


template <typename RefT>
template <typename EntryType, typename Reclaimer>
std::pair<RefT, EntryType *>
DataStoreT<RefT>::allocEntryCopy(uint32_t typeId, const EntryType &rhs)
{
    BufferState::FreeListList &freeListList = getFreeList(typeId);
    if (freeListList._head == NULL) {
        return allocNewEntryCopy<EntryType>(typeId, rhs);
    }
    BufferState &state = *freeListList._head;
    assert(state._state == BufferState::ACTIVE);
    RefType ref(state.popFreeList());
    EntryType *entry =
        getBufferEntry<EntryType>(ref.bufferId(), ref.offset());
    Reclaimer::reclaim(entry);
    *entry = rhs;
    return std::make_pair(ref, entry);
}



template <typename EntryType, typename RefT>
DataStore<EntryType, RefT>::DataStore(void)
    : ParentType(),
      _type(1, RefType::offsetSize(), RefType::offsetSize())
{
    addType(&_type);
    initActiveBuffers();
}

template <typename EntryType, typename RefT>
DataStore<EntryType, RefT>::~DataStore(void)
{
    dropBuffers();	// Drop buffers before type handlers are dropped
}

template <typename EntryType, typename RefT>
EntryRef
DataStore<EntryType, RefT>::addEntry(const EntryType &e)
{
    ensureBufferCapacity(0, 1);
    uint32_t activeBufferId = _activeBufferIds[0];
    BufferState &state = _states[activeBufferId];
    size_t oldSize = state.size();
    EntryType *be = static_cast<EntryType *>(_buffers[activeBufferId]) +
                    oldSize;
    new (static_cast<void *>(be)) EntryType(e);
    RefType ref(oldSize, activeBufferId);
    state.pushed_back(1);
    return ref;
}

template <typename EntryType, typename RefT>
EntryRef
DataStore<EntryType, RefT>::addEntry2(const EntryType &e)
{
    BufferState::FreeListList &freeListList = _freeListLists[0];
    if (freeListList._head == NULL)
        return addEntry(e);
    BufferState &state = *freeListList._head;
    assert(state._state == BufferState::ACTIVE);
    RefType ref(state.popFreeList());
    EntryType *be =
        this->template
        getBufferEntry<EntryType>(ref.bufferId(), ref.offset());
    *be = e;
    return ref;
}

template <typename EntryType, typename RefT>
const EntryType &
DataStore<EntryType, RefT>::getEntry(EntryRef ref) const
{
    RefType intRef(ref);
    const EntryType *be =
        this->template
        getBufferEntry<EntryType>(intRef.bufferId(), intRef.offset());
    return *be;
}

extern template class DataStoreT<EntryRefT<22> >;

} // namespace search::btree
} // namespace search

