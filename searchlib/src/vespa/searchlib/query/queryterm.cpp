// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <math.h>
#include <limits>
#include <vespa/searchlib/query/queryterm.h>
#include <vespa/vespalib/objects/visit.h>
#include <vespa/vespalib/text/utf8.h>
#include <cxxabi.h>

namespace {

class CharInfo {
public:
    CharInfo();
    uint8_t get(uint8_t c) const { return _charInfo[c]; }
private:
    uint8_t _charInfo[256];
};

CharInfo::CharInfo()
{
    // XXX: Should refactor to reduce number of magic constants.
    memset(_charInfo, 0x01, 128); // All 7 bits are ascii7bit
    memset(_charInfo+128, 0x00, 128); // The rest are not.
    memset(_charInfo + '0', 0x07, 10);
    _charInfo[uint8_t('-')] = 0x07;
    _charInfo[uint8_t('<')] = 0x07;
    _charInfo[uint8_t('>')] = 0x07;
    _charInfo[uint8_t(';')] = 0x07;
    _charInfo[uint8_t('[')] = 0x07;
    _charInfo[uint8_t(']')] = 0x07;

    _charInfo[uint8_t('.')] = 0x05;
    _charInfo[uint8_t('+')] = 0x05;
    _charInfo[uint8_t('e')] = 0x05;
    _charInfo[uint8_t('E')] = 0x05;
}

static CharInfo _G_charTable;


template <typename N>
bool isValidInteger(int64_t value)
{
    return value >= std::numeric_limits<N>::min() && value <= std::numeric_limits<N>::max();
}

}

namespace search
{

IMPLEMENT_IDENTIFIABLE_NS(search, QueryTerm, QueryNode);

QueryTermBase::QueryTermBase() :
    QueryTermSimple(),
    _cachedTermLen(0),
    _termUCS4()
{
    _termUCS4.push_back(0);
}

QueryTermBase::QueryTermBase(const string & termS, SearchTerm type) :
    QueryTermSimple(termS, type),
    _cachedTermLen(0),
    _termUCS4()
{
    _termUCS4.reserve(termS.size() + 1);
    vespalib::Utf8Reader r(termS);
    while (r.hasMore()) {
        ucs4_t u = r.getChar();
        _termUCS4.push_back(u);
    }
    _termUCS4.push_back(0);
    _cachedTermLen = _termUCS4.size() - 1;
}

QueryTerm::QueryTerm() :
    QueryTermBase(),
    _index(),
    _encoding(),
    _result(),
    _hitList(),
    _weight(100),
    _uniqueId(0),
    _fieldInfo(32)
{
}

void
QueryTermSimple::visitMembers(vespalib::ObjectVisitor & visitor) const
{
    visit(visitor, "term", _term);
    visit(visitor, "type", _type);
}

template <typename N>
QueryTermSimple::RangeResult<N>
QueryTermSimple::getFloatRange() const
{
    double lowRaw, highRaw;
    bool valid = getAsDoubleTerm(lowRaw, highRaw);
    RangeResult<N> res;
    res.valid = valid;
    if (!valid) {
        res.low = std::numeric_limits<N>::max();
        res.high = - std::numeric_limits<N>::max();
        res.adjusted = true;
    } else {
        res.low = lowRaw;
        res.high = highRaw;
    }
    return res;
}

namespace {

bool isRepresentableByInt64(double d) {
    return    (d > double(std::numeric_limits<int64_t>::min()))
           && (d < double(std::numeric_limits<int64_t>::max()));
}

}

bool
QueryTermSimple::getRangeInternal(int64_t & low, int64_t & high) const
{
    bool valid = getAsIntegerTerm(low, high);
    if ( ! valid ) {
        double l(0), h(0);
        valid = getAsDoubleTerm(l, h);
        if (valid) {
            if ((l == h) && isRepresentableByInt64(l)) {
                low = high = round(l);
            } else {
                if (l > double(std::numeric_limits<int64_t>::min())) {
                    if (l < double(std::numeric_limits<int64_t>::max())) {
                        low = ceil(l);
                    } else {
                        low = std::numeric_limits<int64_t>::max();
                    }
                }
                if (h < double(std::numeric_limits<int64_t>::max())) {
                    if (h > double(std::numeric_limits<int64_t>::min())) {
                        high = floor(h);
                    } else {
                        high = std::numeric_limits<int64_t>::min();
                    }
                }
            }
        }
    }
    return valid;
}

template <typename N>
QueryTermSimple::RangeResult<N>
QueryTermSimple::getIntegerRange() const
{
    int64_t lowRaw, highRaw;
    bool valid = getRangeInternal(lowRaw, highRaw);
    RangeResult<N> res;
    res.valid = valid;
    if (valid) {
        bool validLow = isValidInteger<N>(lowRaw);
        if (validLow) {
            res.low = lowRaw;
        } else {
            res.low = (lowRaw < static_cast<int64_t>(std::numeric_limits<N>::min()) ?
                       std::numeric_limits<N>::min() : std::numeric_limits<N>::max());
            res.adjusted = true;
        }
        bool validHigh = isValidInteger<N>(highRaw);
        if (validHigh) {
            res.high = highRaw;
        } else {
            res.high = (highRaw > static_cast<int64_t>(std::numeric_limits<N>::max()) ?
                        std::numeric_limits<N>::max() : std::numeric_limits<N>::min());
            res.adjusted = true;
        }
    } else {
        res.low = std::numeric_limits<N>::max();
        res.high = std::numeric_limits<N>::min();
        res.adjusted = true;
    }
    return res;
}

template <>
QueryTermSimple::RangeResult<float>
QueryTermSimple::getRange() const
{
    return getFloatRange<float>();
}

template <>
QueryTermSimple::RangeResult<double>
QueryTermSimple::getRange() const
{
    return getFloatRange<double>();
}

template <>
QueryTermSimple::RangeResult<int8_t>
QueryTermSimple::getRange() const
{
    return getIntegerRange<int8_t>();
}

template <>
QueryTermSimple::RangeResult<int16_t>
QueryTermSimple::getRange() const
{
    return getIntegerRange<int16_t>();
}

template <>
QueryTermSimple::RangeResult<int32_t>
QueryTermSimple::getRange() const
{
    return getIntegerRange<int32_t>();
}

template <>
QueryTermSimple::RangeResult<int64_t>
QueryTermSimple::getRange() const
{
    return getIntegerRange<int64_t>();
}

void
QueryTermBase::visitMembers(vespalib::ObjectVisitor & visitor) const
{
    QueryTermSimple::visitMembers(visitor);
    visit(visitor, "termlength", _cachedTermLen);
}

void
QueryTerm::visitMembers(vespalib::ObjectVisitor & visitor) const
{
    QueryTermBase::visitMembers(visitor);
    visit(visitor, "encoding.isBase10Integer", _encoding.isBase10Integer());
    visit(visitor, "encoding.isFloat", _encoding.isFloat());
    visit(visitor, "encoding.isAscii7Bit", _encoding.isAscii7Bit());
    visit(visitor, "index", _index);
    visit(visitor, "weight", _weight.percent());
    visit(visitor, "uniqueid", _uniqueId);
}


QueryTerm::QueryTerm(const QueryNodeResultBase & org, const string & termS, const string & indexS, SearchTerm type) :
    QueryTermBase(termS, type),
    _index(indexS),
    _encoding(0x01),
    _result(org),
    _hitList(),
    _weight(100),
    _uniqueId(0),
    _fieldInfo(32)
{
    if (!termS.empty()) {
        uint8_t enc(0xff);
        for (size_t i(0), m(termS.size()); i < m; i++) {
            enc &= _G_charTable.get(termS[i]);
        }
        _encoding = enc;
    }
}

void QueryTerm::getPhrases(QueryNodeRefList & tl)            { (void) tl; }
void QueryTerm::getPhrases(ConstQueryNodeRefList & tl) const { (void) tl; }
void QueryTerm::getLeafs(QueryTermList & tl)                 { tl.push_back(this); }
void QueryTerm::getLeafs(ConstQueryTermList & tl)      const { tl.push_back(this); }
bool QueryTerm::evaluate()                             const { return !_hitList.empty() && _result->evaluate(); }
void QueryTerm::reset()                                      { _hitList.clear(); _result->reset(); }
const HitList & QueryTerm::evaluateHits(HitList & UNUSED_PARAM(hl)) const { return _hitList; }

void QueryTerm::resizeFieldId(size_t fieldNo)
{
    if (fieldNo >= _fieldInfo.size()) {
        _fieldInfo.resize(fieldNo + 1);
    }
}

void QueryTerm::add(unsigned pos, unsigned context, int32_t weight_)
{
    _hitList.emplace_back(pos, context, weight_);
}

template <int B>
struct IntDecoder {
    static int64_t fromstr(const char * v, char ** end) { return strtoll(v, end, B); }
    static int64_t nearestDownwd(int64_t n, int64_t min) { return (n > min ? n - 1 : n); }
    static int64_t nearestUpward(int64_t n, int64_t max) { return (n < max ? n + 1 : n); }
};

struct DoubleDecoder {
    static double fromstr(const char * v, char ** end) { return strtod(v, end); }
    static double nearestDownwd(double n, double min) { return nextafterf(n, min); }
    static double nearestUpward(double n, double max) { return nextafterf(n, max); }
};

bool QueryTermSimple::getAsIntegerTerm(int64_t & lower, int64_t & upper) const
{
    lower = std::numeric_limits<int64_t>::min();
    upper = std::numeric_limits<int64_t>::max();
    return getAsNumericTerm(lower, upper, IntDecoder<10>());
}

bool QueryTermSimple::getAsDoubleTerm(double & lower, double & upper) const
{
    lower = - std::numeric_limits<double>::max();
    upper =   std::numeric_limits<double>::max();
    return getAsNumericTerm(lower, upper, DoubleDecoder());
}

QueryTermSimple::QueryTermSimple() :
    _type(WORD),
    _term(),
    _diversityAttribute(),
    _rangeLimit(0),
    _maxPerGroup(0),
    _diversityCutoffGroups(std::numeric_limits<uint32_t>::max()),
    _diversityCutoffStrict(false),
    _valid(true)
{
}

namespace {

bool isFullRange(const vespalib::stringref & s) {
    const size_t sz(s.size());
    return (sz >= 3u) &&
           (s[0] == '<' || s[0] == '[') &&
           (s[sz-1] == '>' || s[sz-1] == ']');
}

}

QueryTermSimple::QueryTermSimple(const string & term_, SearchTerm type) :
    _type(type),
    _term(term_),
    _diversityAttribute(),
    _rangeLimit(0),
    _maxPerGroup(0),
    _diversityCutoffGroups(std::numeric_limits<uint32_t>::max()),
    _diversityCutoffStrict(false),
    _valid(true)
{
    if (isFullRange(_term)) {
        stringref rest(_term.c_str() + 1, _term.size() - 2);
        std::vector<stringref> parts;
        parts.reserve(5);
        while (! rest.empty() ) {
            size_t pos(rest.find(';'));
            if (pos != vespalib::string::npos) {
                parts.push_back(rest.substr(0, pos));
                rest = rest.substr(pos + 1);
                if (rest.empty()) {
                    parts.push_back(rest);
                }
            } else {
                parts.push_back(rest);
                rest = stringref();
            }
        }
        _valid = parts.size() >= 2;
        if (parts.size() >= 3) {
            _rangeLimit = strtol(parts[2].c_str(), NULL, 0);
            if (parts.size() > 3) {
                _valid = parts.size() >= 5;
                if (_valid) {
                    _diversityAttribute = parts[3];
                    _maxPerGroup = strtoul(parts[4].c_str(), NULL, 0);
                    if ((_maxPerGroup > 0) && (parts.size() > 5)) {
                        char *err = nullptr;
                        size_t cutoffGroups = strtoul(parts[5].c_str(), &err, 0);
                        if ((err == nullptr) || (size_t(err - parts[5].c_str()) == parts[5].size())) {
                            _diversityCutoffGroups = cutoffGroups;
                        }
                        if (parts.size() > 6) {
                            _diversityCutoffStrict = (parts[6] == "strict");
                            _valid = (parts.size() == 7);
                        }
                    }
                }
            }
        }
    }
} 

template <typename T, typename D>
bool
QueryTermSimple::getAsNumericTerm(T & lower, T & upper, D d) const
{
    bool valid(empty());
    size_t sz(_term.size());
    if (sz) {
        char *err(NULL);
        T low(lower);
        T high(upper);
        const char * q = _term.c_str();
        const char first(q[0]);
        const char last(q[sz-1]);
        q += ((first == '<') || (first == '>') || (first == '[')) ? 1 : 0;
        T ll = d.fromstr(q, &err);
        valid = isValid() && ((*err == 0) || (*err == ';'));
        if (valid) {
            if (first == '<' && (*err == 0)) {
                high = d.nearestDownwd(ll, lower);
            } else if (first == '>' && (*err == 0)) {
                low = d.nearestUpward(ll, upper);
            } else if ((first == '[') || (first == '<')) {
                if (q != err) {
                    low = (first == '[') ? ll : d.nearestUpward(ll, upper);
                }
                q = err + 1;
                T hh = d.fromstr(q, &err);
                bool hasUpperLimit(q != err);
                if (*err == ';') {
                    err = const_cast<char *>(_term.end() - 1);
                }
                valid = (*err == last) && ((last == ']') || (last == '>'));
                if (hasUpperLimit) {
                    high = (last == ']') ? hh : d.nearestDownwd(hh, lower);
                }
            } else {
                low = high = ll;
            }
        }
        if (valid) {
            lower = low;
            upper = high;
        }
    }
    return valid;
}

vespalib::string
QueryTermSimple::getClassName() const
{
    vespalib::string name(typeid(*this).name());
    int status = 0;
    size_t size = 0;
    char *unmangled = abi::__cxa_demangle(name.c_str(), 0, &size, &status);
    vespalib::string result(unmangled);
    free(unmangled);
    return result;
}

}

void visit(vespalib::ObjectVisitor &self, const vespalib::string &name,
           const search::QueryTermSimple *obj)
{
    if (obj != 0) {
        self.openStruct(name, obj->getClassName());
        obj->visitMembers(self);
        self.closeStruct();
    } else {
        self.visitNull(name);
    }
}

void visit(vespalib::ObjectVisitor &self, const vespalib::string &name,
           const search::QueryTermSimple &obj)
{
    visit(self, name, &obj);
}
