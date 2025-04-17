macro (create_splashscreen_activity)
    set(splashscreen_activity)

    # package
    list(APPEND splashscreen_activity "package eu.zeitkunst.${PACKAGE_NAME}\;\n
import android.app.Activity\;
import android.content.Intent\;
import android.os.Bundle\;

public class SplashActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState)\;
        startActivity(new Intent(this, MainActivity.class))\;
        finish()\;
    }
}")

    FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/java/eu/zeitkunst/${PROJECT_NAME}/SplashActivity.java ${splashscreen_activity}) # write it

endmacro()
