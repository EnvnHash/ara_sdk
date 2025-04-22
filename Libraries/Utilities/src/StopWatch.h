/* StopWatch.h **Created on : Jul 18, 2018 * Author : sven */

#pragma once

#include <util_common.h>

namespace ara {
class StopWatch {
public:
    StopWatch() : dt(0.0), showDecTime(2000.0) {}
    ~StopWatch() = default;

    void setStart() {
        start    = std::chrono::system_clock::now();
        startSet = true;
    }

    void setEnd() {
        if (!startSet) {
            return;
        }
        end = std::chrono::system_clock::now();

        actDifF  = std::chrono::duration<double, std::milli>(end - start).count();
        dt       = dt == 0.0 ? actDifF : ((dt * med) + actDifF) / (med + 1.0);
        startSet = false;
    }

    void print(const char *pre, bool direct = false) {
        if ((dt != 0.0 &&
             (std::chrono::duration<double, std::milli>(std::chrono::system_clock::now() - lastPrintTime).count() >=
              showDecTime)) ||
            direct) {
            std::cout << pre << " time: " << dt << " ms  " << 1.0 / dt * 1000.f << " fps" << std::endl;
            lastPrintTime = std::chrono::system_clock::now();
        }
    }

    [[nodiscard]] double getDt() const { return dt; }
    void   setMed(double val) { med = val; }

    std::chrono::time_point<std::chrono::system_clock> start;
    std::chrono::time_point<std::chrono::system_clock> end;
    std::chrono::time_point<std::chrono::system_clock> lastPrintTime;

    bool   startSet    = false;
    double dt          = 0.0;
    double med         = 10.0;
    double actDifF     = 0.0;
    double showDecTime = 0.0;
};
}  // namespace ara