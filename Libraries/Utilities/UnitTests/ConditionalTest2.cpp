//
// Created by user on 27.04.2021.
//

#include <utility_unit_test_common.h>

using namespace std;

struct GotNotify {
    MOCK_METHOD0(doThing, void());
};

TEST(Utilities_UnitTests, ConditionalTest2) {
    testing::NiceMock<GotNotify> mock;
    ara::Conditional sema;
    int nrLoops = 10;
    EXPECT_CALL(mock, doThing).Times(nrLoops);

    for (int j = 0; j < nrLoops; j++) {
        auto t1 = std::thread([&sema, &mock] {
            // delay the wait
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            sema.wait(0);
            mock.doThing();
        });

        // since the thread needs a bit to be running, the notify may reach before the thread is waiting,
        // notify should avoid the wait to happen. If the test fails a the thread would wait forever

        sema.notify();

        t1.join();
    }
}