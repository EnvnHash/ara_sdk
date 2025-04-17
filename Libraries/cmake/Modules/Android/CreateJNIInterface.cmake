macro(create_jni_interface)

    set(jni_interface)

    # package
    list(APPEND jni_interface "package eu.zeitkunst.${PACKAGE_NAME}\;\n\n")

    # imports
    list(APPEND jni_interface "import android.app.Activity\;
import android.content.Context\;
import android.content.res.AssetManager\;
import android.graphics.Bitmap\;
import android.graphics.BitmapFactory\;
import android.opengl.GLUtils\;
import android.util.Log\;
import java.io.IOException\;\n\n")


    # class begin
    list(APPEND jni_interface "/** JNI interface to native layer. */
public class JniInterface {
  static {
    System.loadLibrary(\"${PROJECT_NAME}\")\;
  }

  private static final String TAG = \"JniInterface\"\;
  static AssetManager assetManager\;

  public static native long createNativeApplication(AssetManager assetManager, String internalDataPath)\;

  public static native void setInternalDataPath(String path)\;

  public static native void setDisplayDensity(float density, float w, float h, float xdpi, float ydpi)\;

  public static native void destroyNativeApplication()\;

  public static native void onStart()\;

  public static native void onPause()\;

  public static native void onResume(Context context, Activity activity)\;

  /** Allocate OpenGL resources for rendering. */
  public static native void onGlSurfaceCreated()\;

  /**
   * Called on the OpenGL thread before onGlSurfaceDrawFrame when the view port width, height, or
   * display rotation may have changed.
   */
  public static native void onDisplayGeometryChanged(int displayRotation, int width, int height)\;

  /** Main render loop, called on the OpenGL thread. */
  public static native void onGlSurfaceDrawFrame()\;

  /** OnTouch event, called on the OpenGL thread. */
  public static native void onTouched(float x, float y)\;

  /** OnTouchDown event, called on the OpenGL thread. */
  public static native void onTouchDown(float x, float y)\;

  /** OnScroll event, called on the OpenGL thread. */
  public static native void onScroll(float x, float y)\;

  /** Get plane count in current session. Used to disable the \"searching for surfaces\" snackbar. */
  public static native boolean hasDetectedPlanes(long nativeApplication)\;

  public static Bitmap loadImage(String imageName) {

    try {
      return BitmapFactory.decodeStream(assetManager.open(imageName))\;
    } catch (IOException e) {
      Log.e(TAG, \"Cannot open image \" + imageName)\;
      return null\;
    }
  }

  public static void loadTexture(int target, Bitmap bitmap) {
    GLUtils.texImage2D(target, 0, bitmap, 0)\;
  }
}\n")

    FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/java/eu/zeitkunst/${PROJECT_NAME}/JniInterface.java ${jni_interface}) # write it

endmacro()