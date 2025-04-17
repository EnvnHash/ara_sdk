macro (create_pure_native_app_source)

    # create the native-lib.cpp
    FILE(WRITE  ${ANDROID_STUDIO_PROJ}/app/src/main/cpp/native-lib.cpp
            "#include <jni.h>
#include <string>
#include <stdint.h>
#include <dlfcn.h>
#include <android/native_window.h> // requires ndk r5 or newer
#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/sensor.h>
#include <android/window.h>

#include \"${UIAPP_DERIVATE_CLASS_FILE_NAME_WE}.h\"


/*
* AcquireASensorManagerInstance(void)
*    Workaround ASensorManager_getInstance() deprecation false alarm
*    for Android-N and before, when compiling with NDK-r15
*/
ASensorManager* AcquireASensorManagerInstance(android_app* app)
{
    if(!app)
        return nullptr;

    typedef ASensorManager *(*PF_GETINSTANCEFORPACKAGE)(const char *name);
    void* androidHandle = dlopen(\"libandroid.so\", RTLD_NOW);
    auto getInstanceForPackageFunc = (PF_GETINSTANCEFORPACKAGE) dlsym(androidHandle, \"ASensorManager_getInstanceForPackage\");

    if (getInstanceForPackageFunc)
    {
        JNIEnv* env = nullptr;
        app->activity->vm->AttachCurrentThread(&env, nullptr);

        jclass android_content_Context = env->GetObjectClass(app->activity->clazz);
        jmethodID midGetPackageName = env->GetMethodID(android_content_Context, \"getPackageName\",  \"()Ljava/lang/String;\");
        auto packageName= (jstring)env->CallObjectMethod(app->activity->clazz, midGetPackageName);

        const char *nativePackageName = env->GetStringUTFChars(packageName, nullptr);
        ASensorManager* mgr = getInstanceForPackageFunc(nativePackageName);
        env->ReleaseStringUTFChars(packageName, nativePackageName);
        app->activity->vm->DetachCurrentThread();
        if (mgr) {
            dlclose(androidHandle);
            return mgr;
        }
    }

    typedef ASensorManager *(*PF_GETINSTANCE)();
    auto getInstanceFunc = (PF_GETINSTANCE)  dlsym(androidHandle, \"ASensorManager_getInstance\");
    // by all means at this point, ASensorManager_getInstance should be available
    assert(getInstanceFunc);
    dlclose(androidHandle);

    return getInstanceFunc();
}

//    Main entrypoint for Android application
void android_main(struct android_app* app)
{
    ${UIAPP_DERIVATE_CLASS}* ui_app = new ${UIAPP_DERIVATE_CLASS}();
    ui_app->setAndroidApp(app);
    app->userData = ui_app;
    app->onAppCmd = &ui_app->handle_cmd;
    app->onInputEvent = &ui_app->handle_input;

    // Prepare to monitor accelerometer
    // ui_app.m_sensorManager = AcquireASensorManagerInstance(app);
    // ui_app.m_accelerometerSensor = ASensorManager_getDefaultSensor(ui_app.m_sensorManager, ASENSOR_TYPE_ACCELEROMETER);
    // ui_app.m_sensorEventQueue = ASensorManager_createEventQueue(ui_app.m_sensorManager, app->looper, LOOPER_ID_USER, nullptr, nullptr);
    ui_app->m_internalPath = app->activity->internalDataPath;

    if (app->savedState != nullptr) {
        // We are starting with a previous saved state; restore from it.
        ui_app->m_saved_state = *(ara::android_app_state*)app->savedState;
    }

    ui_app->startAndroidEventLoop(); // blocking
    LOG << \"android_main exit !!!\";
    delete ui_app;
}") # write it

endmacro()
