// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
/**
 * \class storage::spi::PartitionState
 * \ingroup spi
 *
 * \brief Information service layer needs about providers partitions.
 *
 * In order to be able to utilize all hardware in parallel, the service layer
 * is aware of partitions, and what buckets exist in various partitions.
 *
 * The service layer needs information about how many partitions exist, and if
 * any of them are currently unavailable. This object describes what the
 * service layer need to know about disks.
 */
#pragma once

#include <persistence/spi/types.h>

namespace storage {
namespace spi {

struct PartitionState {
    enum State { UP, DOWN };

    PartitionState();
    PartitionState(State s, vespalib::stringref reason);

    State getState() const { return _state; }
    const string& getReason() const { return _reason; }

    bool isUp() const { return (_state == UP); }

private:
    State _state;
    string _reason; // If not up, there should be a reason
};

class PartitionStateList {
    std::vector<PartitionState> _states;

public:
    PartitionStateList(PartitionId::Type partitionCount);

    PartitionState& operator[](PartitionId::Type index);
    const PartitionState& operator[](PartitionId::Type index) const
        { return const_cast<PartitionStateList&>(*this)[index]; }

    PartitionId size() const { return PartitionId(_states.size()); }
};

} // spi
} // storage

