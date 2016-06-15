// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/document/annotation/annotation.h>
#include <vespa/vespalib/util/printable.h>
#include <vector>

namespace document {
class SpanNode;
class SpanTreeVisitor;

class SpanTree : public Printable {
    typedef std::vector<Annotation> AnnotationVector;
    vespalib::string _name;
    std::unique_ptr<SpanNode> _root;
    std::vector<Annotation> _annotations;

public:
    typedef std::unique_ptr<SpanTree> UP;
    typedef AnnotationVector::const_iterator const_iterator;

    template <typename T>
    SpanTree(const vespalib::stringref &name, std::unique_ptr<T> root)
        : _name(name),
          _root(std::move(root)) {
        assert(_root.get());
    }
    ~SpanTree();

    // The annotate functions return the annotation index.
    size_t annotate(std::unique_ptr<Annotation> annotation);
    size_t annotate(const SpanNode &node, std::unique_ptr<Annotation> a);
    size_t annotate(const SpanNode &node, const AnnotationType &a_type);

    Annotation & annotation(size_t index) { return _annotations[index]; }
    const Annotation & annotation(size_t index) const { return _annotations[index]; }

    void accept(SpanTreeVisitor &visitor) const;
    virtual void print(
            std::ostream& out, bool verbose, const std::string& indent) const;

    const vespalib::string & getName() const { return _name; }
    const SpanNode &getRoot() const { return *_root; }
    size_t numAnnotations() const { return _annotations.size(); }
    void reserveAnnotations(size_t sz) { _annotations.resize(sz); }
    const_iterator begin() const { return _annotations.begin(); }
    const_iterator end() const { return _annotations.end(); }
    int compare(const SpanTree &other) const;
};

}  // namespace document

