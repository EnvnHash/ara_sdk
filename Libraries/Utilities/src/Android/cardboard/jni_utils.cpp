//
// Created by sven on 18-10-22.
//

#ifdef __ANDROID__

#include "Android/cardboard/jni_utils.h"

#include "Log.h"

namespace ara::cardboard {

jclass runtime_excepton_class_;

void LoadJNIResources(JNIEnv* env) { runtime_excepton_class_ = LoadJClass(env, "java/lang/RuntimeException"); }

void initializeAndroid(JavaVM* vm, jobject /*context*/) {
    JNIEnv* env;
    LoadJNIEnv(vm, &env);
    LoadJNIResources(env);
}

bool CheckExceptionInJava(JNIEnv* env) {
    const bool exception_occurred = env->ExceptionOccurred();
    if (exception_occurred) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    return exception_occurred;
}

void LoadJNIEnv(JavaVM* vm, JNIEnv** env) {
    switch (vm->GetEnv(reinterpret_cast<void**>(env), JNI_VERSION_1_6)) {
        case JNI_OK: break;
        case JNI_EDETACHED:
            if (vm->AttachCurrentThread(env, nullptr) != 0) {
                *env = nullptr;
            }
            break;
        default: *env = nullptr; break;
    }
}

jclass LoadJClass(JNIEnv* env, const char* class_name) {
    jclass local = env->FindClass(class_name);
    CheckExceptionInJava(env);
    return static_cast<jclass>(env->NewGlobalRef(local));
}

void ThrowJavaRuntimeException(JNIEnv* env, const char* msg) {
    LOGE << "Throw Java RuntimeException: " << msg;
    env->ThrowNew(runtime_excepton_class_, msg);
}

}  // namespace ara::cardboard

#endif