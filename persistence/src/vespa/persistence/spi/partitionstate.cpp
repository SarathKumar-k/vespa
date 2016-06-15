// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/persistence/spi/partitionstate.h>

namespace storage {
namespace spi {

PartitionState::PartitionState()
    : _state(UP),
      _reason()
{
}

PartitionState::PartitionState(State s, vespalib::stringref reason)
    : _state(s),
      _reason(reason)
{
}


PartitionStateList::PartitionStateList(PartitionId::Type partitionCount)
    : _states(partitionCount)
{
}

PartitionState&
PartitionStateList::operator[](PartitionId::Type index)
{
    if (index >= _states.size()) {
        std::ostringstream ost;
        ost << "Cannot return disk " << index << " of " << _states.size();
        throw vespalib::IllegalArgumentException(ost.str(), VESPA_STRLOC);
    }
    return _states[index];
}

} // spi
} // storage
