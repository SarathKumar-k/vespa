// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/vespalib/testkit/test_kit.h>
#include <vespa/vespalib/eval/function.h>
#include <vespa/vespalib/eval/interpreted_function.h>
#include <vespa/vespalib/tensor/tensor.h>
#include <vespa/vespalib/tensor/default_tensor_engine.h>

#include <vespa/searchlib/attribute/attributefactory.h>
#include <vespa/searchlib/attribute/attributevector.h>
#include <vespa/searchlib/attribute/integerbase.h>
#include <vespa/searchlib/attribute/stringbase.h>
#include <vespa/searchlib/features/setup.h>
#include <vespa/searchlib/fef/test/indexenvironment.h>
#include <vespa/searchlib/fef/test/indexenvironmentbuilder.h>
#include <vespa/searchlib/fef/test/queryenvironment.h>
#include <vespa/searchlib/fef/test/ftlib.h>
#include <vespa/searchlib/features/tensor_from_weighted_set_feature.h>
#include <vespa/searchlib/fef/fef.h>

using search::feature_t;
using namespace search::fef;
using namespace search::fef::test;
using namespace search::features;
using search::AttributeFactory;
using search::IntegerAttribute;
using search::StringAttribute;
using vespalib::eval::Value;
using vespalib::eval::Function;
using vespalib::eval::InterpretedFunction;
using vespalib::tensor::Tensor;
using vespalib::tensor::DefaultTensorEngine;

typedef search::attribute::Config AVC;
typedef search::attribute::BasicType AVBT;
typedef search::attribute::CollectionType AVCT;
typedef search::AttributeVector::SP AttributePtr;
typedef FtTestApp FTA;

struct SetupFixture
{
    TensorFromWeightedSetBlueprint blueprint;
    IndexEnvironment indexEnv;
    SetupFixture()
        : blueprint(),
          indexEnv()
    {
    }
};

TEST_F("require that blueprint can be created from factory", SetupFixture)
{
    EXPECT_TRUE(FTA::assertCreateInstance(f.blueprint, "tensorFromWeightedSet"));
}

TEST_F("require that setup fails if source spec is invalid", SetupFixture)
{
    FTA::FT_SETUP_FAIL(f.blueprint, f.indexEnv, StringList().add("source(foo)"));
}

TEST_F("require that setup succeeds with attribute source", SetupFixture)
{
    FTA::FT_SETUP_OK(f.blueprint, f.indexEnv, StringList().add("attribute(foo)"),
            StringList(), StringList().add("tensor"));
}

TEST_F("require that setup succeeds with query source", SetupFixture)
{
    FTA::FT_SETUP_OK(f.blueprint, f.indexEnv, StringList().add("query(foo)"),
            StringList(), StringList().add("tensor"));
}

struct ExecFixture
{
    BlueprintFactory factory;
    FtFeatureTest test;
    ExecFixture(const vespalib::string &feature)
        : factory(),
          test(factory, feature)
    {
        setup_search_features(factory);
        setupAttributeVectors();
        setupQueryEnvironment();
        ASSERT_TRUE(test.setup());
    }
    void setupAttributeVectors() {
        std::vector<AttributePtr> attrs;
        attrs.push_back(AttributeFactory::createAttribute("wsstr", AVC(AVBT::STRING,  AVCT::WSET)));
        attrs.push_back(AttributeFactory::createAttribute("wsint", AVC(AVBT::INT32,  AVCT::WSET)));
        attrs.push_back(AttributeFactory::createAttribute("astr", AVC(AVBT::STRING,  AVCT::ARRAY)));

        for (const auto &attr : attrs) {
            attr->addReservedDoc();
            attr->addDocs(1);
            test.getIndexEnv().getAttributeManager().add(attr);
        }

        StringAttribute *wsstr = static_cast<StringAttribute *>(attrs[0].get());
        wsstr->append(1, "a", 3);
        wsstr->append(1, "b", 5);
        wsstr->append(1, "c", 7);

        IntegerAttribute *wsint = static_cast<IntegerAttribute *>(attrs[1].get());
        wsint->append(1, 11, 3);
        wsint->append(1, 13, 5);
        wsint->append(1, 17, 7);

        for (const auto &attr : attrs) {
            attr->commit();
        }
    }
    void setupQueryEnvironment() {
        test.getQueryEnv().getProperties().add("wsquery", "{d:11,e:13,f:17}");
    }
    const Tensor &extractTensor() {
        const Value::CREF *value = test.resolveObjectFeature();
        ASSERT_TRUE(value != nullptr);
        ASSERT_TRUE(value->get().is_tensor());
        return static_cast<const Tensor &>(*value->get().as_tensor());
    }
    const Tensor &execute() {
        test.executeOnly();
        return extractTensor();
    }
};

struct AsTensor {
    InterpretedFunction ifun;
    InterpretedFunction::Context ctx;
    const Value *result;
    explicit AsTensor(const vespalib::string &expr)
        : ifun(DefaultTensorEngine::ref(), Function::parse(expr)), ctx(), result(&ifun.eval(ctx))
    {
        ASSERT_TRUE(result->is_tensor());
    }
    bool operator==(const Tensor &rhs) const { return static_cast<const Tensor &>(*result->as_tensor()).equals(rhs); }
};

std::ostream &operator<<(std::ostream &os, const AsTensor &my_tensor) {
    os << my_tensor.result->as_tensor();
    return os;
}

TEST_F("require that weighted set string attribute can be converted to tensor (default dimension)",
        ExecFixture("tensorFromWeightedSet(attribute(wsstr))"))
{
    EXPECT_EQUAL(AsTensor("{ {wsstr:b}:5, {wsstr:c}:7, {wsstr:a}:3 }"), f.execute());
}

TEST_F("require that weighted set string attribute can be converted to tensor (explicit dimension)",
        ExecFixture("tensorFromWeightedSet(attribute(wsstr),dim)"))
{
    EXPECT_EQUAL(AsTensor("{ {dim:a}:3, {dim:b}:5, {dim:c}:7 }"), f.execute());
}

TEST_F("require that weighted set integer attribute can be converted to tensor (default dimension)",
        ExecFixture("tensorFromWeightedSet(attribute(wsint))"))
{
    EXPECT_EQUAL(AsTensor("{ {wsint:13}:5, {wsint:17}:7, {wsint:11}:3 }"), f.execute());
}

TEST_F("require that weighted set integer attribute can be converted to tensor (explicit dimension)",
        ExecFixture("tensorFromWeightedSet(attribute(wsint),dim)"))
{
    EXPECT_EQUAL(AsTensor("{ {dim:17}:7, {dim:11}:3, {dim:13}:5 }"), f.execute());
}

TEST_F("require that weighted set from query can be converted to tensor (default dimension)",
        ExecFixture("tensorFromWeightedSet(query(wsquery))"))
{
    EXPECT_EQUAL(AsTensor("{ {wsquery:f}:17, {wsquery:d}:11, {wsquery:e}:13 }"), f.execute());
}

TEST_F("require that weighted set from query can be converted to tensor (explicit dimension)",
        ExecFixture("tensorFromWeightedSet(query(wsquery),dim)"))
{
    EXPECT_EQUAL(AsTensor("{ {dim:d}:11, {dim:e}:13, {dim:f}:17 }"), f.execute());
}

TEST_F("require that empty tensor is created if attribute does not exists",
        ExecFixture("tensorFromWeightedSet(attribute(null))"))
{
    EXPECT_EQUAL(AsTensor("{  }"), f.execute());
}

TEST_F("require that empty tensor is created if attribute type is not supported",
        ExecFixture("tensorFromWeightedSet(attribute(astr))"))
{
    EXPECT_EQUAL(AsTensor("{  }"), f.execute());
}

TEST_F("require that empty tensor is created if query parameter is not found",
        ExecFixture("tensorFromWeightedSet(query(null))"))
{
    EXPECT_EQUAL(AsTensor("{  }"), f.execute());
}

TEST_MAIN() { TEST_RUN_ALL(); }
