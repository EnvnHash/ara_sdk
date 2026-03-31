//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <UtilityUnitTestCommon.h>
#include <DataModel/Node.h>

using json = nlohmann::json;

namespace ara {

class TestClass : public Node {
public:
    ARA_NODE_ADD_SERIALIZE_FUNCTIONS(Node, m_arg1, m_arg2, m_arg3)

    TestClass() {
        setTypeName<TestClass>();
        m_changeCb[cbType::postChange].emplace(this, [this](std::optional<Node*>) {
            m_changeCalled++;
        });
    }

    int m_arg1 = 1;
    float m_arg2 = 2.f;
    double m_arg3 = 3.0;
    size_t m_changeCalled = 0;
};

class ChildTestClass : public TestClass {
public:
    ChildTestClass() {
        m_postLoadCb = [this]{
            m_postLoadCbCalled++;
        };
    }
    size_t m_postLoadCbCalled = 0;
};

static Node& createNestedNode(Node& nd, const int depth) {
    nd.setName("Root");
    Node* lowestChild = nullptr;
    std::function<void(Node&, int&)> addFunc = [&](Node& parent, int& level) {
        auto& child = parent.push<Node>();
        child.setName("Child"+std::to_string(level+1));
        lowestChild = &child;
        if (++level < depth) {
            addFunc(child, level);
        }
    };

    int level = 0;
    addFunc(nd, level);
    return *lowestChild;
}

TEST(Functional_Node, Parent) {
    Node nd;
    const auto& lowestChild = createNestedNode(nd, 1);
    EXPECT_EQ(lowestChild.parent(), &nd);
}

TEST(Functional_Node, GetRoot) {
    Node nd;
    auto& lowestChild = createNestedNode(nd, 3);
    EXPECT_EQ(lowestChild.root(), &nd);
}

TEST(Functional_Node, FindChildrenByName) {
    Node nd;
    createNestedNode(nd, 2);
    const auto list = nd.findChild("Child2");
    EXPECT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->name(), "Child2");
}

TEST(Functional_Node, FindChildrenByUuid) {
    Node nd;
    auto& lowest = createNestedNode(nd, 2);
    const std::string uuid = "6998A327-29B0-90E9-984A-366FEB38BFFE";
    lowest.setUuid(uuid);
    const auto node = nd.findChildByUuid(uuid);
    ASSERT_FALSE(node == nullptr);
    EXPECT_EQ(node->uuid(), uuid);
}

TEST(Functional_Node, AddChild) {
    Node nd;
    nd.push<Node>();
    EXPECT_EQ(nd.children().size(), 1);

    nd.push<Node>();
    EXPECT_EQ(nd.children().size(), 2);

    nd.push(std::make_shared<Node>());
    EXPECT_EQ(nd.children().size(), 3);
}

TEST(Functional_Node, InsertChildByPos) {
    constexpr std::array names = { "Child1", "Child2", "Child3" };
    Node nd;
    auto& n1 = nd.push<Node>();
    n1.setName(names[0]);

    auto& n2 = nd.push<Node>();
    n2.setName(names[1]);

    auto& n3 = nd.insertChild<Node>(0);
    n3.setName(names[2]);

    int cntr=0;
    for (const auto& it : nd.children()) {
        constexpr std::array newOrder = { 2, 0, 1 };
        EXPECT_EQ(it->name(), names[newOrder[cntr]]);
        ++cntr;
    }
}

TEST(Functional_Node, InsertChildByName) {
    constexpr std::array names = { "Child1", "Child2", "Child3" };
    Node nd;
    auto& n1 = nd.push<Node>();
    n1.setName(names[0]);

    auto& n2 = nd.push<Node>();
    n2.setName(names[1]);

    auto& n3 = nd.insertChild<Node>(names[1]);
    n3.setName(names[2]);

    int cntr=0;
    for (const auto& it : nd.children()) {
        constexpr std::array newOrder = { 0, 2, 1 };
        EXPECT_EQ(it->name(), names[newOrder[cntr]]);
        ++cntr;
    }
}

TEST(Functional_Node, InsertAfterByName) {
    constexpr std::array names = { "Child1", "Child2", "Child3" };
    Node nd;
    auto& n1 = nd.push<Node>();
    n1.setName(names[0]);

    auto& n2 = nd.push<Node>();
    n2.setName(names[1]);

    auto& n3 = nd.insertAfter<Node>(names[0]);
    n3.setName(names[2]);

    int cntr=0;
    for (const auto& it : nd.children()) {
        constexpr std::array newOrder = { 0, 2, 1 };
        EXPECT_EQ(it->name(), names[newOrder[cntr]]);
        ++cntr;
    }
}

TEST(Functional_Node, AddChildByRef) {
    Node nd1;
    nd1.push<Node>();
    EXPECT_EQ(nd1.children().size(), 1);

    Node nd2;
    auto& child = nd2.push<Node>();
    child.setName("horst");
    EXPECT_EQ(nd2.children().size(), 1);

    const auto& list = nd2.children();
    nd1.push(list.back());

    const auto child2 = nd2.children().back();
    EXPECT_EQ(child.name(), child2->name());
}

TEST(Functional_Node, AddChildByRefMemCheck) {
    const std::string child1Name = "child1";
    Node nd1;
    auto& child1 = nd1.push<Node>();
    child1.setName(child1Name);

    Node nd2;
    const auto& list = nd1.children();
    nd2.push(list.back());

    nd1.clearChildren();
    EXPECT_EQ(nd1.children().size(), 0);

    const auto child2 = nd2.children().back();
    EXPECT_EQ(child1Name, child2->name());
}

TEST(Functional_Node, RemoveChild) {
    Node nd1;
    nd1.push<Node>();
    EXPECT_EQ(nd1.children().size(), 1);

    nd1.pop();
    EXPECT_EQ(nd1.children().size(), 0);
}

TEST(Functional_Node, ChangedCb) {
    TestClass tc;
    auto j = tc.asJson();
    tc.deserialize(j);
    EXPECT_EQ(tc.m_changeCalled, 0);

    j = tc.asJson();
    j["arg1"] = 2;
    j["arg2"] = 3.f;
    j["arg3"] = 4.0;
    tc.deserialize(j);
    EXPECT_EQ(tc.m_changeCalled, 1);
}

TEST(Functional_Node, PostLoadCb) {
    TestClass tc;
    tc.push<ChildTestClass>();

    auto j = tc.asJson();
    j["arg1"] = 2;
    tc.deserialize(j);

    const auto child = dynamic_cast<ChildTestClass*>(tc.children().front().get());
    EXPECT_EQ(child->m_postLoadCbCalled, 1);
}

TEST(Functional_Node, SignalAddChild) {
    bool preCalled = false;
    bool postCalled = false;

    Node nd1;
    nd1.setOnChangeCb(Node::cbType::preAddChild, nullptr, [&](std::optional<Node*>) {
        EXPECT_EQ(nd1.children().size(), 0);
        preCalled = true;
    });
    nd1.setOnChangeCb(Node::cbType::postAddChild, nullptr, [&] (std::optional<Node*>){
        EXPECT_EQ(nd1.children().size(), 1);
        postCalled = true;
    });
    nd1.push<Node>();

    EXPECT_TRUE(preCalled);
    EXPECT_TRUE(postCalled);
}

TEST(Functional_Node, SignalRemoveChild) {
    bool preCalled = false;
    bool postCalled = false;

    Node nd1;
    nd1.push<Node>();

    nd1.setOnChangeCb(Node::cbType::preRemoveChild, nullptr, [&] (std::optional<Node*>){
        EXPECT_EQ(nd1.children().size(), 1);
        preCalled = true;
    });
    nd1.setOnChangeCb(Node::cbType::postRemoveChild, nullptr, [&] (std::optional<Node*>){
        EXPECT_EQ(nd1.children().size(), 0);
        postCalled = true;
    });

    nd1.pop();

    EXPECT_TRUE(preCalled);
    EXPECT_TRUE(postCalled);
}

TEST(Functional_Node, SignalRemoveChildByPtr) {
    Node nd1;
    auto& child1 = nd1.push<Node>();
    auto& child2 = nd1.push<Node>();
    auto& child3 = nd1.push<Node>();

    nd1.remove(&child2);

    EXPECT_EQ(nd1.children().size(), 2);
    EXPECT_EQ(nd1.children().front().get(), &child1);
    EXPECT_EQ(nd1.children().back().get(), &child3);
}

TEST(Functional_Node, SignalRemoveChildNested) {
    bool preCalled = false;
    bool postCalled = false;
    bool nestedPreCalled = false;
    bool nestedPostCalled = false;

    Node nd1;
    auto& child = nd1.push<Node>();
    child.push<Node>();

    nd1.setOnChangeCb(Node::cbType::preRemoveChild, nullptr, [&] (std::optional<Node*>){
        EXPECT_EQ(nd1.children().size(), 1);
        preCalled = true;
    });
    nd1.setOnChangeCb(Node::cbType::postRemoveChild, nullptr, [&] (std::optional<Node*>){
        EXPECT_EQ(nd1.children().size(), 0);
        postCalled = true;
    });
    child.setOnChangeCb(Node::cbType::preRemoveChild, nullptr, [&] (std::optional<Node*>){
        EXPECT_EQ(child.children().size(), 1);
        nestedPreCalled = true;
    });
    child.setOnChangeCb(Node::cbType::postRemoveChild, nullptr, [&](std::optional<Node*>) {
        nestedPostCalled = true;
    });

    nd1.pop();

    EXPECT_TRUE(preCalled);
    EXPECT_TRUE(postCalled);
    EXPECT_TRUE(nestedPreCalled);
    EXPECT_TRUE(nestedPostCalled);
}

TEST(Functional_Node, Serialize_Values) {
    Node node;
    node.setName("Name");

    json j;
    node.serializeClassValues(j);
    const auto s = j.dump();

    EXPECT_EQ(s, "{\"name\":\"Name\",\"typeName\":\"Node\",\"uuid\":\"\"}");
}

TEST(Functional_Node, Serialize_NonClassValueFloat) {
    Node node;
    node.setKey("Value");
    node.setValue(1.5f);

    json j;
    node.serializeNonClassValue(j);
    const auto s = j.dump();

    EXPECT_EQ(s, "{\"Value\":1.5}");
}

TEST(Functional_Node, Serialize_NonClassValueBool) {
    Node node;
    node.setKey("Value");
    node.setValue(true);

    json j;
    node.serializeNonClassValue(j);
    const auto s = j.dump();

    EXPECT_EQ(s, "{\"Value\":true}");
}

TEST(Functional_Node, Serialize_NonClassValueInt) {
    Node node;
    node.setKey("Value");
    node.setValue(256);

    json j;
    node.serializeNonClassValue(j);
    const auto s = j.dump();

    EXPECT_EQ(s, "{\"Value\":256}");
}

TEST(Functional_Node, Serialize_NonClassValueString) {
    Node node;
    node.setKey("Value");
    node.setValue("hello");

    json j;
    node.serializeNonClassValue(j);
    const auto s = j.dump();

    EXPECT_EQ(s, "{\"Value\":\"hello\"}");
}

TEST(Functional_Node, Serialize_NonClassValueArray) {
    Node node;
    node.setKey("array");
    node.setNodeValueType(nodeValueType::array);
    for (int i=0; i<3; i++) {
        auto& v = node.push<Node>();
        v.setKey(std::to_string(i));
        v.setValue(i);
    }

    json j;
    node.serialize(j, true);
    const auto s = j.dump();
    EXPECT_EQ(s, "{\"array\":[0,1,2]}");
}

TEST(Functional_Node, Serialize_NonClassValueObject) {
    Node node;
    node.setKey("object");
    node.setNodeValueType(nodeValueType::object);

    auto& v = node.push<Node>();
    v.setKey("arg0");
    v.setValue(1);

    auto& v1 = node.push<Node>();
    v1.setKey("arg1");
    v1.setNodeValueType(nodeValueType::array);
    for (int i=0; i<3; i++) {
        auto& vv = v1.push<Node>();
        vv.setKey(std::to_string(i));
        vv.setValue(i);
    }

    auto& v2 = node.push<Node>();
    v2.setKey("arg2");
    v2.setNodeValueType(nodeValueType::object);
    auto &vv = v2.push<Node>();
    vv.setKey("subElement");
    vv.setValue(2);

    auto &vv2 = v2.push<Node>();
    vv2.setKey("subElement");
    vv2.setValue(2);

    auto& v3 = node.push<Node>();
    v3.setKey("arg3");
    v3.setValue(2.f);

    auto &v4 = node.push<Node>();
    v4.setKey("arg4");
    v4.setValue(true);

    auto &v5 = node.push<Node>();
    v5.setKey("arg5");
    v5.setValue("hello");

    json j;
    node.serialize(j, true);
    const auto s = j.dump();
    EXPECT_EQ(s, "{\"object\":{\"arg0\":1,\"arg1\":[0,1,2],\"arg2\":{\"subElement\":2},\"arg3\":2.0,\"arg4\":true,\"arg5\":\"hello\"}}");
}

TEST(Functional_Node, Serialize_Node) {
    Node node;
    node.setName("Name");

    const auto j = node.asJson();
    const std::string s = j.dump();
    EXPECT_EQ(s, "{\"name\":\"Name\",\"typeName\":\"Node\",\"uuid\":\"\"}");
}

TEST(Functional_Node, Serialize_Children) {
    Node node;
    node.setName("Name");

    auto& child = node.push<Node>();
    child.setName("Child");
    child.setUuid(""); // reset uuid for testing

    const auto j = node.asJson();
    const std::string s = j.dump();
    EXPECT_EQ(s, "{\"children\":[{\"name\":\"Child\",\"typeName\":\"Node\",\"uuid\":\"\"}],\"name\":\"Name\",\"typeName\":\"Node\",\"uuid\":\"\"}");
}

TEST(Functional_Node, Serialize_MultipleChildren) {
    Node node;
    node.setName("Root");

    auto& child1 = node.push<Node>();
    child1.setName("Child1");
    child1.setUuid(""); // reset uuid for testing

    auto& child2 = node.push<Node>();
    child2.setName("Child2");
    child2.setUuid("");

    auto& child3 = node.push<Node>();
    child3.setName("Child3");
    child3.setUuid("");

    auto& child4 = node.push<Node>();
    child4.setName("Child4");
    child4.setUuid("");

    const auto j = node.asJson();
    const std::string s = j.dump();
    EXPECT_EQ(s, "{\"children\":[{\"name\":\"Child1\",\"typeName\":\"Node\",\"uuid\":\"\"},{\"name\":\"Child2\",\"typeName\":\"Node\",\"uuid\":\"\"},{\"name\":\"Child3\",\"typeName\":\"Node\",\"uuid\":\"\"},{\"name\":\"Child4\",\"typeName\":\"Node\",\"uuid\":\"\"}],\"name\":\"Root\",\"typeName\":\"Node\",\"uuid\":\"\"}");
}

TEST(Functional_Node, Parse_Values) {
    const auto j = R"({"name":"Name","typeName":"Node","uuid":"6998A327-29B0-90E9-984A-366FEB38BFFE"})"_json;

    Node node;
    node.deserializeClassValues(j);

    EXPECT_EQ(node.name(), "Name");
    EXPECT_EQ(node.typeName(), "Node");
    EXPECT_EQ(node.uuid(), "6998A327-29B0-90E9-984A-366FEB38BFFE");
}

TEST(Functional_Node, Parse_Node) {
    const auto j = R"({"name":"Name","typeName":"Node","uuid":"6998A327-29B0-90E9-984A-366FEB38BFFE"})"_json;

    Node node;
    node.deserialize(j);

    EXPECT_EQ(node.name(), "Name");
    EXPECT_EQ(node.typeName(), "Node");
    EXPECT_EQ(node.uuid(), "6998A327-29B0-90E9-984A-366FEB38BFFE");
}

TEST(Functional_Node, ParseDerived) {
    Node::clearClassKeys();

    class Derived : public Node {
    public:
        Derived() { setTypeName<Derived>(); }
        ARA_NODE_ADD_SERIALIZE_FUNCTIONS(Node, m_par1, m_par2)
        int     m_par1 = 2;
        float   m_par2 = 3.f;
    };

    Derived d;
    d.setName("Name");
    d.setUuid("6998A327-29B0-90E9-984A-366FEB38BFFE");
    const auto& serialized = d.asJson();

    Derived loadedD;
    loadedD.deserialize(serialized);

    EXPECT_EQ(loadedD.name(), "Name");
    EXPECT_EQ(loadedD.typeName(), "Derived");
    EXPECT_EQ(loadedD.m_par1, 2);
    EXPECT_EQ(loadedD.m_par2, 3.f);
    EXPECT_EQ(loadedD.getClassKeys().size(), 1);
    EXPECT_EQ(loadedD.getClassKeys().at("Derived").first.size(), 5);
}

TEST(Functional_Node, ParseDerivedNested) {
    Node::clearClassKeys();
    class Derived : public Node {
    public:
        Derived() { setTypeName<Derived>(); }
        ARA_NODE_ADD_SERIALIZE_FUNCTIONS(Node, m_par1, m_par2)
        int     m_par1 = 2;
        float   m_par2 = 3.f;
    };

    class DerivedDerived : public Derived {
    public:
        DerivedDerived() { setTypeName<DerivedDerived>(); }
        ARA_NODE_ADD_SERIALIZE_FUNCTIONS(Derived, m_par3, m_par4)
        float   m_par3 = 2.f;
        int     m_par4 = 3;
    };

    DerivedDerived d;
    d.setName("Name");
    d.setUuid("6998A327-29B0-90E9-984A-366FEB38BFFE");
    const auto& serialized = d.asJson();

    DerivedDerived loadedD;
    loadedD.deserialize(serialized);

    EXPECT_EQ(loadedD.name(), "Name");
    EXPECT_EQ(loadedD.typeName(), "DerivedDerived");
    EXPECT_EQ(loadedD.m_par1, 2);
    EXPECT_EQ(loadedD.m_par2, 3.f);
    EXPECT_EQ(loadedD.m_par3, 2.f);
    EXPECT_EQ(loadedD.m_par4, 3);
    EXPECT_EQ(loadedD.getClassKeys().size(), 1);
    EXPECT_EQ(loadedD.getClassKeys().at("DerivedDerived").first.size(), 7);
}

TEST(Functional_Node, Parse_Generic) {
    const auto j = R"({"key0":0,"key1":1.2345,"key2":"value"})"_json;

    Node node;
    node.deserialize(j, true);

    ASSERT_NE(node.findChildByKey("key0"), nullptr);
    ASSERT_NE(node.findChildByKey("key1"), nullptr);
    ASSERT_NE(node.findChildByKey("key2"), nullptr);

    const auto key0 = node.findChildByKey("key0");
    EXPECT_EQ(key0->value<int32_t>(), 0);

    const auto key1 = node.findChildByKey("key1");
    EXPECT_EQ(key1->value<float>(), 1.2345f);

    const auto key2 = node.findChildByKey("key2");
    EXPECT_EQ(key2->value<std::string>(), "value");
}

TEST(Functional_Node, Parse_Generic_Array) {
    const auto j = R"({"array":[1,2,3,4]})"_json;

    Node node;
    node.deserialize(j, true);
    const auto arrayChild = node.findChildByKey("array");
    ASSERT_NE(arrayChild, nullptr);

    for (int i=0; i<4; i++) {
        const auto entr = arrayChild->findChildByKey(std::to_string(i));
        ASSERT_NE(arrayChild, nullptr);
        EXPECT_EQ(entr->value<int32_t>(), i+1);
    }
}

TEST(Functional_Node, Parse_Generic_Object) {
    const auto j = R"({"objEntry":{ "key0" : 1, "key1" : 1.234, "key2" : "some string"}})"_json;

    Node node;
    node.deserialize(j);
    const auto objChild = node.findChildByKey("objEntry");
    ASSERT_NE(objChild, nullptr);

    ASSERT_NE(objChild->findChildByKey("key0"), nullptr);
    ASSERT_NE(objChild->findChildByKey("key1"), nullptr);
    ASSERT_NE(objChild->findChildByKey("key2"), nullptr);

    const auto key0 = objChild->findChildByKey("key0");
    EXPECT_EQ(key0->value<int32_t>(), 1);

    const auto key1 = objChild->findChildByKey("key1");
    EXPECT_EQ(key1->value<float>(), 1.234f);

    const auto key2 = objChild->findChildByKey("key2");
    EXPECT_EQ(key2->value<std::string>(), "some string");
}

TEST(Functional_Node, Parse_Generic_And_Reserialize) {
    const std::string str = "{\"object\":{\"arg0\":1,\"arg1\":[0,1,2],\"arg2\":{\"subElement\":2},\"arg3\":2.0,\"arg4\":true,\"arg5\":\"hello\"}}";

    Node node;
    node.deserialize(str, true);

    const auto retJson = node.asJson(true);
    const auto s = retJson.dump();
    EXPECT_EQ(s, str);
}

TEST(Functional_Node, Parse_Generic_And_Reserialize_Array_of_Objects) {
    const std::string str = "{\"arrayOfObjects\":[{\"name\":\"obj1\",\"val\":1},{\"name\":\"obj2\",\"val\":2},{\"name\":\"obj3\",\"val\":3}]}";

    Node node;
    node.deserialize(str, true);

    LOG << "\n-------------------------\n";
    const auto retJson = node.asJson(true);
    const auto s = retJson.dump();
    EXPECT_EQ(s, str);
}

TEST(Functional_Node, Parse_Children) {
    const std::string s = R"({
        "children":[
            {
                "name":"Child",
                "typeName":"Node",
                "uuid":"6998A327-29B0-90E9-984A-366FEB39768A"
            }
        ],
        "name":"Name",
        "typeName":"Node",
        "uuid":"6998A327-29B0-90E9-984A-366FEB38BFFE"
    })";

    Node node;
    node.deserialize(s);

    EXPECT_EQ(node.name(), "Name");
    EXPECT_EQ(node.children().size(), 1);
    EXPECT_EQ(node.children().front()->name(), "Child");
}

TEST(Functional_Node, Parse_Update_SwappedNode) {
    const std::string s = R"({
        "children":[
            {
                "name":"Child1",
                "typeName":"Node",
                "uuid":"1998A327-29B0-90E9-984A-366FEB39768A"
            },
            {
                "name":"Child2",
                "typeName":"Node",
                "uuid":"2998A327-29B0-90E9-984A-366FEB39768A"
            },
            {
                "name":"Child3",
                "typeName":"Node",
                "uuid":"3998A327-29B0-90E9-984A-366FEB39768A"
            }
        ],
        "name":"Root",
        "typeName":"Node",
        "uuid":"6998A327-29B0-90E9-984A-366FEB38BFFE"
    })";

    Node node;
    node.deserialize(s);

    EXPECT_EQ(node.name(), "Root");
    EXPECT_EQ(node.children().size(), 3);
    EXPECT_EQ(node.children().front()->name(), "Child1");
    EXPECT_EQ(std::next(node.children().begin(), 1)->get()->name(), "Child2");
    EXPECT_EQ(std::next(node.children().begin(), 2)->get()->name(), "Child3");

    bool triggered = false;
    node.children().front()->setOnChangeCb(Node::cbType::preChange, nullptr, [&](std::optional<Node*>){ triggered = true; });

    const std::string s2 = R"({
        "children":[
            {
                "name":"Child2",
                "typeName":"Node",
                "uuid":"2998A327-29B0-90E9-984A-366FEB39768A"
            },
            {
                "name":"Child1",
                "typeName":"Node",
                "uuid":"1998A327-29B0-90E9-984A-366FEB39768A"
            },
            {
                "name":"Child3",
                "typeName":"Node",
                "uuid":"3998A327-29B0-90E9-984A-366FEB39768A"
            }
        ],
        "name":"Root",
        "typeName":"Node",
        "uuid":"6998A327-29B0-90E9-984A-366FEB38BFFE"
    })";

    node.deserialize(s2);

    EXPECT_EQ(node.name(), "Root");
    EXPECT_EQ(node.children().size(), 3);
    EXPECT_EQ(node.children().front()->name(), "Child2");
    EXPECT_EQ(std::next(node.children().begin(), 1)->get()->name(), "Child1");
    EXPECT_EQ(std::next(node.children().begin(), 2)->get()->name(), "Child3");
    EXPECT_FALSE(triggered);
}

TEST(Functional_Node, SavingLoading) {
    Node nd;
    nd.setName("Root");
    auto& child1 = nd.push<Node>();
    child1.setName("Child1");
    auto& child2 = child1.push<Node>();
    child2.setName("Child2");

    const std::string fn = "Functional_Node_SavingLoading.json";
    nd.saveAs(fn);

    Node newNode;
    newNode.load(fn);

    EXPECT_EQ(nd.name(), newNode.name());
    EXPECT_EQ(nd.children().size(), newNode.children().size());
    EXPECT_EQ(nd.children().front()->name(), newNode.children().front()->name());
    EXPECT_EQ(nd.children().front()->children().size(), newNode.children().front()->children().size());
    EXPECT_EQ(nd.children().front()->children().front()->name(), newNode.children().front()->children().front()->name());
}

TEST(Functional_Node, Undo_Redo_Child) {
    Node nd;
    nd.setUndoBuffer(true, 10);
    nd.setName("Root");

    auto& child1 = nd.push<Node>();
    EXPECT_EQ(nd.children().size(), 1);

    nd.undo();
    EXPECT_EQ(nd.children().size(), 0);

    nd.redo();
    EXPECT_EQ(nd.children().size(), 1);
}

TEST(Functional_Node, Undo_Redo_Value) {
    Node nd;
    nd.setUndoBuffer(true, 10);
    nd.setName("Root");
    nd.setName("Child1");

    nd.undo();
    EXPECT_EQ(nd.name(), "Root");

    nd.redo();
    EXPECT_EQ(nd.name(), "Child1");
}

TEST(Functional_Node, Undo_Cut_Queue) {
    Node nd;
    nd.setUndoBuffer(true, 10);
    nd.setName("Root");
    nd.setName("Child1");
    nd.setName("Child2");

    nd.undo();
    EXPECT_EQ(nd.name(), "Child1");

    nd.undo();
    EXPECT_EQ(nd.name(), "Root");
    EXPECT_EQ(nd.undoBufQueue().size(), 4);

    nd.setName("Child3");
    EXPECT_EQ(nd.undoBufQueue().size(), 2);
}

TEST(Functional_Node, Undo_Queue_Juggle) {
    Node nd;
    nd.setUndoBuffer(true, 10);
    nd.setName("Root");
    nd.setName("Child1");
    nd.setName("Child2");
    nd.setName("Child3");

    nd.undo();
    EXPECT_EQ(nd.name(), "Child2");

    nd.undo();
    EXPECT_EQ(nd.name(), "Child1");

    nd.redo();
    EXPECT_EQ(nd.name(), "Child2");

    nd.redo();
    EXPECT_EQ(nd.name(), "Child3");
}

}  // namespace ara
