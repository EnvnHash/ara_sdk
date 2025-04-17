//
// Created by sven on 02-04-25.
//

#include <utility_unit_test_common.h>
#include "ListProperty.h"

namespace ara {

class ListPropertyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup the list property with some initial parameters
    }

    void TearDown() override {
        // Clear the list property after each test
        lp.clear();
    }

    ListProperty<int> lp;
};

TEST_F(ListPropertyTest, AssignmentFromInitializerList) {
    lp = {1, 2, 3};
    EXPECT_EQ(lp.size(), 3);
    EXPECT_EQ(lp.front(), 1);
    EXPECT_EQ(lp.back(), 3);
}

TEST_F(ListPropertyTest, AssignmentFromStdList) {
    std::list<int> lst = {4, 5, 6};
    lp = lst;
    EXPECT_EQ(lp.size(), 3);
    EXPECT_EQ(lp.front(), 4);
    EXPECT_EQ(lp.back(), 6);
}

TEST_F(ListPropertyTest, PushBack) {
    lp.push_back(10);
    lp.push_back(20);
    lp.push_back(30);
    EXPECT_EQ(lp.size(), 3);
    EXPECT_EQ(lp.front(), 10);
    EXPECT_EQ(lp.back(), 30);

    // Test with doProcCb = false
    lp.push_back(40, false);
    EXPECT_EQ(lp.back(), 40);
}

TEST_F(ListPropertyTest, PopBack) {
    lp.push_back(50);
    lp.push_back(60);
    lp.pop_back();
    EXPECT_EQ(lp.size(), 1);
    EXPECT_EQ(lp.front(), 50);

    // Test with doProcCb = false
    lp.pop_back(false);
    EXPECT_TRUE(lp.empty());
}

TEST_F(ListPropertyTest, EmplaceBack) {
    lp.emplace_back(70);
    lp.emplace_back(80);
    lp.emplace_back(90);
    EXPECT_EQ(lp.size(), 3);
    EXPECT_EQ(lp.front(), 70);
    EXPECT_EQ(lp.back(), 90);

    // Test with doProcCb = false
    lp.emplace_back(100, false);
    EXPECT_EQ(lp.back(), 100);
}

TEST_F(ListPropertyTest, Clear) {
    lp.push_back(110);
    lp.push_back(120);
    lp.clear();
    EXPECT_TRUE(lp.empty());
}

TEST_F(ListPropertyTest, Insert) {
    lp.push_back(130);
    auto it = lp.begin();
    lp.insert(it, 140);
    EXPECT_EQ(lp.front(), 140);
    EXPECT_EQ(lp.size(), 2);
}

TEST_F(ListPropertyTest, CallbacksWithSharedPtr) {
    auto funcPtr = std::make_shared<std::function<void(std::any)>>([](std::any listPtr) {
                auto lst = std::any_cast<std::list<int>*>(listPtr);
                EXPECT_EQ(lst->size(), 1);
                EXPECT_EQ(lst->front(), 150);
            });

    lp.onChanged(funcPtr);
    lp.push_back(150); // This should trigger the callback
}

TEST_F(ListPropertyTest, CallbacksWithFunction) {
    auto func = [](std::list<int> *listPtr) {
        EXPECT_EQ(listPtr->size(), 1);
        EXPECT_EQ(listPtr->front(), 160);
    };

    lp.onChanged(func, this);
    lp.push_back(160); // This should trigger the callback
}

TEST_F(ListPropertyTest, RemoveCallback) {
    auto func = [](std::list<int> *listPtr) {
        EXPECT_EQ(listPtr->size(), 1);
        EXPECT_EQ(listPtr->front(), 170);
    };

    lp.onChanged(func, this);
    lp.push_back(170); // This should trigger the callback
    lp.removeOnValueChanged(this);
    lp.push_back(180); // This should not trigger the callback again
}

}  // namespace ara
