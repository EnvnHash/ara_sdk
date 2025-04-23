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

    // all threads receive the notify?

    for (int j = 0; j < nrLoops; j++) {
        for (int i = 0; i < nrThreads; i++)
            threads.push_back(std::thread([&sema, &mock, &readyThreads] {
                readyThreads++;
                sema.wait(0);
                mock.doThing();
            }));

        // thread need a bit to be running, be sure all threads are waiting
        while (readyThreads < nrThreads)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        sema.notify();

        for (auto &it: threads)
            it.join();

        threads.clear();
        readyThreads = 0;
    }

}