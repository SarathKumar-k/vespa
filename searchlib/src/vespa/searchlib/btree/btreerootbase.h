// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "btreetraits.h"
#include "btreenode.h"
#include "btreenodeallocator.h"
#include <atomic>

namespace search {
namespace btree {

template <typename KeyT,
          typename DataT,
          typename AggrT,
          size_t INTERNAL_SLOTS,
          size_t LEAF_SLOTS>
class BTreeRootBase
{
protected:
    typedef KeyT  KeyType;
    typedef DataT DataType;
    typedef AggrT AggregatedType;
    typedef BTreeRootBase<KeyT, DataT, AggrT, INTERNAL_SLOTS, LEAF_SLOTS>
    BTreeRootBaseType;
    typedef BTreeInternalNode<KeyT, AggrT, INTERNAL_SLOTS> InternalNodeType;
    typedef BTreeLeafNode<KeyT, DataT, AggrT, LEAF_SLOTS>  LeafNodeType;
    typedef BTreeNodeAllocator<KeyT, DataT, AggrT,
                               INTERNAL_SLOTS, LEAF_SLOTS> NodeAllocatorType;

    BTreeNode::Ref  _root;
    std::atomic<uint32_t> _frozenRoot;

    static_assert(sizeof(_root) == sizeof(_frozenRoot),
                  "BTree root reference size mismatch");

    BTreeRootBase(void);

    BTreeRootBase(const BTreeRootBase &rhs);

    BTreeRootBase &operator=(const BTreeRootBase &rhs);

    ~BTreeRootBase(void);

public:
    void
    freeze(NodeAllocatorType &allocator);

    bool isFrozen() const {
        return (_root.ref() == _frozenRoot.load(std::memory_order_relaxed));
    }

    void
    setRoot(BTreeNode::Ref newRoot, NodeAllocatorType &allocator)
    {
        bool oldFrozen = isFrozen();
        _root = newRoot;
        if (oldFrozen && !isFrozen())
            allocator.needFreeze(this);
    }

    void
    setRoots(BTreeNode::Ref newRoot)
    {
        _root = newRoot;
        _frozenRoot = newRoot.ref();
    }

    BTreeNode::Ref
    getRoot(void) const
    {
        return _root;
    }

    BTreeNode::Ref
    getFrozenRoot(void) const
    {
        return BTreeNode::Ref(_frozenRoot.load(std::memory_order_acquire));
    }

    BTreeNode::Ref
    getFrozenRootRelaxed(void) const
    {
        return BTreeNode::Ref(_frozenRoot.load(std::memory_order_relaxed));
    }

    const AggrT &
    getAggregated(const NodeAllocatorType &allocator) const
    {
        return allocator.getAggregated(_root);
    }

    void
    recycle(void)
    {
        _root = BTreeNode::Ref();
        _frozenRoot = BTreeNode::Ref().ref();
    }

protected:
    void
    recursiveDelete(BTreeNode::Ref node, NodeAllocatorType &allocator);
};

extern template class BTreeRootBase<uint32_t, uint32_t,
                                    NoAggregated,
                                    BTreeDefaultTraits::INTERNAL_SLOTS,
                                    BTreeDefaultTraits::LEAF_SLOTS>;
extern template class BTreeRootBase<uint32_t, BTreeNoLeafData,
                                    NoAggregated,
                                    BTreeDefaultTraits::INTERNAL_SLOTS,
                                    BTreeDefaultTraits::LEAF_SLOTS>;
extern template class BTreeRootBase<uint32_t, int32_t,
                                    MinMaxAggregated,
                                    BTreeDefaultTraits::INTERNAL_SLOTS,
                                    BTreeDefaultTraits::LEAF_SLOTS>;

} // namespace btree
} // namespace search


