// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fastos/fastos.h>
#include "eval_spec.h"
#include <vespa/vespalib/util/stringfmt.h>
#include <vespa/vespalib/util/string_hash.h>
#include <cmath>

namespace vespalib {
namespace eval {
namespace test {

const double my_nan = std::numeric_limits<double>::quiet_NaN();
const double my_inf = std::numeric_limits<double>::infinity();

vespalib::string
EvalSpec::EvalTest::as_string(const std::vector<vespalib::string> &param_names,
                              const std::vector<double> &param_values,
                              const vespalib::string &expression)
{
    assert(param_values.size() == param_names.size());
    vespalib::string str;
    str += "f(";
    for (size_t i = 0; i < param_names.size(); ++i) {
        if (i > 0) {
            str += ", ";
        }
        str += param_names[i];
        str += "=";
        str += make_string("%g", param_values[i]);
    }
    str += ") { ";
    str += expression;
    str += " }";
    return str;
}

bool
EvalSpec::EvalTest::is_same(double expected, double actual) {
    if (std::isnan(expected)) {
        return std::isnan(actual);
    }
    return (actual == expected);
}

void
EvalSpec::add_terminal_cases() {
    add_expression({}, "(-100)").add_case({}, -100.0);
    add_expression({}, "(-10)").add_case({}, -10.0);
    add_expression({}, "(-5.75)").add_case({}, -5.75);
    add_expression({}, "(-4.5)").add_case({}, -4.5);
    add_expression({}, "(-3)").add_case({}, -3.0);
    add_expression({}, "(-2)").add_case({}, -2.0);
    add_expression({}, "(-0.1)").add_case({}, -0.1);
    add_expression({}, "0").add_case({}, 0.0);
    add_expression({}, "0.1").add_case({}, 0.1);
    add_expression({}, "2").add_case({}, 2.0);
    add_expression({}, "3").add_case({}, 3.0);
    add_expression({}, "4.5").add_case({}, 4.5);
    add_expression({}, "5.75").add_case({}, 5.75);
    add_expression({}, "10").add_case({}, 10.0);
    add_expression({}, "100").add_case({}, 100.0);
    add_rule({"a", -5.0, 5.0}, "a", [](double a){ return a; });
    add_expression({}, "[]").add_case({}, 0.0);
    add_expression({}, "[1]").add_case({}, 1.0);
    add_expression({}, "[1,2]").add_case({}, 2.0);    
    add_expression({}, "[1,2,3]").add_case({}, 3.0);
    add_expression({}, "[3,2,1]").add_case({}, 3.0);
    add_expression({}, "[1,1,1,1,1]").add_case({}, 5.0);
    add_expression({}, "\"\"").add_case({}, vespalib::hash_code(""));
    add_expression({}, "\"foo\"").add_case({}, vespalib::hash_code("foo"));
    add_expression({}, "\"foo bar baz\"").add_case({}, vespalib::hash_code("foo bar baz"));
    add_expression({}, "\">\\\\\\\"\\t\\n\\r\\f<\"").add_case({}, vespalib::hash_code(">\\\"\t\n\r\f<"));
    add_expression({}, "\">\\x08\\x10\\x12\\x14<\"").add_case({}, vespalib::hash_code(">\x08\x10\x12\x14<"));
}

void
EvalSpec::add_arithmetic_cases() {
    add_rule({"a", -5.0, 5.0}, "(-a)", [](double a){ return -a; });
    add_rule({"a", -5.0, 5.0}, {"b", -5.0, 5.0}, "(a+b)", [](double a, double b){ return (a + b); });
    add_rule({"a", -5.0, 5.0}, {"b", -5.0, 5.0}, "(a-b)", [](double a, double b){ return (a - b); });
    add_rule({"a", -5.0, 5.0}, {"b", -5.0, 5.0}, "(a*b)", [](double a, double b){ return (a * b); });
    add_rule({"a", -5.0, 5.0}, {"b", -5.0, 5.0}, "(a/b)", [](double a, double b){ return (a / b); });
    add_rule({"a", -5.0, 5.0}, {"b", -5.0, 5.0}, "(a^b)", [](double a, double b){ return pow(a,b); });
    add_expression({"a", "b", "c", "d"}, "(((a+1)*(b-1))/((c+1)/(d-1)))")
        .add_case({0.0, 2.0, 0.0, 2.0}, 1.0)
        .add_case({1.0, 3.0, 0.0, 2.0}, 4.0)
        .add_case({1.0, 3.0, 1.0, 2.0}, 2.0)
        .add_case({1.0, 3.0, 1.0, 5.0}, 8.0);
}

void
EvalSpec::add_function_call_cases() {
    add_rule({"a", -1.0, 1.0}, "cos(a)", [](double a){ return cos(a); });
    add_rule({"a", -1.0, 1.0}, "sin(a)", [](double a){ return sin(a); });
    add_rule({"a", -1.0, 1.0}, "tan(a)", [](double a){ return tan(a); });
    add_rule({"a", -1.0, 1.0}, "cosh(a)", [](double a){ return cosh(a); });
    add_rule({"a", -1.0, 1.0}, "sinh(a)", [](double a){ return sinh(a); });
    add_rule({"a", -1.0, 1.0}, "tanh(a)", [](double a){ return tanh(a); });
    add_rule({"a", -1.0, 1.0}, "acos(a)", [](double a){ return acos(a); });
    add_rule({"a", -1.0, 1.0}, "asin(a)", [](double a){ return asin(a); });
    add_rule({"a", -1.0, 1.0}, "atan(a)", [](double a){ return atan(a); });
    add_rule({"a", -1.0, 1.0}, "exp(a)", [](double a){ return exp(a); });
    add_rule({"a", -1.0, 1.0}, "log10(a)", [](double a){ return log10(a); });
    add_rule({"a", -1.0, 1.0}, "log(a)", [](double a){ return log(a); });
    add_rule({"a", -1.0, 1.0}, "sqrt(a)", [](double a){ return sqrt(a); });
    add_rule({"a", -1.0, 1.0}, "ceil(a)", [](double a){ return ceil(a); });
    add_rule({"a", -1.0, 1.0}, "fabs(a)", [](double a){ return fabs(a); });
    add_rule({"a", -1.0, 1.0}, "floor(a)", [](double a){ return floor(a); });
    add_expression({"a"}, "isNan(a)")
        .add_case({-1.0}, 0.0).add_case({-0.5}, 0.0).add_case({0.0}, 0.0).add_case({0.5}, 0.0).add_case({1.0}, 0.0)
        .add_case({my_nan}, 1.0).add_case({my_inf}, 0.0).add_case({-my_inf}, 0.0);
    add_rule({"a", -1.0, 1.0}, {"b", -1.0, 1.0}, "atan2(a,b)", [](double a, double b){ return atan2(a, b); });
    add_rule({"a", -1.0, 1.0}, {"b", -1.0, 1.0}, "ldexp(a,b)", [](double a, double b){ return ldexp(a, b); });
    add_rule({"a", -1.0, 1.0}, {"b", -1.0, 1.0}, "pow(a,b)", [](double a, double b){ return pow(a, b); });
    add_rule({"a", -1.0, 1.0}, {"b", -1.0, 1.0}, "fmod(a,b)", [](double a, double b){ return fmod(a, b); });
    add_rule({"a", -1.0, 1.0}, {"b", -1.0, 1.0}, "min(a,b)", [](double a, double b){ return std::min(a, b); });
    add_rule({"a", -1.0, 1.0}, {"b", -1.0, 1.0}, "max(a,b)", [](double a, double b){ return std::max(a, b); });
}

void
EvalSpec::add_comparison_cases() {
    add_expression({"a", "b"}, "(a==b)")
        .add_case({my_nan, 2.0},      0.0)
        .add_case({2.0, my_nan},      0.0)
        .add_case({my_nan, my_nan},   0.0)
        .add_case({1.0, 2.0},         0.0)
        .add_case({2.0 - 1e-10, 2.0}, 0.0)
        .add_case({2.0, 2.0},         1.0)
        .add_case({2.0 + 1e-10, 2.0}, 0.0)
        .add_case({3.0, 2.0},         0.0);

    add_expression({"a", "b"}, "(a!=b)")
        .add_case({my_nan, 2.0},      1.0)
        .add_case({2.0, my_nan},      1.0)
        .add_case({my_nan, my_nan},   1.0)
        .add_case({1.0, 2.0},         1.0)
        .add_case({2.0 - 1e-10, 2.0}, 1.0)
        .add_case({2.0, 2.0},         0.0)
        .add_case({2.0 + 1e-10, 2.0}, 1.0)
        .add_case({3.0, 2.0},         1.0);

    add_expression({"a", "b"}, "(a~=b)")
        .add_case({my_nan, 2.0},      0.0)
        .add_case({2.0, my_nan},      0.0)
        .add_case({my_nan, my_nan},   0.0)
        .add_case({0.5, 0.5},         1.0)
        .add_case({1.0, 2.0},         0.0)
        .add_case({2.0, 2.0},         1.0)
        .add_case({3.0, 2.0},         0.0)
        .add_case({0.5 - 1e-10, 0.5}, 1.0)
        .add_case({0.5, 0.5 - 1e-10}, 1.0)
        .add_case({2.0 - 1e-10, 2.0}, 1.0)
        .add_case({2.0, 2.0 - 1e-10}, 1.0)
        .add_case({0.5 + 1e-10, 0.5}, 1.0)
        .add_case({0.5, 0.5 + 1e-10}, 1.0)
        .add_case({2.0 + 1e-10, 2.0}, 1.0)
        .add_case({2.0, 2.0 + 1e-10}, 1.0)
        .add_case({0.5 - 2e-7, 0.5},  0.0)
        .add_case({0.5, 0.5 - 2e-7},  0.0)
        .add_case({2.0 - 5e-7, 2.0},  0.0)
        .add_case({2.0, 2.0 - 5e-7},  0.0)
        .add_case({0.5 + 2e-7, 0.5},  0.0)
        .add_case({0.5, 0.5 + 2e-7},  0.0)
        .add_case({2.0 + 5e-7, 2.0},  0.0)
        .add_case({2.0, 2.0 + 5e-7},  0.0);

    add_expression({"a", "b"}, "(a<b)")
        .add_case({my_nan, 2.0},      0.0)
        .add_case({2.0, my_nan},      0.0)
        .add_case({my_nan, my_nan},   0.0)
        .add_case({1.0, 2.0},         1.0)
        .add_case({2.0 - 1e-10, 2.0}, 1.0)
        .add_case({2.0, 2.0},         0.0)
        .add_case({2.0 + 1e-10, 2.0}, 0.0)
        .add_case({3.0, 2.0},         0.0);

    add_expression({"a", "b"}, "(a<=b)")
        .add_case({my_nan, 2.0},      0.0)
        .add_case({2.0, my_nan},      0.0)
        .add_case({my_nan, my_nan},   0.0)
        .add_case({1.0, 2.0},         1.0)
        .add_case({2.0 - 1e-10, 2.0}, 1.0)
        .add_case({2.0, 2.0},         1.0)
        .add_case({2.0 + 1e-10, 2.0}, 0.0)
        .add_case({3.0, 2.0},         0.0);

    add_expression({"a", "b"}, "(a>b)")
        .add_case({my_nan, 2.0},      0.0)
        .add_case({2.0, my_nan},      0.0)
        .add_case({my_nan, my_nan},   0.0)
        .add_case({1.0, 2.0},         0.0)
        .add_case({2.0 - 1e-10, 2.0}, 0.0)
        .add_case({2.0, 2.0},         0.0)
        .add_case({2.0 + 1e-10, 2.0}, 1.0)
        .add_case({3.0, 2.0},         1.0);

    add_expression({"a", "b"}, "(a>=b)")
        .add_case({my_nan, 2.0},      0.0)
        .add_case({2.0, my_nan},      0.0)
        .add_case({my_nan, my_nan},   0.0)
        .add_case({1.0, 2.0},         0.0)
        .add_case({2.0 - 1e-10, 2.0}, 0.0)
        .add_case({2.0, 2.0},         1.0)
        .add_case({2.0 + 1e-10, 2.0}, 1.0)
        .add_case({3.0, 2.0},         1.0);
}

void
EvalSpec::add_set_membership_cases()
{
    add_expression({"a"}, "(a in [])")
        .add_case({0.0}, 0.0)
        .add_case({1.0}, 0.0)
        .add_case({2.0}, 0.0);

    add_expression({"a"}, "(a in [[]])")
        .add_case({0.0}, 1.0)
        .add_case({1.0}, 0.0)
        .add_case({2.0}, 0.0);

    add_expression({"a"}, "(a in [[[]]])")
        .add_case({0.0}, 0.0)
        .add_case({1.0}, 1.0)
        .add_case({2.0}, 0.0);

    add_expression({"a", "b"}, "(a in b)")
        .add_case({my_nan, 2.0},      0.0)
        .add_case({2.0, my_nan},      0.0)
        .add_case({my_nan, my_nan},   0.0)
        .add_case({1.0, 2.0},         0.0)
        .add_case({2.0 - 1e-10, 2.0}, 0.0)
        .add_case({2.0, 2.0},         1.0)
        .add_case({2.0 + 1e-10, 2.0}, 0.0)
        .add_case({3.0, 2.0},         0.0);

    add_expression({"a", "b"}, "(a in [b])")
        .add_case({my_nan, 2.0},      0.0)
        .add_case({2.0, my_nan},      0.0)
        .add_case({my_nan, my_nan},   0.0)
        .add_case({1.0, 2.0},         0.0)
        .add_case({2.0 - 1e-10, 2.0}, 0.0)
        .add_case({2.0, 2.0},         1.0)
        .add_case({2.0 + 1e-10, 2.0}, 0.0)
        .add_case({3.0, 2.0},         0.0);

    add_expression({"a", "b"}, "(a in [[b]])")
        .add_case({1.0, 2.0},         1.0)
        .add_case({2.0, 2.0},         0.0);

    add_expression({"a", "b", "c", "d"}, "(a in [b,c,d])")
        .add_case({0.0, 10.0, 20.0, 30.0}, 0.0)
        .add_case({3.0, 10.0, 20.0, 30.0}, 0.0)
        .add_case({10.0, 10.0, 20.0, 30.0}, 1.0)
        .add_case({20.0, 10.0, 20.0, 30.0}, 1.0)
        .add_case({30.0, 10.0, 20.0, 30.0}, 1.0)
        .add_case({10.0, 30.0, 20.0, 10.0}, 1.0)
        .add_case({20.0, 30.0, 20.0, 10.0}, 1.0)
        .add_case({30.0, 30.0, 20.0, 10.0}, 1.0);
}

void
EvalSpec::add_boolean_cases() {
    add_expression({"a"}, "(!a)")
        .add_cases({my_nan, -my_inf, -123.0, -1.0, -0.001, 0.0, 0.001, 1.0, 123.0, my_inf},
                   [](double a)->double{ return !bool(a); });

    add_expression({"a"}, "(!(!a))")
        .add_cases({my_nan, -my_inf, -123.0, -1.0, -0.001, 0.0, 0.001, 1.0, 123.0, my_inf},
                   [](double a)->double{ return bool(a); });

    add_expression({"a", "b"}, "(a&&b)")
        .add_cases({my_nan, -my_inf, -123.0, -1.0, -0.001, 0.0, 0.001, 1.0, 123.0, my_inf},
                   {my_nan, -my_inf, -123.0, -1.0, -0.001, 0.0, 0.001, 1.0, 123.0, my_inf},
                   [](double a, double b)->double{ return (bool(a) && bool(b)); });

    add_expression({"a", "b"}, "(a||b)")
        .add_cases({my_nan, -my_inf, -123.0, -1.0, -0.001, 0.0, 0.001, 1.0, 123.0, my_inf},
                   {my_nan, -my_inf, -123.0, -1.0, -0.001, 0.0, 0.001, 1.0, 123.0, my_inf},
                   [](double a, double b)->double{ return (bool(a) || bool(b)); });
}

void
EvalSpec::add_if_cases() {
    add_expression({"a"}, "if(a,1,0)")
        .add_cases({my_nan, -my_inf, -123.0, -1.0, -0.001, 0.0, 0.001, 1.0, 123.0, my_inf},
                   [](double a){ if (a) { return 1.0; } else { return 0.0; } });

    add_expression({"a", "b"}, "if(a,if(b,1,2),if(b,3,4))")
        .add_cases({my_nan, -my_inf, -123.0, -1.0, -0.001, 0.0, 0.001, 1.0, 123.0, my_inf},
                   {my_nan, -my_inf, -123.0, -1.0, -0.001, 0.0, 0.001, 1.0, 123.0, my_inf},
                   [](double a, double b)
                   {
                       if (a) {
                           if (b) {
                               return 1.0;
                           } else {
                               return 2.0;
                           }
                       } else {
                           if (b) {
                               return 3.0;
                           } else {
                               return 4.0;
                           }
                       }
                   });
    add_expression({"a"}, "if(a,1,0,0.25)")
        .add_cases({my_nan, -my_inf, -123.0, -1.0, -0.001, 0.0, 0.001, 1.0, 123.0, my_inf},
                   [](double a){ if (a) { return 1.0; } else { return 0.0; } });
    add_expression({"a"}, "if(a,1,0,0.75)")
        .add_cases({my_nan, -my_inf, -123.0, -1.0, -0.001, 0.0, 0.001, 1.0, 123.0, my_inf},
                   [](double a){ if (a) { return 1.0; } else { return 0.0; } });
}

void
EvalSpec::add_let_cases() {
    add_rule({"a", -10.0, 10.0}, "let(tmp,(a+1),(tmp*tmp))", [](double a){ return (a+1)*(a+1); });
    add_rule({"a", -10.0, 10.0}, "let(a,(a+1),((a*a)*a))", [](double a){ return (a+1)*(a+1)*(a+1); });
    add_rule({"a", -10.0, 10.0}, "let(a,(a+1),let(a,(a+1),let(b,2,let(a,(a+1),(a+b)))))", [](double a) { return (a + 5.0); });
    add_rule({"a", -10.0, 10.0}, {"b", -10.0, 10.0}, "let(a,(a*b),let(b,(b+a),(a*b)))",
             [](double a, double b)
             {
                 double let_a = (a * b);
                 double let_b = (b + let_a);
                 return (let_a * let_b);
             });
}

void
EvalSpec::add_complex_cases() {
    add_expression({"a", "b"}, "((a<3)||b)")
        .add_cases({2.0, 4.0}, {0.0, 0.5, 1.0},
                   [](double a, double b)->double{ return ((a < 3) || bool(b)); });

    add_expression({"a", "b"}, "((a<3)==b)")
        .add_cases({2.0, 4.0}, {0.0, 0.5, 1.0},
                   [](double a, double b)->double{ return (double((a < 3)) == b); });

    add_expression({"a"}, "(!(-a))")
        .add_cases({my_nan, -my_inf, -123.0, -1.0, -0.001, 0.0, 0.001, 1.0, 123.0, my_inf},
                   [](double a)->double{ return !bool(-a); });

    add_expression({"a"}, "(-(!a))")
        .add_cases({my_nan, -my_inf, -123.0, -1.0, -0.001, 0.0, 0.001, 1.0, 123.0, my_inf},
                   [](double a)->double{ return -double(!bool(a)); });
}

} // namespace vespalib::eval::test
} // namespace vespalib::eval
} // namespace vespalib
