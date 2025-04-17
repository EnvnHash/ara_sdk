macro(create_main_activity_layout)

    file(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/res/layout/activity_main.xml  "<RelativeLayout xmlns:android=\"http://schemas.android.com/apk/res/android\"
    xmlns:tools=\"http://schemas.android.com/tools\"
    android:layout_width=\"match_parent\"
    android:layout_height=\"match_parent\"
    tools:context=\"eu.zeitkunst.${PROJECT_NAME}Activity\">

  <android.opengl.GLSurfaceView
      android:id=\"@+id/surfaceview\"
      android:layout_width=\"match_parent\"
      android:layout_height=\"match_parent\"
      android:layout_gravity=\"top\"/>

</RelativeLayout>
")

endmacro()