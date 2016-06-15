// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "configsnapshot.h"
#include <vespa/vespalib/stllike/asciistream.h>
#include <vespa/config/common/misc.h>

using vespalib::Slime;
using vespalib::slime::Cursor;
using vespalib::slime::Inspector;
using vespalib::slime::Memory;

namespace config {

const int64_t ConfigSnapshot::SNAPSHOT_FORMAT_VERSION = 1;

ConfigSnapshot::ConfigSnapshot()
    : _valueMap(),
      _generation(0)
{}

ConfigSnapshot::~ConfigSnapshot()
{
}

ConfigSnapshot::ConfigSnapshot(const ConfigSnapshot & rhs) :
    _valueMap(rhs._valueMap),
    _generation(rhs._generation)
{
}

ConfigSnapshot &
ConfigSnapshot::operator = (const ConfigSnapshot & rhs)
{
    if (&rhs != this) {
        ConfigSnapshot tmp(rhs);
        tmp.swap(*this);
    }
    return *this;
}

void
ConfigSnapshot::swap(ConfigSnapshot & rhs)
{
    _valueMap.swap(rhs._valueMap);
    std::swap(_generation, rhs._generation);
}

ConfigSnapshot::ConfigSnapshot(const SubscriptionList & subscriptionList, int64_t generation)
    : _valueMap(),
      _generation(generation)
{
    for (SubscriptionList::const_iterator it(subscriptionList.begin()), mt(subscriptionList.end()); it != mt; it++) {
        _valueMap[(*it)->getKey()] = Value((*it)->getLastGenerationChanged(), (*it)->getConfig());
    }
}

ConfigSnapshot::ConfigSnapshot(const ValueMap & valueMap, int64_t generation)
    : _valueMap(valueMap),
      _generation(generation)
{
}

ConfigSnapshot
ConfigSnapshot::subset(const ConfigKeySet & keySet) const
{
    ValueMap subSet;
    for (ConfigKeySet::const_iterator it(keySet.begin()), mt(keySet.end()); it != mt; it++) {
        ValueMap::const_iterator found(_valueMap.find(*it));
        if (found != _valueMap.end()) {
            subSet[*it] = found->second;
        }
    }
    return ConfigSnapshot(subSet, _generation);
}

int64_t ConfigSnapshot::getGeneration() const { return _generation; }
size_t ConfigSnapshot::size() const { return _valueMap.size(); }
bool ConfigSnapshot::empty() const { return _valueMap.empty(); }

void
ConfigSnapshot::serialize(ConfigDataBuffer & buffer) const
{
    Slime & slime(buffer.slimeObject());
    Cursor & root(slime.setObject());
    root.setDouble("version", SNAPSHOT_FORMAT_VERSION);

    switch (SNAPSHOT_FORMAT_VERSION) {
        case 1:
            serializeV1(root);
            break;
        case 2:
            serializeV2(root);
            break;
        default:
            vespalib::asciistream ss;
            ss << "Version '" << SNAPSHOT_FORMAT_VERSION << "' is not a valid version.";
            throw ConfigWriteException(ss.str());
    }
}

void
ConfigSnapshot::serializeV1(Cursor & root) const
{
    root.setDouble("generation", _generation);
    Cursor & snapshots(root.setArray("snapshots"));
    for (ValueMap::const_iterator it(_valueMap.begin()), mt(_valueMap.end()); it != mt; it++) {
        Cursor & snapshot(snapshots.addObject());
        serializeKeyV1(snapshot.setObject("configKey"), it->first);
        serializeValueV1(snapshot.setObject("configPayload"), it->second);
    }
}

void
ConfigSnapshot::serializeV2(Cursor & root) const
{
    root.setDouble("generation", _generation);
    Cursor & snapshots(root.setArray("snapshots"));
    for (ValueMap::const_iterator it(_valueMap.begin()), mt(_valueMap.end()); it != mt; it++) {
        Cursor & snapshot(snapshots.addObject());
        serializeKeyV1(snapshot.setObject("configKey"), it->first);
        serializeValueV2(snapshot.setObject("configPayload"), it->second);
    }
}

void
ConfigSnapshot::serializeKeyV1(Cursor & cursor, const ConfigKey & key) const
{
    typedef std::vector<vespalib::string> SchemaVector;
    cursor.setString("configId", Memory(key.getConfigId()));
    cursor.setString("defName", Memory(key.getDefName()));
    cursor.setString("defNamespace", Memory(key.getDefNamespace()));
    cursor.setString("defMd5", Memory(key.getDefMd5()));
    Cursor & defSchema(cursor.setArray("defSchema"));
    const SchemaVector & vec(key.getDefSchema());
    for (SchemaVector::const_iterator it(vec.begin()), mt(vec.end()); it != mt; it++) {
        defSchema.addString(vespalib::slime::Memory(*it));
    }
}

void
ConfigSnapshot::serializeValueV1(Cursor & cursor, const Value & value) const
{
    cursor.setDouble("lastChanged", value.first);
    value.second.serializeV1(cursor.setArray("lines"));
}

void
ConfigSnapshot::serializeValueV2(Cursor & cursor, const Value & value) const
{
    cursor.setDouble("lastChanged", value.first);
    cursor.setString("md5", Memory(value.second.getMd5()));
    value.second.serializeV2(cursor.setObject("payload"));
}

void
ConfigSnapshot::deserialize(const ConfigDataBuffer & buffer)
{
    const Slime & slime(buffer.slimeObject());
    Inspector & inspector(slime.get());
    int64_t version = static_cast<int64_t>(inspector["version"].asDouble());
    switch (version) {
    case 1:
        deserializeV1(inspector);
        break;
    case 2:
        deserializeV2(inspector);
        break;
    default:
        vespalib::asciistream ss;
        ss << "Version '" << version << "' is not a valid version.";
        throw ConfigReadException(ss.str());
    }
}

void
ConfigSnapshot::deserializeV1(Inspector & root)
{
    _generation = static_cast<int64_t>(root["generation"].asDouble());
    Inspector & snapshots(root["snapshots"]);
    for (size_t i = 0; i < snapshots.children(); i++) {
        Inspector & snapshot(snapshots[i]);
        ConfigKey key(deserializeKeyV1(snapshot["configKey"]));
        Value value(deserializeValueV1(snapshot["configPayload"]));
        _valueMap[key] = value;
    }
}

void
ConfigSnapshot::deserializeV2(Inspector & root)
{
    _generation = static_cast<int64_t>(root["generation"].asDouble());
    Inspector & snapshots(root["snapshots"]);
    for (size_t i = 0; i < snapshots.children(); i++) {
        Inspector & snapshot(snapshots[i]);
        ConfigKey key(deserializeKeyV1(snapshot["configKey"]));
        Value value(deserializeValueV2(snapshot["configPayload"]));
        _valueMap[key] = value;
    }
}

ConfigKey
ConfigSnapshot::deserializeKeyV1(Inspector & inspector) const
{
    std::vector<vespalib::string> schema;
    Inspector & s(inspector["defSchema"]);
    for (size_t i = 0; i < s.children(); i++) {
        schema.push_back(s[i].asString().make_string());
    }
    return ConfigKey(inspector["configId"].asString().make_string(),
                     inspector["defName"].asString().make_string(),
                     inspector["defNamespace"].asString().make_string(),
                     inspector["defMd5"].asString().make_string(),
                     schema);

}

std::pair<int64_t, ConfigValue>
ConfigSnapshot::deserializeValueV1(Inspector & inspector) const
{
    std::vector<vespalib::string> payload;
    int64_t lastChanged = static_cast<int64_t>(inspector["lastChanged"].asDouble());
    Inspector & s(inspector["lines"]);
    for (size_t i = 0; i < s.children(); i++) {
        payload.push_back(s[i].asString().make_string());
    }
    return Value(lastChanged, ConfigValue(payload, calculateContentMd5(payload)));
}

namespace {

class FixedPayload : public protocol::Payload {
public:
    const Inspector & getSlimePayload() const
    {
        return _data.get();
    }

    Slime & getData() {
        return _data;
    }
private:
    Slime _data;
};

}

std::pair<int64_t, ConfigValue>
ConfigSnapshot::deserializeValueV2(Inspector & inspector) const
{
    int64_t lastChanged = static_cast<int64_t>(inspector["lastChanged"].asDouble());
    vespalib::string md5(inspector["md5"].asString().make_string());
    FixedPayload * payload = new FixedPayload();
    PayloadPtr data(payload);
    copySlimeObject(inspector["payload"], payload->getData().setObject());
    return Value(lastChanged, ConfigValue(data, md5));
}

}
