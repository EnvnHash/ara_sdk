#include<utility_unit_test_common.h>

#include <DataModel/Node.h>

using json = nlohmann::json;

namespace ara {

TEST(Functional_Node, Parent) {
    Node nd;
    nd.setName("Root");
    auto& child = nd.push<Node>();
    child.setName("Child");
    EXPECT_EQ(child.parent(), &nd);
}

TEST(Functional_Node, GetRoot) {
    Node nd;
    nd.setName("Root");
    auto& child = nd.push<Node>();
    child.setName("Child");
    auto& child2 = child.push<Node>();
    child2.setName("Child2");
    auto& child3 = child2.push<Node>();
    child3.setName("Child3");
    EXPECT_EQ(child3.root(), &nd);
}

TEST(Functional_Node, FindChildrenByName) {
    Node nd;
    nd.setName("Root");
    auto& child = nd.push<Node>();
    child.setName("Child");
    auto& child2 = child.push<Node>();
    child2.setName("Child2");

    auto list = nd.findChild("Child2");
    EXPECT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->name(), "Child2");
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

TEST(Functional_Node, AddChildByRef) {
    Node nd1;
    nd1.push<Node>();
    EXPECT_EQ(nd1.children().size(), 1);

    Node nd2;
    auto& child = nd2.push<Node>();
    child.setName("horst");
    EXPECT_EQ(nd2.children().size(), 1);

    auto& list = nd2.children();
    nd1.push(list.back());

    auto child2 = nd2.children().back();
    EXPECT_EQ(child.name(), child2->name());
}

TEST(Functional_Node, AddChildByRefMemCheck) {
    std::string child1Name = "child1";
    Node nd1;
    auto& child1 = nd1.push<Node>();
    child1.setName(child1Name);

    Node nd2;
    auto& list = nd1.children();
    nd2.push(list.back());

    nd1.clearChildren();
    EXPECT_EQ(nd1.children().size(), 0);

    auto child2 = nd2.children().back();
    EXPECT_EQ(child1Name, child2->name());
}

TEST(Functional_Node, RemoveChild) {
    Node nd1;
    nd1.push<Node>();
    EXPECT_EQ(nd1.children().size(), 1);

    nd1.pop();
    EXPECT_EQ(nd1.children().size(), 0);
}

TEST(Functional_Node, SignalAddChild) {
    bool preCalled = false;
    bool postCalled = false;

    Node nd1;
    nd1.setOnChangeCb(Node::cbType::preAddChild, nullptr, [&] {
        EXPECT_EQ(nd1.children().size(), 0);
        preCalled = true;
    });
    nd1.setOnChangeCb(Node::cbType::postAddChild, nullptr, [&] {
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

    nd1.setOnChangeCb(Node::cbType::preRemoveChild, nullptr, [&] {
        EXPECT_EQ(nd1.children().size(), 1);
        preCalled = true;
    });
    nd1.setOnChangeCb(Node::cbType::postRemoveChild, nullptr, [&] {
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

    nd1.setOnChangeCb(Node::cbType::preRemoveChild, nullptr, [&] {
        EXPECT_EQ(nd1.children().size(), 1);
        preCalled = true;
    });
    nd1.setOnChangeCb(Node::cbType::postRemoveChild, nullptr, [&] {
        EXPECT_EQ(nd1.children().size(), 0);
        postCalled = true;
    });
    child.setOnChangeCb(Node::cbType::preRemoveChild, nullptr, [&] {
        EXPECT_EQ(child.children().size(), 1);
        nestedPreCalled = true;
    });
    child.setOnChangeCb(Node::cbType::postRemoveChild, nullptr, [&] {
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
    node.serializeValues(j);
    std::string s = j.dump();

    EXPECT_EQ(s, "{\"name\":\"Name\",\"typeName\":\"Node\",\"uuid\":\"\"}");
}

TEST(Functional_Node, Serialize_Node) {
    Node node;
    node.setName("Name");

    auto j = node.asJson();
    std::string s = j.dump();
    EXPECT_EQ(s, "{\"name\":\"Name\",\"typeName\":\"Node\",\"uuid\":\"\"}");
}

TEST(Functional_Node, Serialize_Children) {
    Node node;
    node.setName("Name");

    auto& child = node.push<Node>();
    child.setName("Child");
    child.setUuid(""); // reset uuid for testing

    auto j = node.asJson();
    std::string s = j.dump();
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

    auto j = node.asJson();
    std::string s = j.dump();
    EXPECT_EQ(s, "{\"children\":[{\"name\":\"Child1\",\"typeName\":\"Node\",\"uuid\":\"\"},{\"name\":\"Child2\",\"typeName\":\"Node\",\"uuid\":\"\"},{\"name\":\"Child3\",\"typeName\":\"Node\",\"uuid\":\"\"},{\"name\":\"Child4\",\"typeName\":\"Node\",\"uuid\":\"\"}],\"name\":\"Root\",\"typeName\":\"Node\",\"uuid\":\"\"}");
}

TEST(Functional_Node, Parse_Values) {
    auto j = R"({"name":"Name","typeName":"Node","uuid":"6998A327-29B0-90E9-984A-366FEB38BFFE"})"_json;

    Node node;
    node.deserializeValues(j);

    EXPECT_EQ(node.name(), "Name");
    EXPECT_EQ(node.typeName(), "Node");
    EXPECT_EQ(node.uuid(), "6998A327-29B0-90E9-984A-366FEB38BFFE");
}

TEST(Functional_Node, Parse_Node) {
    auto j = R"({"name":"Name","typeName":"Node","uuid":"6998A327-29B0-90E9-984A-366FEB38BFFE"})"_json;

    Node node;
    node.deserialize(j);

    EXPECT_EQ(node.name(), "Name");
    EXPECT_EQ(node.typeName(), "Node");
    EXPECT_EQ(node.uuid(), "6998A327-29B0-90E9-984A-366FEB38BFFE");
}

TEST(Functional_Node, Parse_Children) {
    std::string s = R"({
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
    std::string s = R"({
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
    node.children().front()->setOnChangeCb(Node::cbType::preChange, nullptr, [&]{ triggered = true; });

    std::string s2 = R"({
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

    std::string fn = "Functional_Node_SavingLoading.json";
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
