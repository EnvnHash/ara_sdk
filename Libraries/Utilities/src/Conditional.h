
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

#pragma once

#include <util_common.h>

#ifdef ARA_DEBUG
#define SEMA_CHECK_TIMEOUT
#endif

/** Note: there can be multiple threads with a wait() which will be all release
 * with a single notify() simultaneous notify() calls from multiple threads will
 * unlock at least two consecutive wait() calls and should be avoided!!
 */
namespace ara {

class Conditional {
public:
    void notify() {
        std::unique_lock<std::mutex> lock(mtx);
        flag = true;
        cv.notify_all();
    }

    void wait(uint32_t timeOut_ms = 5000) {
        if (!flag) {
            ++waitingThreads;

            std::unique_lock<std::mutex> lock(mtx);

#ifdef SEMA_CHECK_TIMEOUT
            start = std::chrono::system_clock::now();
#endif
            if (timeOut_ms) {
                cv.wait_for(lock, std::chrono::milliseconds(timeOut_ms), [&] {
#ifdef SEMA_CHECK_TIMEOUT
                    if (timeOut_ms == 5000) {
                        auto end     = std::chrono::system_clock::now();
                        auto actDifF = std::chrono::duration<double, std::milli>(end - start).count();
                        if (actDifF > 5000.0) LOGE << " Semaphore timed out " << actDifF;
                    }
#endif
                    return flag.load();
                });
            } else {
                cv.wait(lock, [&] { return flag.load(); });
            }
            --waitingThreads;
        }

        if (waitingThreads == 0) {
            flag = false;
        }
    }

    bool isNotified() { return flag; }
    void reset() { flag = false; }

private:
    std::mutex              mtx;
    std::condition_variable cv;
    std::atomic<bool>       flag           = {false};
    std::atomic<int>        waitingThreads = {0};

#ifdef SEMA_CHECK_TIMEOUT
    std::chrono::time_point<std::chrono::system_clock> start;
#endif
};

}  // namespace ara