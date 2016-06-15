#!/bin/sh
# Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

dir=`dirname $0`
. "$dir/create-base.sh"

cat <<EOF
#include <vespa/log/log.h>
LOG_SETUP(".$name");
#include <vespa/fastos/fastos.h>
#include "$name.h"

$ns_open

$class::$class()
{
}

$class::~$class()
{
}

$ns_close
EOF
