// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/storageframework/defaultimplementation/clock/realclock.h>
#include <tests/dummystoragelink.h>
#include <sys/time.h>

namespace storage {

DummyStorageLink* DummyStorageLink::_last(0);

DummyStorageLink::DummyStorageLink()
    : StorageLink("Dummy storage link"),
      _commands(),
      _replies(),
      _injected(),
      _autoReply(false),
      _useDispatch(false),
      _ignore(false),
      _waitMonitor()
{
    _last = this;
}

DummyStorageLink::~DummyStorageLink()
{
        // Often a chain with dummy link on top is deleted in unit tests.
        // If they haven't been closed already, close them for a cleaner
        // shutdown
    if (getState() == OPENED) {
        close();
        flush();
    }
    closeNextLink();
    reset();
}

bool DummyStorageLink::onDown(const api::StorageMessage::SP& cmd)
{
    if (_ignore) {
        return false;
    }
    if (_injected.size() > 0) {
        vespalib::LockGuard guard(_lock);
        sendUp(*_injected.begin());
        _injected.pop_front();
    } else if (_autoReply) {
        if (!cmd->getType().isReply()) {
            std::shared_ptr<api::StorageReply> reply(
                    std::dynamic_pointer_cast<api::StorageCommand>(cmd)
                    ->makeReply().release());
            reply->setResult(api::ReturnCode(
                    api::ReturnCode::OK, "Automatically generated reply"));
            sendUp(reply);
        }
    }
    if (isBottom()) {
        vespalib::MonitorGuard lock(_waitMonitor);
        {
            vespalib::LockGuard guard(_lock);
            _commands.push_back(cmd);
        }
        lock.broadcast();
        return true;
    }
    return StorageLink::onDown(cmd);
}

bool DummyStorageLink::onUp(const api::StorageMessage::SP& reply) {
    if (isTop()) {
        vespalib::MonitorGuard lock(_waitMonitor);
        {
            vespalib::LockGuard guard(_lock);
            _replies.push_back(reply);
        }
        lock.broadcast();
        return true;
    }
    return StorageLink::onUp(reply);

}

void DummyStorageLink::injectReply(api::StorageReply* reply)
{
    assert(reply);
    vespalib::LockGuard guard(_lock);
    _injected.push_back(std::shared_ptr<api::StorageReply>(reply));
}

void DummyStorageLink::reset() {
    vespalib::MonitorGuard lock(_waitMonitor);
    vespalib::LockGuard guard(_lock);
    _commands.clear();
    _replies.clear();
    _injected.clear();
}

void DummyStorageLink::waitForMessages(unsigned int msgCount, int timeout)
{
    framework::defaultimplementation::RealClock clock;
    framework::MilliSecTime endTime(
            clock.getTimeInMillis() + framework::MilliSecTime(timeout * 1000));
    vespalib::MonitorGuard lock(_waitMonitor);
    while (_commands.size() + _replies.size() < msgCount) {
        if (timeout != 0 && clock.getTimeInMillis() > endTime) {
            std::ostringstream ost;
            ost << "Timed out waiting for " << msgCount << " messages to "
                << "arrive in dummy storage link. Only "
                << (_commands.size() + _replies.size()) << " messages seen "
                << "after timout of " << timeout << " seconds was reached.";
            throw vespalib::IllegalStateException(ost.str(), VESPA_STRLOC);
        }
        if (timeout >= 0) {
            lock.wait((endTime - clock.getTimeInMillis()).getTime());
        } else {
            lock.wait();
        }
    }
}

void DummyStorageLink::waitForMessage(const api::MessageType& type, int timeout)
{
    framework::defaultimplementation::RealClock clock;
    framework::MilliSecTime endTime(
            clock.getTimeInMillis() + framework::MilliSecTime(timeout * 1000));
    vespalib::MonitorGuard lock(_waitMonitor);
    while (true) {
        for (uint32_t i=0; i<_commands.size(); ++i) {
            if (_commands[i]->getType() == type) return;
        }
        for (uint32_t i=0; i<_replies.size(); ++i) {
            if (_replies[i]->getType() == type) return;
        }
        if (timeout != 0 && clock.getTimeInMillis() > endTime) {
            std::ostringstream ost;
            ost << "Timed out waiting for " << type << " message to "
                << "arrive in dummy storage link. Only "
                << (_commands.size() + _replies.size()) << " messages seen "
                << "after timout of " << timeout << " seconds was reached.";
            if (_commands.size() == 1) {
                ost << " Found command of type " << _commands[0]->getType();
            }
            if (_replies.size() == 1) {
                ost << " Found command of type " << _replies[0]->getType();
            }
            throw vespalib::IllegalStateException(ost.str(), VESPA_STRLOC);
        }
        if (timeout >= 0) {
            lock.wait((endTime - clock.getTimeInMillis()).getTime());
        } else {
            lock.wait();
        }
    }
}

api::StorageMessage::SP
DummyStorageLink::getAndRemoveMessage(const api::MessageType& type)
{
    vespalib::MonitorGuard lock(_waitMonitor);
    for (std::vector<api::StorageMessage::SP>::iterator it = _commands.begin();
         it != _commands.end(); ++it)
    {
        if ((*it)->getType() == type) {
            api::StorageMessage::SP result(*it);
            _commands.erase(it);
            return result;
        }
    }
    for (std::vector<api::StorageMessage::SP>::iterator it = _replies.begin();
         it != _replies.end(); ++it)
    {
        if ((*it)->getType() == type) {
            api::StorageMessage::SP result(*it);
            _replies.erase(it);
            return result;
        }
    }
    std::ostringstream ost;
    ost << "No message of type " << type << " found.";
    throw vespalib::IllegalStateException(ost.str(), VESPA_STRLOC);
}

} // storage
