macro(create_main_activity_layout)

    get_property(PACKAGE_URL TARGET ${PROJECT_NAME} PROPERTY ARASDK_PACKAGE_URL)
    get_property(PACKAGE_NAME TARGET ${PROJECT_NAME} PROPERTY ARASDK_PACKAGE_NAME)

    file(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/res/layout/activity_main.xml  "<RelativeLayout xmlns:android=\"http://schemas.android.com/apk/res/android\"
    xmlns:tools=\"http://schemas.android.com/tools\"
    android:layout_width=\"match_parent\"
    android:layout_height=\"match_parent\"
    android:id=\"@+id/relativeLayout\"
    tools:context=\"${PACKAGE_URL}.${PACKAGE_NAME}.${PROJECT_NAME}Activity\">

</RelativeLayout>
")

endmacro()