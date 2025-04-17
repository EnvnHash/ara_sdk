macro(create_camera_permission_helper)
    set(cam_perm_help)

    # package
    list(APPEND cam_perm_help "package eu.zeitkunst.${PACKAGE_NAME}\;\n\n")

    # imports
    list(APPEND cam_perm_help "import android.Manifest\;
import android.app.Activity\;
import android.content.Intent\;
import android.content.pm.PackageManager\;
import android.net.Uri\;
import android.provider.Settings\;
import androidx.core.app.ActivityCompat\;
import androidx.core.content.ContextCompat\;\n\n")

    # class b
    list(APPEND cam_perm_help "/** Helper to ask camera permission. */
public class CameraPermissionHelper {
  private static final String CAMERA_PERMISSION = Manifest.permission.CAMERA\;
  private static final int CAMERA_PERMISSION_CODE = 0\;

  /** Check to see we have the necessary permissions for this app. */
  public static boolean hasCameraPermission(Activity activity) {
    return ContextCompat.checkSelfPermission(activity, CAMERA_PERMISSION) == PackageManager.PERMISSION_GRANTED\;
  }

  /** Check to see we have the necessary permissions for this app, and ask for them if we don't. */
  public static void requestCameraPermission(Activity activity) {
    ActivityCompat.requestPermissions(
        activity, new String[] {CAMERA_PERMISSION}, CAMERA_PERMISSION_CODE)\;
  }

  /** Check to see if we need to show the rationale for this permission. */
  public static boolean shouldShowRequestPermissionRationale(Activity activity) {
    return ActivityCompat.shouldShowRequestPermissionRationale(activity, CAMERA_PERMISSION)\;
  }

  /** Launch Application Setting to grant permission. */
  public static void launchPermissionSettings(Activity activity) {
    Intent intent = new Intent()\;
    intent.setAction(Settings.ACTION_APPLICATION_DETAILS_SETTINGS)\;
    intent.setData(Uri.fromParts(\"package\", activity.getPackageName(), null))\;
    activity.startActivity(intent)\;
  }
}

\n")

    FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/java/eu/zeitkunst/${PROJECT_NAME}/CameraPermissionHelper.java ${cam_perm_help}) # write it

endmacro()