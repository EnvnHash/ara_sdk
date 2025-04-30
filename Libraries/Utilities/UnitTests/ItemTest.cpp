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
#include <DataModel/ItemUi.h>

namespace ara {

TEST(Functional_Item, AddChild) {
    Item parent;
    auto child = parent.add<Item>();
    auto addedChild = child->add<Item>();
    ASSERT_NE(addedChild, nullptr);
    EXPECT_EQ(parent.size(), 1u);
}

TEST(Functional_Item, AddChildUniquePtr) {
    Item parent;
    auto child = parent.add(std::make_unique<Item>());
    auto addedChild = child->add(std::make_unique<Item>());
    ASSERT_NE(addedChild, nullptr);
    EXPECT_EQ(parent.size(), 1u);
}

TEST(Functional_Item, AddChildWithName) {
    Item parent;
    auto addedChild = parent.add<Item>("the_item");
    ASSERT_NE(addedChild, nullptr);
    EXPECT_EQ(parent.size(), 1u);
    EXPECT_EQ(parent.children().begin()->get()->name(), "the_item");
}

TEST(Functional_Item, AddNamedChild) {
    Item parent;
    auto namedChild = parent.add<Item>("testName");
    ASSERT_NE(namedChild, nullptr);
    EXPECT_EQ(namedChild->name.get(), "testName");
    EXPECT_EQ(parent.size(), 1u);
}

TEST(Functional_Item, RemoveChild) {
    Item parent;
    auto child = std::make_unique<Item>();
    parent.add(std::move(child));
    parent.remove(parent.children().begin()->get());
    EXPECT_EQ(parent.size(), 0u);
}

TEST(Functional_Item, FindParent) {
    Item grandparent;
    auto parent = grandparent.add<Item>();
    auto child = parent->add<Item>();

    EXPECT_EQ(grandparent.findParent<Item>(), nullptr);
    auto foundParent = child->findParent<Item>();
    ASSERT_NE(foundParent, nullptr);
    EXPECT_EQ(&(*foundParent), &(*parent));
}

TEST(Functional_Item, GetRoot) {
    Item grandparent;
    auto parent = grandparent.add<Item>();
    auto child = parent->add<Item>();

    EXPECT_EQ(grandparent.getRoot(), &grandparent);
    EXPECT_EQ(child->getRoot(), &grandparent);
}

TEST(Functional_Item, Children) {
    Item parent;
    auto child1 =  parent.add(std::make_unique<Item>());
    auto child2 = parent.add(std::make_unique<Item>());

    auto children = parent.children<Item>();
    EXPECT_EQ(children.size(), 2u);
}

TEST(Functional_Item, FindChildren) {
    Item grandparent;
    auto parent = grandparent.add(std::make_unique<Item>());
    auto child1 = parent->add(std::make_unique<Item>());
    auto child2 = parent->add(std::make_unique<Item>());

    auto foundChildren = grandparent.findChildren<Item>();
    EXPECT_EQ(foundChildren.size(), 3u);
}

TEST(Functional_Item, SetTypeName) {
    Item item;
    item.setTypeName<int>();
    EXPECT_EQ(item.itemType.get(), "int");
}

TEST(Functional_Item, SignalItemChanged) {
    Item item;
    bool changed = false;
    auto callback = [&]() { changed = true; };
    item.onItemChanged(&callback, std::move(callback));

    item.signalItemChanged();
    EXPECT_TRUE(changed);
}

TEST(Functional_ItemUi, SavingLoading) {
    ItemUi parent;
    parent.name = "parent";
    parent.displayName = "parent_disp";

    auto child = parent.add<ItemUi>();
    child->name = "child";
    child->displayName = "child_disp";

    auto addedChild = child->add<ItemUi>();
    ASSERT_NE(addedChild, nullptr);
    EXPECT_EQ(parent.size(), 1u);

    std::string fileName = "Functional_Item_SavingTest.json";
    parent.saveAs(fileName);

    ItemUi load_parent;
    load_parent.load(fileName);

    EXPECT_EQ(load_parent.name(), parent.name());
    EXPECT_EQ(load_parent.displayName(), parent.displayName());
    EXPECT_EQ(load_parent.children().size(), 1u);
}

}  // namespace ara
