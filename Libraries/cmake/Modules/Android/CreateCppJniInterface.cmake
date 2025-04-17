macro (create_cpp_jni_interface)

    FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/cpp/jni_interface.h "#pragma once

#include <jni.h>

/**
 * Helper functions to provide access to Java from C via JNI.
 */
extern \"C\" {

// Helper function used to access the jni environment on the current thread.
// In this sample, no consideration is made for detaching the thread when the
// thread exits. This can cause memory leaks, so production applications should
// detach when the thread no longer needs access to the JVM.
JNIEnv *GetJniEnv();

jclass FindClass(const char *classname);

}  // extern \"C\"
\n\n")

    # package name may contain a "_" char, which will translate to "_1" in a ndk method name
    string(REPLACE "_" "_1" PACKAGE_NAME_CLASS ${PACKAGE_NAME})

    FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/cpp/jni_interface.cpp "#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>
#include \"${UIAPP_DERIVATE_CLASS_FILE_NAME_WE}.h\"

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL Java_eu_zeitkunst_${PACKAGE_NAME_CLASS}_JniInterface_##method_name

extern \"C\" {

namespace {
// maintain a reference to the JVM so we can use it later.
static JavaVM *g_vm = nullptr;
static ara::${UIAPP_DERIVATE_CLASS} *app = nullptr;
static float mpX = 0;
static float mpY = 0;
static jobject g_actObj=nullptr;

inline jlong jptr(ara::${UIAPP_DERIVATE_CLASS} *native_uiapp) {
  return reinterpret_cast<intptr_t>(native_uiapp);
}

}  // namespace


jint JNI_OnLoad(JavaVM *vm, void *) {
    g_vm = vm;
    return JNI_VERSION_1_6;
}

JNI_METHOD(jlong, createNativeApplication)
(JNIEnv* env, jobject obj, jobject j_asset_manager, jstring internalDataPath) {
    app = new ara::${UIAPP_DERIVATE_CLASS}();
    jboolean isCopy = true;
    const char *cstr = env->GetStringUTFChars(internalDataPath, &isCopy);
    app->m_internalPath = std::string(cstr);

    g_actObj = obj;

    //app->m_asset_manager = AAssetManager_fromJava(env, j_asset_manager);
    //app->m_cmd_data.context = env->NewGlobalRef(obj);
    //env->ReleaseStringUTFChars(path, cstr);

    app->m_cmd_data.oriCb = [&](int ori){
        JNIEnv* env;
        if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) return; // JNI version not supported.

        jclass clz = env->FindClass(\"eu/zeitkunst/${UIAPP_DERIVATE_CLASS_FILE_NAME_WE}/${UIAPP_DERIVATE_CLASS_FILE_NAME_WE}Activity\");
        jmethodID jniFixOri = env->GetStaticMethodID(clz, \"fixOrientation\", \"(I)V\"\);
        env->CallStaticVoidMethod(clz, jniFixOri, 1);
    };

    app->m_cmd_data.resetOri = [&]{
        JNIEnv* env;
        if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) return; // JNI version not supported.

        jclass clz = env->FindClass(\"eu/zeitkunst/${UIAPP_DERIVATE_CLASS_FILE_NAME_WE}/${UIAPP_DERIVATE_CLASS_FILE_NAME_WE}Activity\");
        jmethodID jniResOri = env->GetStaticMethodID(clz, \"resetOrientation\", \"()V\"\);
        env->CallStaticVoidMethod(clz, jniResOri);
    };

    return jptr(app);
}

JNI_METHOD(void, setExternalDataPath)
(JNIEnv* env, jclass, jstring path) {
    const char *cstr = env->GetStringUTFChars(path, NULL);
    std::string str = std::string(cstr);
    if (app) app->setExternalDataPath(str);
    env->ReleaseStringUTFChars(path, cstr);
}

JNI_METHOD(void, setDisplayDensity)
(JNIEnv* env, jclass, float density, float w, float h, float xdpi, float ydpi) {
    if (app) app->setDisplayDensity(density, w, h, xdpi, ydpi);
}

JNI_METHOD(void, destroyNativeApplication)
(JNIEnv*, jclass) {
  if (app) delete app;
}

JNI_METHOD(void, onStart)
(JNIEnv*, jclass) {
  if (app) app->OnStart();
}

JNI_METHOD(void, onPause)
(JNIEnv*, jclass) {
  if (app) app->OnPause();
}

JNI_METHOD(void, onResume)
(JNIEnv* env, jobject obj, jobject context, jobject activity) {
  if (app) app->OnResume(env, context, activity);
//  if (app) app->OnResume(env, obj, activity);
}

JNI_METHOD(void, onGlSurfaceCreated)
(JNIEnv* env, jclass) {
  if (app) app->OnSurfaceCreated(env);
}

JNI_METHOD(void, onDisplayGeometryChanged)
(JNIEnv*, jobject, int display_rotation, int width, int height) {
  if (app) app->OnDisplayGeometryChanged(display_rotation, width, height);
}

JNI_METHOD(void, onGlSurfaceDrawFrame)
(JNIEnv*, jclass) {
  if (app) app->OnDrawFrame();
}

JNI_METHOD(void, onTouched)
(JNIEnv*, jclass, jfloat x, jfloat y) {
    if (app) {
        app->OnTouched(x, y);

        auto win = app->getMainWindow();
        // convert to virtual coordinates
        mpX = x / app->getDisplayDensity();
        mpY = y / app->getDisplayDensity();
        win->osMouseMove(mpX, mpY, 0);
        win->osMouseUpLeft();
    }
}

JNI_METHOD(void, onTouchDown)
(JNIEnv*, jclass, jfloat x, jfloat y) {
    if (app) {
        auto win = app->getMainWindow();
        // convert to virtual coordinates
        mpX = x / app->getDisplayDensity();
        mpY = y / app->getDisplayDensity();
        win->osMouseMove(mpX, mpY, 0);
        win->osMouseDownLeft(mpX, mpY, false, false, false);
    }
}

JNI_METHOD(void, onScroll)
(JNIEnv*, jclass, jfloat x, jfloat y) {
    if (app) {
        auto win = app->getMainWindow();
        // convert to virtual coordinates
        mpX = x / app->getDisplayDensity();
        mpY = y / app->getDisplayDensity();
        win->osMouseMove(mpX, mpY, 0);
    }
}

JNIEnv* GetJniEnv() {
  JNIEnv *env;
  jint result = g_vm->AttachCurrentThread(&env, nullptr);
  return result == JNI_OK ? env : nullptr;
}

jclass FindClass(const char* classname) {
  JNIEnv *env = GetJniEnv();
  return env->FindClass(classname);
}

}  // extern \"C\"")

endmacro()
