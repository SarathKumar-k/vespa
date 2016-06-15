// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "cursor.h"
#include "inspector.h"
#include "type.h"
#include "inserter.h"
#include "inject.h"

namespace vespalib {

class Slime;

namespace slime {

struct Memory;
class Type;
class Symbol;

namespace convenience {

using ::vespalib::Slime;

using ::vespalib::slime::Symbol;
using ::vespalib::slime::Memory;
using ::vespalib::slime::Cursor;
using ::vespalib::slime::Inspector;
using ::vespalib::slime::Type;

using ::vespalib::slime::Inserter;
using ::vespalib::slime::SlimeInserter;
using ::vespalib::slime::ArrayInserter;
using ::vespalib::slime::ObjectInserter;
using ::vespalib::slime::inject;

} // namespace vespalib::slime::convenience
} // namespace vespalib::slime
} // namespace vespalib

