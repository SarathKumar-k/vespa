// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP("objectselection_test");
#include <vespa/vespalib/testkit/testapp.h>
#include <vespa/vespalib/objects/identifiable.h>
#include <vespa/vespalib/objects/objectpredicate.h>
#include <vespa/vespalib/objects/objectoperation.h>

using namespace vespalib;

#define CID_Foo  60000005
#define CID_Bar  60000010

struct Foo : public Identifiable
{
    typedef IdentifiablePtr<Foo> CP;
    std::vector<CP> nodes;

    DECLARE_IDENTIFIABLE(Foo);
    virtual Foo *clone() const { return new Foo(*this); }
    virtual void selectMembers(const ObjectPredicate &p, ObjectOperation &o) {
        for (uint32_t i = 0; i < nodes.size(); ++i) {
            nodes[i]->select(p, o);
        }
    }
};
IMPLEMENT_IDENTIFIABLE(Foo, Identifiable);

struct Bar : public Foo
{
    int value;

    DECLARE_IDENTIFIABLE(Bar);
    Bar() : value(0) {}
    Bar(int v) { value = v; }
    virtual Bar *clone() const { return new Bar(*this); }
};
IMPLEMENT_IDENTIFIABLE(Bar, Identifiable);

struct ObjectType : public ObjectPredicate
{
    uint32_t cid;
    ObjectType(uint32_t id) : cid(id) {}
    virtual bool check(const Identifiable &obj) const {
        return (obj.getClass().id() == cid);
    }
};

struct ObjectCollect : public ObjectOperation
{
    std::vector<Identifiable*> nodes;
    virtual void execute(Identifiable &obj) {
        nodes.push_back(&obj);
    }
};

TEST_SETUP(Test);

int
Test::Main()
{
    TEST_INIT("objectselection_test");
    {
        Foo f1;
        Foo f2;
        Foo f3;
        Bar b1(1);
        Bar b2(2);
        Bar b3(3);
        Bar b4(4);
        f2.nodes.push_back(b1);
        f2.nodes.push_back(b2);
        f3.nodes.push_back(b3);
        f3.nodes.push_back(b4);
        f1.nodes.push_back(f2);
        f1.nodes.push_back(f3);

        ObjectType predicate(Bar::classId);
        ObjectCollect operation;
        f1.select(predicate, operation);
        ASSERT_TRUE(operation.nodes.size() == 4);
        ASSERT_TRUE(operation.nodes[0]->getClass().id() == Bar::classId);
        ASSERT_TRUE(operation.nodes[1]->getClass().id() == Bar::classId);
        ASSERT_TRUE(operation.nodes[2]->getClass().id() == Bar::classId);
        ASSERT_TRUE(operation.nodes[3]->getClass().id() == Bar::classId);
        ASSERT_TRUE(((Bar*)operation.nodes[0])->value == 1);
        ASSERT_TRUE(((Bar*)operation.nodes[1])->value == 2);
        ASSERT_TRUE(((Bar*)operation.nodes[2])->value == 3);
        ASSERT_TRUE(((Bar*)operation.nodes[3])->value == 4);
    }
    TEST_DONE();
}
