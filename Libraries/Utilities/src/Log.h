
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

#include <functional>
#include <iostream>
#include <sstream>

#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace ara {

class Log {
public:
    std::ostringstream &GetStream() {
        return m_stringStream;
    }

    std::ostringstream &GetStreamN() {
        addReturn = false;
        return m_stringStream;
    }

    std::ostringstream &GetStreamE() {
        err = true;
        return m_stringStream;
    }

    ~Log() {
#ifdef __ANDROID__
        // stupid restriction of max 4k log length
        std::string ssStr      = m_stringStream.str();
        int         chunkCount = (int)std::ceil(static_cast<float>(ssStr.size()) / 1000.f);
        for (int i = 0, offs = 0; i < chunkCount; i++, offs += 1000) {
            if (err) {
                __android_log_print(
                    ANDROID_LOG_ERROR, "native-activity", "%s",
                    (ssStr.substr(offs, std::min<size_t>(1000, ssStr.size() - offs)) + (addReturn ? "\n" : ""))
                        .c_str());
            } else {
                __android_log_print(
                    ANDROID_LOG_INFO, "native-activity", "%s",
                    (ssStr.substr(offs, std::min<size_t>(1000, ssStr.size() - offs)) + (addReturn ? "\n" : ""))
                        .c_str());
            }
        }
#else
        if (err) {
            std::cerr << m_stringStream.str() << std::endl;
        } else {
            std::cout << m_stringStream.str() << std::endl;
        }
#endif
        if (g_logCb) {
            g_logCb(m_stringStream, err);
        }
    }

    inline static std::function<void(std::ostringstream &, bool)> g_logCb = nullptr;

private:
    std::ostringstream m_stringStream;
    bool               err       = false;
    bool               addReturn = true;
};

// simple logging
#ifdef LOG
#undef LOG
#endif
#ifdef LOGN
#undef LOGN
#endif
#ifdef LOGE
#undef LOGE
#endif

#define LOG ara::Log().GetStream()
#define LOGN ara::Log().GetStreamN()
#define LOGE ara::Log().GetStreamE()
}  // namespace ara