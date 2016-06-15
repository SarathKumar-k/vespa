// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/searchlib/attribute/attributevector.h>
#include <vespa/searchlib/attribute/integerbase.h>
#include <cmath>

namespace search {

template <typename T>
inline bool myIsNan(T v) { (void)v; return false; }

template <>
inline bool
myIsNan<float>(float v)
{
    return std::isnan(v);
}

template <>
inline bool
myIsNan<double>(double v)
{
    return std::isnan(v);
}

template<typename T>
bool
AttributeVector::adjustWeight(ChangeVectorT< ChangeTemplate<T> > & changes, DocId doc, const T & v,
                              const ArithmeticValueUpdate & wd)
{
    bool retval(hasWeightedSetType() && (doc < getNumDocs()));
    if (retval) {
        size_t oldSz(changes.size());
        ArithmeticValueUpdate::Operator op(wd.getOperator());
        int32_t w(static_cast<int32_t>(wd.getOperand()));
        if (op == ArithmeticValueUpdate::Add) {
            changes.push_back(ChangeTemplate<T>(ChangeBase::INCREASEWEIGHT, doc, v, w));
        } else if (op == ArithmeticValueUpdate::Sub) {
            changes.push_back(ChangeTemplate<T>(ChangeBase::INCREASEWEIGHT, doc, v, -w));
        } else if (op == ArithmeticValueUpdate::Mul) {
            changes.push_back(ChangeTemplate<T>(ChangeBase::MULWEIGHT, doc, v, w));
        } else if (op == ArithmeticValueUpdate::Div) {
            if (w != 0) {
                changes.push_back(ChangeTemplate<T>(ChangeBase::DIVWEIGHT, doc, v, w));
            } else {
                divideByZeroWarning();
            }
        } else {
            retval = false;
        }
        if (retval) {
            const size_t diff = changes.size() - oldSz;
            _status.incNonIdempotentUpdates(diff);
            _status.incUpdates(diff);
        }
    }
    return retval;
}

template<typename T>
bool
AttributeVector::applyArithmetic(ChangeVectorT< ChangeTemplate<T> > & changes, DocId doc, const T & v,
                                 const ArithmeticValueUpdate & arithm)
{
    (void) v;
    bool retval(!hasMultiValue() && (doc < getNumDocs()));
    if (retval) {
        size_t oldSz(changes.size());
        ArithmeticValueUpdate::Operator op(arithm.getOperator());
        double aop = arithm.getOperand();
        if (op == ArithmeticValueUpdate::Add) {
            changes.push_back(ChangeTemplate<T>(ChangeBase::ADD, doc, 0, 0));
        } else if (op == ArithmeticValueUpdate::Sub) {
            changes.push_back(ChangeTemplate<T>(ChangeBase::SUB, doc, 0, 0));
        } else if (op == ArithmeticValueUpdate::Mul) {
            changes.push_back(ChangeTemplate<T>(ChangeBase::MUL, doc, 0, 0));
        } else if (op == ArithmeticValueUpdate::Div) {
            if (this->getClass().inherits(IntegerAttribute::classId) && aop == 0) {
                divideByZeroWarning();
            } else {
                changes.push_back(ChangeTemplate<T>(ChangeBase::DIV, doc, 0, 0));
            }
        } else {
            retval = false;
        }
        if (retval) {
            const size_t diff = changes.size() - oldSz;
            _status.incNonIdempotentUpdates(diff);
            _status.incUpdates(diff);
            if (diff > 0) {
                changes.back()._arithOperand = aop;
            }
        }
    }
    return retval;
}

template<typename T>
bool AttributeVector::clearDoc(ChangeVectorT< ChangeTemplate<T> > & changes, DocId doc) {
    bool retval(doc < getNumDocs());
    if (retval) {
        changes.push_back(ChangeTemplate<T>(ChangeBase::CLEARDOC, doc, T()));
        _status.incUpdates();
        updateUncommittedDocIdLimit(doc);
    }
    return retval;
}

template<typename T>
bool AttributeVector::update(ChangeVectorT< ChangeTemplate<T> > & changes, DocId doc, const T & v) {
    bool retval(doc < getNumDocs());
    if (retval) {
        if (hasMultiValue()) {
            clearDoc(doc);
            retval = append(changes, doc, v, 1, false);
        } else {
            changes.push_back(ChangeTemplate<T>(ChangeBase::UPDATE, doc, v));
            _status.incUpdates();
            updateUncommittedDocIdLimit(doc);
        }
    }
    return retval;
}

template<typename T>
bool AttributeVector::append(ChangeVectorT< ChangeTemplate<T> > & changes, DocId doc, const T & v, int32_t w, bool doCount) {
    bool retval(hasMultiValue() && (doc < getNumDocs()));
    if (retval) {
        changes.push_back(ChangeTemplate<T>(ChangeBase::APPEND, doc, v, w));
        _status.incUpdates();
        updateUncommittedDocIdLimit(doc);
        if ( hasArrayType() && doCount) {
            _status.incNonIdempotentUpdates();
        }
    }
    return retval;
}

template<typename T, typename Accessor>
bool AttributeVector::append(ChangeVectorT< ChangeTemplate<T> > & changes, DocId doc, Accessor & ac) {
    bool retval(hasMultiValue() && (doc < getNumDocs()));
    if (retval) {
        changes.push_back(doc, ac);
        _status.incUpdates(ac.size());
        updateUncommittedDocIdLimit(doc);
        if ( hasArrayType() ) {
            _status.incNonIdempotentUpdates(ac.size());
        }
    }
    return retval;
}

template<typename T>
bool AttributeVector::remove(ChangeVectorT< ChangeTemplate<T> > & changes, DocId doc, const T & v, int32_t w) {
    bool retval(hasMultiValue() && (doc < getNumDocs()));
    if (retval) {
        changes.push_back(ChangeTemplate<T>(ChangeBase::REMOVE, doc, v, w));
        _status.incUpdates();
        updateUncommittedDocIdLimit(doc);
        if ( hasArrayType() ) {
            _status.incNonIdempotentUpdates();
        }
    }
    return retval;
}

}

