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
#include <Property.h>

using namespace std;

struct OnValMock {
    MOCK_METHOD0(doThing, void());
};

using testing::_;

TEST(Functional_Property, PropertyTest) {
    testing::NiceMock<OnValMock> mock;

    ara::Property<int> prop;
    prop = 20;
    EXPECT_EQ(20, prop());

    ara::Property<float> prop2;
    EXPECT_CALL(mock, doThing).Times(1);
    prop2.onPreChange([&mock](const float &val) {
        mock.doThing();
    }, (void *) this);
    prop2 = 2.f;
    EXPECT_TRUE(2.f == prop2());

    ara::Property<std::vector<int>> prop3;
    prop3().push_back(20);
    EXPECT_TRUE(1 == prop3().size());
    EXPECT_TRUE(prop3().back() == 20);
}

TEST(Functional_Property, InitialValue) {
    ara::Property<int> prop(42);
    EXPECT_EQ(prop(), 42);
}

TEST(Functional_Property, SetValue) {
    ara::Property<int> prop;
    prop = 10;
    EXPECT_EQ(prop(), 10);
}

TEST(Functional_Property, ClampValue) {
    ara::Property<int> prop(5, 1, 10, 1);
    prop.setClamp(20);
    EXPECT_EQ(prop(), 10);

    prop.setClamp(0);
    EXPECT_EQ(prop(), 1);
}

TEST(Functional_Property, StepValue) {
    ara::Property<int> prop;
    prop.setStep(5);
    prop = 2;
    EXPECT_EQ(prop.get(), 2); // No change since it's less than the step
}

TEST(Functional_Property, OnPreChangeCallback) {
    bool callbackCalled = false;

    ara::Property<int> prop(0, 0, 10, 1);
    auto f = [&callbackCalled](std::any value) {
        int val = std::any_cast<int>(value);
        if (val == 5) callbackCalled = true;
    };
    auto sh = std::make_shared<std::function<void(std::any)>>(f);
    prop.onPreChange(sh);

    prop = 5;
    EXPECT_TRUE(callbackCalled);
}

TEST(Functional_Property, OnPostChangeCallback) {
    bool callbackCalled = false;
    auto postChangeFunc = [&callbackCalled](const std::any& value) {
        callbackCalled = true;
    };
    auto sh = std::make_shared<std::function<void(std::any)>>(postChangeFunc);

    ara::Property<int> prop(0, 0, 10, 1);
    prop.onPostChange(sh);

    prop.set(5);
    EXPECT_TRUE(callbackCalled);
}

TEST(Functional_PropertyPtr, InitialValue) {
    auto prop = std::make_unique<ara::Property<int>>(42);
    ara::PropertyPtr<int> propPtr(prop.get());
    EXPECT_EQ(propPtr.get(), 42);
}

TEST(Functional_PropertyPtr, SetValue) {
    auto prop = std::make_unique<ara::Property<int>>();
    ara::PropertyPtr<int> propPtr(prop.get());
    propPtr = 10;
    EXPECT_EQ(propPtr(), 10);
}

TEST(Functional_PropertyPtr, StepValue) {
    auto prop = std::make_unique<ara::Property<int>>();
    ara::PropertyPtr<int> propPtr(prop.get());

    propPtr.setStep(5);
    propPtr = 2;
    EXPECT_EQ(propPtr(), 2); // No change since it's less than the step
}

TEST(Functional_PropertyPtr, OnPreChangeCallback) {
    bool callbackCalled = false;
    auto preChangeFunc = [&callbackCalled](std::any value) {
        int val = std::any_cast<int>(value);
        if (val == 5) callbackCalled = true;
    };

    auto prop = std::make_unique<ara::Property<int>>();
    ara::PropertyPtr<int> propPtr(prop.get());

    auto sh = std::make_shared<std::function<void(std::any)>>(preChangeFunc);
    propPtr.onPreChange(sh);
    propPtr = 5;
    EXPECT_TRUE(callbackCalled);
}

TEST(Functional_PropertyPtr, OnPostChangeCallback) {
    bool callbackCalled = false;
    auto postChangeFunc = [&callbackCalled](const std::any& value) {
        callbackCalled = true;
    };

    auto prop = std::make_unique<ara::Property<int>>();
    ara::PropertyPtr<int> propPtr(prop.get());

    auto sh = std::make_shared<std::function<void(std::any)>>(postChangeFunc);
    propPtr.onPostChange(sh);

    propPtr = 5;
    EXPECT_TRUE(callbackCalled);
}

TEST(Functional_PropertyPtr, ToJson) {
    ara::Property<int> prop(60, 0, 1, 2);

    // conversion: Property -> json
    nlohmann::json j = prop;

    // conversion: json -> Property
    auto p2 = j.template get<ara::Property<int>>();

    // that's it
    EXPECT_EQ(prop(), p2());
    EXPECT_EQ(prop.getStep(), p2.getStep());
    EXPECT_EQ(prop.getMin(), p2.getMin());
    EXPECT_EQ(prop.getMax(), p2.getMax());
}
