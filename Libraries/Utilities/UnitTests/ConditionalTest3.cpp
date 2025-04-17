//
// Created by user on 27.04.2021.
//

#include <utility_unit_test_common.h>

using namespace std;

struct GotNotify {
    MOCK_METHOD0(doThing, void());
};

using testing::_;


TEST(Utilities_UnitTests, ConditionalTest3) {
    testing::NiceMock<GotNotify> mock;
    ara::Conditional sema;
    std::atomic<bool> threadRunning = false;

    for (int i = 0; i < 5; i++) {

        EXPECT_CALL(mock, doThing).Times(2);

        // check if the notify should only unblock the first wait

        int nrNotifies = 100;
        for (int j = 0; j < nrNotifies; j++)
            sema.notify();

        auto t1 = std::thread([&sema, &mock, &threadRunning] {
            threadRunning = true;
            sema.wait(0);
            mock.doThing();
        });

        // be sure thread is running
        while (!threadRunning)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        t1.join();

        auto start = std::chrono::system_clock::now();

        auto t2 = std::thread([&sema, &mock] {
            sema.wait(100);
            mock.doThing();
        });
        t2.join();

        auto end = std::chrono::system_clock::now();
        auto actDifF = std::chrono::duration<double, std::milli>(end - start).count();
        EXPECT_GT(actDifF, 100); // due to rouding error diff may be 249 or something, put a lower val to be sure

    }

}