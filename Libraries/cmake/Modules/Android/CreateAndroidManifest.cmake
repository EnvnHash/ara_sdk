macro (create_android_manifest APP_TYPE APP_ICON_NAME APP_ORIENTATION)
    set(manifest)
    list(APPEND manifest "<?xml version=\"1.0\" encoding=\"utf-8\"?>
<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"
    android:versionCode=\"1\"
    android:versionName=\"1.0\">\n\n")

    list(APPEND manifest "\t<uses-permission android:name=\"android.permission.INTERNET\"/>
    <uses-permission android:name=\"android.permission.WAKE_LOCK\" />
    <uses-permission android:name=\"android.permission.ACCESS_WIFI_STATE\" />
    <uses-permission android:name=\"android.permission.ACCESS_NETWORK_STATE\" />
    <uses-permission android:name=\"android.permission.READ_EXTERNAL_STORAGE\" />\n\n")

    if (ARA_USE_ARCORE)
        list(APPEND manifest "\t<uses-feature android:name=\"android.hardware.camera.ar\" android:required=\"true\"/>\n")
        list(APPEND manifest "\t<uses-permission android:name=\"android.permission.CAMERA\" />\n")
        list(APPEND manifest "\t<uses-feature android:glEsVersion=\"0x00030000\" android:required=\"true\"/>\n\n")
    endif()

    list(APPEND manifest "\t<application
        android:allowBackup=\"false\"
        android:fullBackupContent=\"false\"
        android:label=\"@string/app_name\">\n\n")

#    android:icon=\"@mipmap/${APP_ICON_NAME}\"

    if (ARA_USE_ARCORE)
        list(APPEND manifest "\t\t<meta-data android:name=\"com.google.ar.core\" android:value=\"required\" />\n\n")
    endif()

    if (${APP_TYPE} EQUAL 0)
        list(APPEND manifest "\t\t<activity
            android:name=\"android.app.NativeActivity\"
            android:theme=\"@style/Theme.Splash\"")
    elseif(${APP_TYPE} EQUAL 1)
        list(APPEND manifest "\t\t<activity android:name=\".${PROJECT_NAME}Activity\"
            android:theme=\"@style/Theme.AppCompat.NoActionBar\"")
    endif()

    if (NOT ${APP_ORIENTATION} STREQUAL "")
        message(STATUS "got APP_ORIENTATION")
        list(APPEND manifest "\n\t\t\tandroid:screenOrientation=\"${APP_ORIENTATION}\"")
    endif ()

    list(APPEND manifest "
            android:label=\"${PROJECT_NAME}\"
            android:exported=\"true\"
            android:configChanges=\"screenSize\">\n\n")

    if (${APP_TYPE} EQUAL 0)
        list(APPEND manifest "\t\t\t<meta-data android:name=\"android.app.lib_name\" android:value=\"${PACKAGE_NAME}\" />
            ")
    endif()

    list(APPEND manifest "
            <intent-filter>
                <action android:name=\"android.intent.action.MAIN\" />
                <category android:name=\"android.intent.category.LAUNCHER\" />
            </intent-filter>
        </activity>
    </application>

</manifest>")

    FILE(WRITE  ${ANDROID_STUDIO_PROJ}/app/src/main/AndroidManifest.xml ${manifest}) # write it
endmacro()