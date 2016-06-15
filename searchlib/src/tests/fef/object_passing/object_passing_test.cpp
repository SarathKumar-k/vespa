// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/vespalib/testkit/test_kit.h>
#include <vespa/vespalib/stllike/string.h>
#include <vespa/vespalib/util/stringfmt.h>
#include <vespa/searchlib/features/valuefeature.h>
#include <vespa/searchlib/fef/blueprintfactory.h>
#include <vespa/searchlib/fef/test/indexenvironment.h>
#include <vespa/searchlib/fef/test/queryenvironment.h>
#include <vespa/searchlib/fef/test/plugin/sum.h>
#include <vespa/searchlib/fef/rank_program.h>
#include <vespa/searchlib/fef/verify_feature.h>
#include <vespa/vespalib/eval/value_type.h>
#include <vespa/searchlib/fef/feature_type.h>

using namespace search::fef;
using namespace search::fef::test;
using namespace search::features;
using vespalib::eval::ValueType;

struct ProxyExecutor : FeatureExecutor {
    double                    number_value;
    vespalib::eval::Value::UP object_value;
    ProxyExecutor() : number_value(0.0), object_value() {}
    bool isPure() override { return true; }
    void execute(search::fef::MatchData &md) override {
        double was_object = 0.0;
        if (md.feature_is_object(inputs()[0])) {
            was_object = 1.0;
            number_value = md.resolve_object_feature(inputs()[0])->get().as_double();
            object_value.reset(new vespalib::eval::DoubleValue(number_value));
        } else {
            number_value = *md.resolveFeature(inputs()[0]);
            object_value.reset(new vespalib::eval::DoubleValue(number_value));
        }
        if (md.feature_is_object(outputs()[0])) {
            *md.resolve_object_feature(outputs()[0]) = *object_value;
        } else {
            *md.resolveFeature(outputs()[0]) = number_value;
        }
        *md.resolveFeature(outputs()[1]) = was_object;
    }
};

struct ProxyBlueprint : Blueprint {
    vespalib::string name;
    AcceptInput accept_input;
    bool object_output;
    ProxyBlueprint(const vespalib::string &name_in, AcceptInput accept_input_in, bool object_output_in)
        : Blueprint(name_in), name(name_in), accept_input(accept_input_in), object_output(object_output_in) {}
    void visitDumpFeatures(const IIndexEnvironment &, IDumpFeatureVisitor &) const override {}
    Blueprint::UP createInstance() const override {
        return Blueprint::UP(new ProxyBlueprint(name, accept_input, object_output));
    }
    bool setup(const IIndexEnvironment &, const std::vector<vespalib::string> &params) override {
        ASSERT_EQUAL(1u, params.size());
        defineInput(params[0], accept_input);
        describeOutput("value", "the value", object_output ? FeatureType::object(ValueType::double_type()) : FeatureType::number());
        describeOutput("was_object", "whether input was object", FeatureType::number());
        return true;
    }
    FeatureExecutor::LP createExecutor(const IQueryEnvironment &) const override {
        return FeatureExecutor::LP(new ProxyExecutor());
    }
};

struct Fixture {
    BlueprintFactory factory;
    IndexEnvironment indexEnv;

    explicit Fixture() {
        factory.addPrototype(Blueprint::SP(new ValueBlueprint()));
        factory.addPrototype(Blueprint::SP(new ProxyBlueprint("box",         Blueprint::AcceptInput::NUMBER, true)));
        factory.addPrototype(Blueprint::SP(new ProxyBlueprint("maybe_box",   Blueprint::AcceptInput::ANY,    true)));
        factory.addPrototype(Blueprint::SP(new ProxyBlueprint("unbox",       Blueprint::AcceptInput::OBJECT, false)));
        factory.addPrototype(Blueprint::SP(new ProxyBlueprint("maybe_unbox", Blueprint::AcceptInput::ANY,    false)));
    }

    double eval(const vespalib::string &feature) {
        BlueprintResolver::SP resolver(new BlueprintResolver(factory, indexEnv));
        resolver->addSeed(feature);
        if (!resolver->compile()) {
            return vespalib::eval::error_value;        
        }
        MatchDataLayout mdl;
        QueryEnvironment queryEnv(&indexEnv);
        Properties overrides;
        RankProgram program(resolver);
        program.setup(mdl, queryEnv, overrides);        
        program.run(1);
        std::vector<vespalib::string> names;
        std::vector<FeatureHandle> handles;
        program.get_seed_handles(names, handles);
        EXPECT_EQUAL(1u, names.size());
        EXPECT_EQUAL(names.size(), handles.size());
        const auto &md = program.match_data();
        EXPECT_TRUE(!md.feature_is_object(handles[0])); // verifies auto-unboxing
        return *md.resolveFeature(handles[0]);
    }

    bool verify(const vespalib::string &feature) {
        return verifyFeature(factory, indexEnv, feature, "unit test");
    }
};

TEST_F("require that values can be boxed and unboxed", Fixture()) {
    EXPECT_EQUAL(3.0, f1.eval("box(value(3))"));
    EXPECT_EQUAL(0.0, f1.eval("box(value(3)).was_object"));
    EXPECT_EQUAL(3.0, f1.eval("unbox(box(value(3)))"));
    EXPECT_EQUAL(1.0, f1.eval("unbox(box(value(3))).was_object"));
    EXPECT_EQUAL(3.0, f1.eval("box(unbox(box(value(3))))"));
    EXPECT_EQUAL(0.0, f1.eval("box(unbox(box(value(3)))).was_object"));
}

TEST_F("require that output features may be either objects or numbers", Fixture()) {
    EXPECT_TRUE(f1.verify("value(3)"));
    EXPECT_TRUE(f1.verify("box(value(3))"));
}

TEST_F("require that feature input/output types must be compatible", Fixture()) {
    EXPECT_TRUE(!f1.verify("unbox(value(3))"));
    EXPECT_TRUE(f1.verify("maybe_unbox(value(3))"));
    EXPECT_TRUE(f1.verify("unbox(box(value(3)))"));
    EXPECT_TRUE(!f1.verify("unbox(box(box(value(3))))"));
    EXPECT_TRUE(f1.verify("unbox(maybe_box(box(value(3))))"));
    EXPECT_TRUE(f1.verify("unbox(box(unbox(box(value(3)))))"));
}

TEST_MAIN() { TEST_RUN_ALL(); }
