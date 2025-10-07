macro (create_splashscreen_activity APP_TYPE)
    set(splashscreen_activity)

    # package
    list(APPEND splashscreen_activity "package ${PACKAGE_URL}.${PACKAGE_NAME}\;\n
import android.app.Activity\;
import android.content.Intent\;
import android.os.Bundle\;

public class SplashActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState)\;
        startActivity(new Intent(this, ")

    if (${APP_TYPE} EQUAL 0)
        list(APPEND splashscreen_activity "MainActivity")
    else ()
        list(APPEND splashscreen_activity "${PROJECT_NAME}Activity")
    endif ()

    list(APPEND splashscreen_activity ".class))\;
        finish()\;
    }
}")

    replace_dot_with_char(${PACKAGE_URL} "/" package_url_slashes)
    replace_dot_with_char(${PACKAGE_NAME} "/" package_name_slashes)
    FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/java/${package_url_slashes}/${package_name_slashes}/SplashActivity.java ${splashscreen_activity}) # write it

endmacro()
