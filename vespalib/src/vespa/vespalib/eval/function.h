// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <cmath>
#include <memory>
#include <vespa/vespalib/stllike/string.h>
#include <vespa/vespalib/util/stringfmt.h>
#include <map>
#include "basic_nodes.h"
#include "delete_node.h"
#include "value.h"

namespace vespalib {
namespace eval {

/**
 * Interface used to perform custom symbol extraction. This is
 * typically used by the ranking framework to extend what will be
 * parsed as parameter names.
 **/
struct SymbolExtractor {
    virtual void extract_symbol(const char *pos_in, const char *end_in,
                                const char *&pos_out, vespalib::string &symbol_out) const = 0;
    virtual ~SymbolExtractor() {}
};

struct NodeVisitor;

/**
 * When you parse an expression you get a Function. It contains the
 * AST root and the names of all parameters. A function can only be
 * evaluated using the appropriate number of parameters.
 **/
class Function
{
private:
    nodes::Node_UP _root;
    std::vector<vespalib::string> _params;

public:
    Function() : _root(new nodes::Number(0.0)), _params() {}
    Function(nodes::Node_UP root_in, std::vector<vespalib::string> &&params_in)
        : _root(std::move(root_in)), _params(std::move(params_in)) {}
    Function(Function &&rhs) : _root(std::move(rhs._root)), _params(std::move(rhs._params)) {}
    ~Function() { delete_node(std::move(_root)); }
    size_t num_params() const { return _params.size(); }
    vespalib::stringref param_name(size_t idx) const { return _params[idx]; }
    bool has_error() const;
    vespalib::string get_error() const;
    const nodes::Node &root() const { return *_root; }
    static Function parse(vespalib::stringref expression);
    static Function parse(vespalib::stringref expression, const SymbolExtractor &symbol_extractor);
    static Function parse(const std::vector<vespalib::string> &params, vespalib::stringref expression);
    static Function parse(const std::vector<vespalib::string> &params, vespalib::stringref expression,
                          const SymbolExtractor &symbol_extractor);
    vespalib::string dump() const {
        nodes::DumpContext dump_context(_params);
        return _root->dump(dump_context);
    }
    // Utility function used to unwrap an expression contained inside
    // a named wrapper. For example 'max(x+y)' -> 'max', 'x+y'
    static bool unwrap(vespalib::stringref input,
                       vespalib::string &wrapper,
                       vespalib::string &body,
                       vespalib::string &error);
};

} // namespace vespalib::eval
} // namespace vespalib
