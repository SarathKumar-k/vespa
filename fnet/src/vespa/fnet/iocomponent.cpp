// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include <vespa/fnet/fnet.h>


FNET_IOComponent::FNET_IOComponent(FNET_TransportThread *owner,
                                   FastOS_Socket *mysocket,
                                   const char *spec,
                                   bool shouldTimeOut)
    : _ioc_next(NULL),
      _ioc_prev(NULL),
      _ioc_owner(owner),
      _ioc_counters(_ioc_owner->GetStatCounters()),
      _ioc_socket(mysocket),
      _ioc_spec(NULL),
      _flags(shouldTimeOut),
      _ioc_timestamp(fastos::ClockSystem::now()),
      _ioc_cond(),
      _ioc_refcnt(1),
      _ioc_directPacketWriteCnt(0),
      _ioc_directDataWriteCnt(0)
{
    _ioc_spec = strdup(spec);
    assert(_ioc_spec != NULL);
}


FNET_IOComponent::~FNET_IOComponent()
{
    free(_ioc_spec);
}


void
FNET_IOComponent::AddRef()
{
    Lock();
    assert(_ioc_refcnt > 0);
    _ioc_refcnt++;
    Unlock();
}


void
FNET_IOComponent::AddRef_NoLock()
{
    assert(_ioc_refcnt > 0);
    _ioc_refcnt++;
}


void
FNET_IOComponent::SubRef()
{
    Lock();
    assert(_ioc_refcnt > 0);
    if (--_ioc_refcnt > 0) {
        Unlock();
        return;
    }
    Unlock();
    CleanupHook();
    delete this;
}


void
FNET_IOComponent::SubRef_HasLock()
{
    assert(_ioc_refcnt > 0);
    if (--_ioc_refcnt > 0) {
        Unlock();
        return;
    }
    Unlock();
    CleanupHook();
    delete this;
}


void
FNET_IOComponent::SubRef_NoLock()
{
    assert(_ioc_refcnt > 1);
    _ioc_refcnt--;
}


void
FNET_IOComponent::SetSocketEvent(FastOS_SocketEvent *event)
{
    bool rc = _ioc_socket->SetSocketEvent(event, this);
    assert(rc); // XXX: error handling
    (void) rc;

    if (event != NULL) {
        _ioc_socket->EnableReadEvent(_flags._ioc_readEnabled);
        _ioc_socket->EnableWriteEvent(_flags._ioc_writeEnabled);
    }
}


void
FNET_IOComponent::EnableReadEvent(bool enabled)
{
    _flags._ioc_readEnabled = enabled;
    if (_ioc_socket->GetSocketEvent() != NULL)
        _ioc_socket->EnableReadEvent(enabled);
}


void
FNET_IOComponent::EnableWriteEvent(bool enabled)
{
    _flags._ioc_writeEnabled = enabled;
    if (_ioc_socket->GetSocketEvent() != NULL)
        _ioc_socket->EnableWriteEvent(enabled);
}


void
FNET_IOComponent::CleanupHook()
{
}
