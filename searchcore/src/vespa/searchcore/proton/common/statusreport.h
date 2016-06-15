// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/vespalib/stllike/string.h>
#include <cmath>
#include <limits>
#include <memory>

namespace proton {

/**
 * A StatusReport describes the status of a search component.
 */
class StatusReport {
public:
    typedef std::unique_ptr<StatusReport> UP;
    typedef std::shared_ptr<StatusReport> SP;
    typedef std::vector<SP> List;

    enum State {
        DOWN = 0,
        PARTIAL,
        UPOK
    };

    struct Params {
        vespalib::string _component;
        State _state;
        vespalib::string _internalState;
        vespalib::string _internalConfigState;
        float _progress;
        vespalib::string _message;

        Params(const vespalib::string &component)
            : _component(component),
              _state(DOWN),
              _internalState(),
              _internalConfigState(),
              _progress(std::numeric_limits<float>::quiet_NaN()),
              _message()
        {}
        Params &state(State value) {
            _state = value;
            return *this;
        }
        Params &internalState(const vespalib::string &value) {
            _internalState = value;
            return *this;
        }
        Params &internalConfigState(const vespalib::string &value) {
            _internalConfigState = value;
            return *this;
        }
        Params &progress(float value) {
            _progress = value;
            return *this;
        }
        Params &message(const vespalib::string &value) {
            _message = value;
            return *this;
        }
    };

private:
    vespalib::string _component;
    State _state;
    vespalib::string _internalState;
    vespalib::string _internalConfigState;
    float _progress;
    vespalib::string _message;

public:
    StatusReport(const Params &params)
        : _component(params._component),
          _state(params._state),
          _internalState(params._internalState),
          _internalConfigState(params._internalConfigState),
          _progress(params._progress),
          _message(params._message)
    {}

    static StatusReport::UP create(const Params &params) {
        return StatusReport::UP(new StatusReport(params));
    }

    const vespalib::string &getComponent() const {
        return _component;
    }

    State getState() const {
        return _state;
    }

    const vespalib::string &getInternalState() const {
        return _internalState;
    }

    const vespalib::string &getInternalConfigState() const {
        return _internalConfigState;
    }

    bool hasProgress() const {
        return !std::isnan(_progress);
    }

    float getProgress() const {
        return _progress;
    }

    const vespalib::string &getMessage() const {
        return _message;
    }

    vespalib::string getInternalStatesStr() const {
        vespalib::string retval = "state=" + _internalState;
        if (!_internalConfigState.empty()) {
            retval = retval + " configstate=" + _internalConfigState;
        }
        return retval;
    }

};




/**
 * A StatusProducer is able to produce a list of StatusReport objects
 * when needed.
 **/
struct StatusProducer {
    virtual StatusReport::List getStatusReports() const = 0;
    virtual ~StatusProducer() {}
};

} // namespace proton

