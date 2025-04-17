//
// Created by sven on 05-07-22.
//

#pragma once

#include "Android/UIAppAndroidCommon.h"
#include "UIApplicationBase.h"

namespace ara {

class UIAppAndroidNative : public UIApplicationBase {
public:
    UIAppAndroidNative();

    static bool IsAnimating();
    void        setAndroidApp(struct android_app* app) {
        if (!app) return;
        m_androidApp         = app;
        m_glbase.android_app = app;
    }
    static void    handle_cmd(struct android_app* app, int32_t cmd);
    static int32_t handle_input(struct android_app* app, AInputEvent* event);

    /// get the display density
    int32_t getDensityDpi();

    /// get the display orientation
    int32_t get_orientation();

    /// get the display density
    float get_density(float width_pixels, float height_pixels);

    /// wrapper for the getExternalStorageDirectory()
    /// https://developer.android.com/reference/android/os/Environment#getExternalStorageDirectory()
    std::filesystem::path getExternalStorageDirectory();

    /**
     * \brief Gets the internal name for an android permission.
     * \param[in] lJNIEnv a pointer to the JNI environment
     * \param[in] perm_name the name of the permission, e.g.,
     *   "READ_EXTERNAL_STORAGE", "WRITE_EXTERNAL_STORAGE".
     * \return a jstring with the internal name of the permission,
     *   to be used with android Java functions
     *   Context.checkSelfPermission() or Activity.requestPermissions()
     */
    jstring permission_name(JNIEnv* lJNIEnv, const char* perm_name);

    /**
     * \brief Tests whether a permission is granted.
     * \param[in] perm_name the name of the permission, e.g.,
     * "READ_EXTERNAL_STORAGE", "WRITE_EXTERNAL_STORAGE".
     * \retval true if the permission is granted.
     * \retval false otherwise.
     * \note Requires Android API level 23 (Marshmallow, May 2015)
     */
    bool has_permission(const char* perm_name);

    /**
     * \brief Query camera permissions.
     * \details This opens the system dialog that lets the user grant (or deny)
     * camea permission.
     * \note Requires Android API level 23 (Marshmallow, May 2015)
     */
    void request_permissions(std::vector<std::string> perms);

    /**
     * \brief Checks if a specific permission is set and otherwise opens a modal
     * window, requesting the user to set it
     * \param[in] perm the name of the permission, e.g.,
     * "READ_EXTERNAL_STORAGE", "WRITE_TERNAL_STORAGE".
     */
    void check_permission(std::string perm);

    /// read files from the android projetcs asset directory (if there is any)
    bool AssetReadFile(std::string& name, std::vector<uint8_t>& buf);

    /// brief Starts the main loop which in this case is identical to the main
    /// ui draw loop
    void startAndroidEventLoop();
    void set_requested_screen_orientation(int an_orientation);

    static inline bool mHasFocus  = false;
    static inline bool mIsVisible = false;
    static inline bool mHasWindow = false;

    android_app_state   m_saved_state;
    struct android_app* m_androidApp          = nullptr;
    ANativeWindow*      m_androidNativeWin    = nullptr;
    ASensorManager*     m_sensorManager       = nullptr;
    const ASensor*      m_accelerometerSensor = nullptr;
    ASensorEventQueue*  m_sensorEventQueue    = nullptr;
    void*               m_app_context         = nullptr;
    android_cmd_data    m_cmd_data;

    float m_xdpi          = 0.f;
    float m_ydpi          = 0.f;
    float m_densityDpi    = 0.f;
    float m_width_meters  = 0.f;
    float m_height_meters = 0.f;

    std::unordered_map<android_app_cmd, std::list<std::function<void(android_cmd_data*)>>> m_appStateCbs;
};

}  // namespace ara