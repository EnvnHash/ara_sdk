//
// Created by sven on 05-07-22.
//
#ifdef __ANDROID__

#include "Android/UIAppAndroidNative.h"


#include <UIElements/UINodeBase/UINode.h>
#include "UIApplication.h"

namespace ara {

UIAppAndroidNative::UIAppAndroidNative() : UIApplicationBase() {
    m_threadedWindowRendering = false;
}

void UIAppAndroidNative::startAndroidEventLoop() {
    // Read all pending events.
    int                         events;
    struct android_poll_source* source;
    GLWindow*                   win = nullptr;

    while (true) {
        // timeout == -1 means blocking until a new event comes
        // a return value of -1 == ALOOPER_POLL_WAKE, i.e. forceRedraw
        while (ALooper_pollOnce(-1, nullptr, &events, reinterpret_cast<void**>(&source)) >= -1) {
            if (source != nullptr) {
                source->process(m_androidApp, source);
            }

            // Check if we are exiting.
            if (m_androidApp->destroyRequested != 0) {
                // engine_term_display(&engine);
                return;
            }

            // pass alooper to main window
            if (m_glbase.getWinMan()->getWindows()->size() > 1) {
                win = (m_glbase.getWinMan()->getWindows()->begin() +1)->get();
                if (win) {
                    win->setALooper(ALooper_forThread());
                }
            }
            if (win) {
                update();
            }
        }
    }
}

int32_t UIAppAndroidNative::handle_input(struct android_app* app, AInputEvent* event) {
    auto ctx = static_cast<UIAppAndroidNative*>(app->userData);
    if (ctx->getWinMan()->getWindows()->size() <= 1) {
        return -1;
    }

    switch (AInputEvent_getType(event)) {
        case AINPUT_EVENT_TYPE_MOTION: {
            float x   = AMotionEvent_getX(event, 0);
            float y   = AMotionEvent_getY(event, 0);
            auto win = (ctx->getWinMan()->getWindows()->begin() +1)->get();

            if (!win) {
                return -1;
            }

            // convert to virtual coordinates
            x /= win->getContentScale().x;
            y /= win->getContentScale().y;

            switch (AInputEvent_getSource(event)) {
                case AINPUT_SOURCE_TOUCHSCREEN: {
                    int action = AKeyEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
                    switch (action) {
                        case AMOTION_EVENT_ACTION_DOWN:
                            win->addEventToQueue([win, x, y] {
                                win->onMouseCursor(x, y);
                                win->onMouseButton(GLSG_MOUSE_BUTTON_LEFT, GLSG_PRESS, 0);
                                return true;
                            });
                            win->iterate();
                            break;
                        case AMOTION_EVENT_ACTION_UP:
                            win->addEventToQueue([win, x, y] {
                                win->onMouseCursor(x, y);
                                win->onMouseButton(GLSG_MOUSE_BUTTON_LEFT, GLSG_RELEASE, 0);
                                return true;
                            });
                            win->iterate();
                            break;
                        case AMOTION_EVENT_ACTION_MOVE:
                            win->addEventToQueue([win, x, y] {
                                win->onMouseCursor(x, y);
                                return true;
                            });
                            win->iterate();
                            break;
                    }
                } break;
                default: break;
            }  // end switch
        } break;
        case AINPUT_EVENT_TYPE_KEY:
            // handle key input...
            break;
    }  // end AInputEvent_getType switch

    return 0;
}

void UIAppAndroidNative::handle_cmd(struct android_app* app, int32_t cmd) {
    auto ctx = static_cast<UIAppAndroidNative*>(app->userData);

    if (!ctx) {
        return;
    }
    if (!ctx->m_androidApp) {
        return;
    }

    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            ctx->m_androidApp->savedState                        = malloc(sizeof(android_app_state));
            *static_cast<android_app_state*>(ctx->m_androidApp->savedState) = ctx->m_saved_state;
            ctx->m_androidApp->savedStateSize                    = sizeof(android_app_state);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (ctx->m_androidApp->window && !ctx->m_inited) {
                m_hasWindow                         = true;
                ctx->m_androidNativeWin             = ctx->m_androidApp->window;
                float density                       = ctx->get_density(ANativeWindow_getWidth(ctx->m_androidApp->window),
                                                                      ANativeWindow_getHeight(ctx->m_androidApp->window));
                ctx->getGLBase()->g_androidDensity  = density;
                ctx->getGLBase()->g_androidDpi      = { ctx->m_xdpi, ctx->m_ydpi };

                // context is not ready at this point???
                static_cast<UIApplication*>(app->userData)->init(nullptr);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            break;
        case APP_CMD_GAINED_FOCUS:
            m_hasFocus = true;
            break;
        case APP_CMD_LOST_FOCUS:
            m_hasFocus = false;
            break;
        case APP_CMD_INPUT_CHANGED: break;
        case APP_CMD_WINDOW_RESIZED: break;
        case APP_CMD_WINDOW_REDRAW_NEEDED: break;
        case APP_CMD_CONTENT_RECT_CHANGED: {
            auto win = ctx->getWinMan()->getFirstWin();
            if (win) {
                win->checkSize();
            }
        } break;
        case APP_CMD_CONFIG_CHANGED: {
        } break;
        case APP_CMD_LOW_MEMORY: break;
        case APP_CMD_START:
            for (const auto& it : ctx->m_appStateCbs[android_app_cmd::onStart]) {
                it(&ctx->m_cmd_data);
            }
            m_isVisible = true;
            break;
        case APP_CMD_RESUME:
            for (const auto& it : ctx->m_appStateCbs[android_app_cmd::onResume]) {
                it(&ctx->m_cmd_data);
            }
            break;
        case APP_CMD_PAUSE:
            for (const auto& it : ctx->m_appStateCbs[android_app_cmd::onPause]) {
                it(&ctx->m_cmd_data);
            }
            m_isVisible = false;
            break;
        case APP_CMD_STOP:
            for (const auto& it : ctx->m_appStateCbs[android_app_cmd::onStop]) {
                it(&ctx->m_cmd_data);
            }
            break;
        case APP_CMD_DESTROY: break;
        default: break;
    }
}

int32_t UIAppAndroidNative::get_orientation() {
    if (!m_androidApp) return -1;

    JNIEnv* jni = nullptr;
    m_androidApp->activity->vm->AttachCurrentThread(&jni, nullptr);
    if (jni) {
        auto classNativeActivity = jni->FindClass("android/app/NativeActivity");
        auto classWindowManager  = jni->FindClass("android/view/WindowManager");
        auto classDisplay        = jni->FindClass("android/view/Display");
        if (classWindowManager) {
            auto idNativeActivity_getWindowManager =
                jni->GetMethodID(classNativeActivity, "getWindowManager", "()Landroid/view/WindowManager;");
            auto idWindowManager_getDefaultDisplay =
                jni->GetMethodID(classWindowManager, "getDefaultDisplay", "()Landroid/view/Display;");
            jmethodID idWindowManager_getRotation = jni->GetMethodID(classDisplay, "getRotation", "()I");
            if (idWindowManager_getRotation) {
                auto windowManager =
                    jni->CallObjectMethod(m_androidApp->activity->clazz, idNativeActivity_getWindowManager);
                if (windowManager) {
                    auto display = jni->CallObjectMethod(windowManager, idWindowManager_getDefaultDisplay);
                    if (display) {
                        int rotation = jni->CallIntMethod(display, idWindowManager_getRotation);

                        if (rotation == 0 || rotation == 2) {
                            LOG << "default screen rotation is PORTRAIT";
                        } else {
                            LOG << "default screen rotation is LANDSCAPE";
                        }

                        m_androidApp->activity->vm->DetachCurrentThread();
                        return rotation;
                    }
                }
            }
        }

        m_androidApp->activity->vm->DetachCurrentThread();
    }
    return -1;
}

float UIAppAndroidNative::get_density(float width_pixels, float height_pixels) {
    if (!m_androidApp) {
        return -1;
    }

    JNIEnv* jni     = nullptr;
    float   density = 0;
    m_androidApp->activity->vm->AttachCurrentThread(&jni, nullptr);
    if (jni) {
        auto classNativeActivity = jni->FindClass("android/app/NativeActivity");
        auto classWindowManager  = jni->FindClass("android/view/WindowManager");
        auto classDisplay        = jni->FindClass("android/view/Display");
        auto displayMetricsClass = jni->FindClass("android/util/DisplayMetrics");

        if (classWindowManager) {
            auto getWindowManager =
                jni->GetMethodID(classNativeActivity, "getWindowManager", "()Landroid/view/WindowManager;");
            auto getDefaultDisplay =
                jni->GetMethodID(classWindowManager, "getDefaultDisplay", "()Landroid/view/Display;");

            auto windowManager = jni->CallObjectMethod(m_androidApp->activity->clazz, getWindowManager);
            auto display       = jni->CallObjectMethod(windowManager, getDefaultDisplay);
            auto displayMetricsConstructor = jni->GetMethodID(displayMetricsClass, "<init>", "()V");
            auto displayMetrics            = jni->NewObject(displayMetricsClass, displayMetricsConstructor);
            auto getMetrics = jni->GetMethodID(classDisplay, "getMetrics", "(Landroid/util/DisplayMetrics;)V");
            jni->CallVoidMethod(display, getMetrics, displayMetrics);

            auto logDens_id = jni->GetFieldID(displayMetricsClass, "density", "F");
            density             = jni->GetFloatField(displayMetrics, logDens_id);

            auto xdpi_id = jni->GetFieldID(displayMetricsClass, "xdpi", "F");
            m_xdpi           = jni->GetFloatField(displayMetrics, xdpi_id);

            auto ydpi_id = jni->GetFieldID(displayMetricsClass, "ydpi", "F");
            m_ydpi           = jni->GetFloatField(displayMetrics, ydpi_id);

            auto densDpi_id = jni->GetFieldID(displayMetricsClass, "densityDpi", "I");
            m_densityDpi        = jni->GetIntField(displayMetrics, densDpi_id);

            m_width_meters  = (width_pixels / m_xdpi) * kMetersPerInch;
            m_height_meters = (height_pixels / m_ydpi) * kMetersPerInch;

            jni->DeleteLocalRef(displayMetrics);
        }
    }

    m_androidApp->activity->vm->DetachCurrentThread();

    return density;
}

std::filesystem::path UIAppAndroidNative::getExternalStorageDirectory() const {
    if (!m_androidApp) {
        return {};
    }

    JNIEnv*               jni = nullptr;
    jthrowable            exception;
    std::filesystem::path ret;

    m_androidApp->activity->vm->AttachCurrentThread(&jni, nullptr);
    if (jni) {
        // Get File object for the external storage directory.
        auto classEnvironment = jni->FindClass("android/os/Environment");
        if (!classEnvironment) {
            return ret;
        }

        auto methodIDgetExternalStorageDirectory =
            jni->GetStaticMethodID(classEnvironment, "getExternalStorageDirectory",
                                   "()Ljava/io/File;");  // public static File
                                                         // getExternalStorageDirectory ()
        if (!methodIDgetExternalStorageDirectory) {
            return ret;
        }

        auto objectFile = jni->CallStaticObjectMethod(classEnvironment, methodIDgetExternalStorageDirectory);
        if (!objectFile) {
            return ret;
        }

        exception = jni->ExceptionOccurred();
        if (exception) {
            jni->ExceptionDescribe();
            jni->ExceptionClear();
        }

        // Call method on File object to retrieve String object.
        auto classFile               = jni->GetObjectClass(objectFile);
        auto methodIDgetAbsolutePath = jni->GetMethodID(classFile, "getAbsolutePath", "()Ljava/lang/String;");

        auto stringPath = (jstring)jni->CallObjectMethod(objectFile, methodIDgetAbsolutePath);
        exception       = jni->ExceptionOccurred();
        if (exception) {
            jni->ExceptionDescribe();
            jni->ExceptionClear();
        }

        // Extract a string from the String object
        ret = std::filesystem::path(std::string(jni->GetStringUTFChars(stringPath, nullptr)));

        // jni->ReleaseStringUTFChars(stringPath, wpath3.c_str());
        m_androidApp->activity->vm->DetachCurrentThread();
    }

    return ret;
}

bool UIAppAndroidNative::AssetReadFile(std::string& assetName, std::vector<uint8_t>& buf) {
    if (assetName.empty()) {
        return false;
    }

    auto assetDescriptor =  AAssetManager_open(m_androidApp->activity->assetManager, assetName.c_str(), AASSET_MODE_BUFFER);
    auto fileLength = AAsset_getLength(assetDescriptor);

    buf.resize(fileLength);
    auto readSize = AAsset_read(assetDescriptor, buf.data(), buf.size());

    AAsset_close(assetDescriptor);

    return (readSize == buf.size());
}

jstring UIAppAndroidNative::permission_name(JNIEnv* lJNIEnv, const char* perm_name) {
    // nested class permission in class android.Manifest, hence android 'slash' Manifest 'dollar' permission
    auto ClassManifestpermission = lJNIEnv->FindClass("android/Manifest$permission");
    auto lid_PERM = lJNIEnv->GetStaticFieldID(ClassManifestpermission, perm_name, "Ljava/lang/String;");
    return static_cast<jstring>(lJNIEnv->GetStaticObjectField(ClassManifestpermission, lid_PERM));
}

bool UIAppAndroidNative::has_permission(const char* perm_name) {
    if (!m_androidApp) {
        return false;
    }

    auto    lJavaVM         = m_androidApp->activity->vm;
    JNIEnv* lJNIEnv         = nullptr;
    bool    lThreadAttached = false;

    // Get JNIEnv from lJavaVM using GetEnv to test whether
    // thread is attached or not to the VM. If not, attach it
    // (and note that it will need to be detached at the end
    //  of the function).
    switch (lJavaVM->GetEnv((void**)&lJNIEnv, JNI_VERSION_1_6)) {
        case JNI_OK: break;
        case JNI_EDETACHED: {
            jint lResult = lJavaVM->AttachCurrentThread(&lJNIEnv, nullptr);
            if (lResult == JNI_ERR) {
                throw std::runtime_error("Could not attach current thread");
            }
            lThreadAttached = true;
        } break;
        case JNI_EVERSION: throw std::runtime_error("Invalid java version");
    }

    bool result = false;

    auto ls_PERM = permission_name(lJNIEnv, perm_name);

    auto ClassPackageManager    = lJNIEnv->FindClass("android/content/pm/PackageManager");
    auto lid_PERMISSION_GRANTED = lJNIEnv->GetStaticFieldID(ClassPackageManager, "PERMISSION_GRANTED", "I");
    auto PERMISSION_GRANTED     = lJNIEnv->GetStaticIntField(ClassPackageManager, lid_PERMISSION_GRANTED);

    auto activity     = m_androidApp->activity->clazz;
    auto ClassContext = lJNIEnv->FindClass("android/content/Context");
    auto MethodcheckSelfPermission = lJNIEnv->GetMethodID(ClassContext, "checkSelfPermission", "(Ljava/lang/String;)I");
    auto int_result = lJNIEnv->CallIntMethod(activity, MethodcheckSelfPermission, ls_PERM);
    result          = (int_result == PERMISSION_GRANTED);

    if (lThreadAttached) {
        lJavaVM->DetachCurrentThread();
    }

    return result;
}

void UIAppAndroidNative::request_permissions(std::vector<std::string> perms) {
    if (!m_androidApp) {
        return;
    }

    auto    lJavaVM         = m_androidApp->activity->vm;
    JNIEnv* lJNIEnv         = nullptr;
    bool    lThreadAttached = false;

    // Get JNIEnv from lJavaVM using GetEnv to test whether
    // thread is attached or not to the VM. If not, attach it
    // (and note that it will need to be detached at the end
    //  of the function).
    switch (lJavaVM->GetEnv((void**)&lJNIEnv, JNI_VERSION_1_6)) {
        case JNI_OK: break;
        case JNI_EDETACHED: {
            jint lResult = lJavaVM->AttachCurrentThread(&lJNIEnv, nullptr);
            if (lResult == JNI_ERR) {
                throw std::runtime_error("Could not attach current thread");
            }
            lThreadAttached = true;
        } break;
        case JNI_EVERSION: throw std::runtime_error("Invalid java version");
    }

    auto perm_array =
        lJNIEnv->NewObjectArray(perms.size(), lJNIEnv->FindClass("java/lang/String"), lJNIEnv->NewStringUTF(""));

    for (int i = 0; i < perms.size(); i++) {
        lJNIEnv->SetObjectArrayElement(perm_array, i, permission_name(lJNIEnv, perms[i].c_str()));
    }

    auto activity      = m_androidApp->activity->clazz;
    auto ClassActivity = lJNIEnv->FindClass("android/app/Activity");
    auto MethodrequestPermissions = lJNIEnv->GetMethodID(ClassActivity, "requestPermissions", "([Ljava/lang/String;I)V");

    // Last arg (0) is just for the callback (that I do not use)
    lJNIEnv->CallVoidMethod(activity, MethodrequestPermissions, perm_array, 0);

    if (lThreadAttached) {
        lJavaVM->DetachCurrentThread();
    }
}

void UIAppAndroidNative::check_permission(const std::string& perm) {
    if (!has_permission(perm.c_str())) {
        request_permissions({perm});
    }
}

void UIAppAndroidNative::set_requested_screen_orientation(int an_orientation) {
    JNIEnv* jni;
    m_androidApp->activity->vm->AttachCurrentThread(&jni, nullptr);
    auto clazz    = jni->GetObjectClass(m_androidApp->activity->clazz);
    auto methodID = jni->GetMethodID(clazz, "setRequestedOrientation", "(I)V");
    jni->CallVoidMethod(m_androidApp->activity->clazz, methodID, an_orientation);
    m_androidApp->activity->vm->DetachCurrentThread();
}

int32_t UIAppAndroidNative::getDensityDpi() {
    auto config = AConfiguration_new();
    AConfiguration_fromAssetManager(config, m_androidApp->activity->assetManager);
    int32_t density = AConfiguration_getDensity(config);
    AConfiguration_delete(config);
    return density;
}

}  // namespace ara

#endif