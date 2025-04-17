//
// Created by sven on 18-10-22.
//

#ifdef __ANDROID__

#pragma once

#include <Android/cardboard/DeviceParams.h>
#include <Android/cardboard/ScreenParams.h>
#include <Android/cardboard/cardboard_common.h>
#include <util_common.h>

namespace ara::cardboard {

class Cardboard {
public:
    ~Cardboard() {
        if (m_env && m_global_context) m_env->DeleteGlobalRef(m_global_context);
    }
    void init(void* cmd_data);

private:
    JNIEnv*      m_env            = nullptr;
    jobject      m_global_context = nullptr;
    ScreenParams m_screenParams;
    DeviceParams m_deviceParams;
};

}  // namespace ara::cardboard

#endif