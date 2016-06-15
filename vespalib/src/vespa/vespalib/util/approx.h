// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <math.h>

namespace vespalib {

/**
 * Compare two double-precision floating-point numbers to see if they
 * are approximately equal.  We convert to floating-point and then
 * step 1 unit in the last place towards the other number.  This means the
 * two numbers must be equal to 23 bits precision.
 **/
inline bool approx_equal(double a, double b)
{
    if (a == b) return true;
    if (a > 1.0 || a < -1.0) {
        // This is in a way the simple case, but it's needed
        // anyway to handle numbers that are outside "float" range.
        double frac = b / a;
        float rounded = nextafterf(frac, 1.0);
        return (rounded == 1.0);
    }
    // in reality this may allow up to 2 bits difference
    // since we round to float and also call nextafterf
    float aa = (float) a;
    return (aa == nextafterf((float) b, aa));
}

} // namespace vespalib

