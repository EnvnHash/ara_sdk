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