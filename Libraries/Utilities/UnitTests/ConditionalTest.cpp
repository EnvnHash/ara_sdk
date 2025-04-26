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

using namespace std;

struct GotNotify {
    MOCK_METHOD0(doThing, void());
};

using testing::_;

TEST(Utilities_UnitTests, ConditionalTest) {
    testing::NiceMock<GotNotify> mock;
    ara::Conditional sema;
    int nrThreads = 20;
    int nrLoops = 5;
    std::atomic<int> readyThreads = {0};

    EXPECT_CALL(mock, doThing).Times(nrThreads * nrLoops);

    std::list<std::thread> threads;

    // all threads received the notify?
    for (int j = 0; j < nrLoops; j++) {
        for (int i = 0; i < nrThreads; i++)
            threads.push_back(std::thread([&sema, &mock, &readyThreads] {
                readyThreads++;
                sema.wait(0);
                mock.doThing();
            }));

        // thread need a bit to be running, be sure all threads are waiting
        while (readyThreads < nrThreads) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        sema.notify();

        for (auto &it: threads) {
            it.join();
        }

        threads.clear();
        readyThreads = 0;
    }
}

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

TEST(Utilities_UnitTests, ConditionalTest3) {
    testing::NiceMock<GotNotify> mock;
    ara::Conditional sema;
    std::atomic<bool> threadRunning = false;

    for (int i = 0; i < 5; i++) {
        EXPECT_CALL(mock, doThing).Times(2);

        // check if the notify should only unblock the first wait
        int nrNotifies = 100;
        for (int j = 0; j < nrNotifies; j++) {
            sema.notify();
        }

        auto t1 = std::thread([&sema, &mock, &threadRunning] {
            threadRunning = true;
            sema.wait(0);
            mock.doThing();
        });

        // be sure thread is running
        while (!threadRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        t1.join();

        sema.reset();
        auto start = std::chrono::system_clock::now();

        auto t2 = std::thread([&sema, &mock] {
            sema.wait(100);
            mock.doThing();
        });
        t2.join();

        auto end = std::chrono::system_clock::now();
        auto actDifF = std::chrono::duration<double, std::milli>(end - start).count();
        EXPECT_GT(actDifF, 100); // due to rounding errors diff may be 249 or something, put a lower val to be sure
    }
}