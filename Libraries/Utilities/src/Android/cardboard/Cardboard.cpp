//
// Created by sven on 18-10-22.
//

#ifdef __ANDROID__

#include "Android/cardboard/Cardboard.h"

#include "Android/cardboard/ScreenParams.h"

namespace ara::cardboard {

void Cardboard::init(void* cmd_data) {
    if (!cmd_data) return;
    /*
        auto cmd = static_cast<android_cmd_data*>(cmd_data);

        m_env = cmd->env;
        m_global_context = env->NewGlobalRef(cmd->context);

        m_screenParams.init(cmd->env, m_global_context);
        m_deviceParams.init(cmd->env, m_global_context);*/
}

}  // namespace ara::cardboard

#endif