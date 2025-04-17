macro (create_main_activity app_type)
    set(main_activity)

    if (${app_type} EQUAL 0)
        list(APPEND main_activity "package eu.zeitkunst.${PACKAGE_NAME}\;\n
import android.app.NativeActivity\;

public class MainActivity extends NativeActivity {
}")

        FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/java/eu/zeitkunst/${PROJECT_NAME}/MainActivity.java ${main_activity}) # write it
    else ()
        # package
        list(APPEND main_activity "package eu.zeitkunst.${PACKAGE_NAME}\;\n\n")

        # imports
        if (ARA_USE_NDI)
            list(APPEND main_activity "import android.net.nsd.NsdManager\;
    import android.content.Context\;
    ")
        endif()

    list(APPEND main_activity "import android.content.res.Resources\;
    import android.content.pm.ActivityInfo\;
    import android.hardware.display.DisplayManager\;
    import android.opengl.GLES20\;
    import android.opengl.GLES30\;
    import android.opengl.GLSurfaceView\;
    import android.os.Bundle\;
    import android.util.DisplayMetrics\;
    import android.util.Log\;
    import android.view.GestureDetector\;
    import android.view.MotionEvent\;
    import android.view.View\;
    import android.view.WindowManager\;
    import android.widget.Toast\;
    import androidx.appcompat.app.AppCompatActivity\;
    import javax.microedition.khronos.egl.EGLConfig\;
    import javax.microedition.khronos.opengles.GL10\;\n\n")

        # class begin
        list(APPEND main_activity "public class ${PROJECT_NAME}Activity extends AppCompatActivity implements GLSurfaceView.Renderer, DisplayManager.DisplayListener {
      private static final String TAG = ${PROJECT_NAME}Activity.class.getSimpleName()\;

      private GLSurfaceView surfaceView\;

      private boolean viewportChanged = false\;
      private int viewportWidth\;
      private int viewportHeight\;

      // Opaque native pointer to the native application instance.
      private long nativeApplication\;
      private GestureDetector gestureDetector\;
      private static AppCompatActivity m_activity\;

    ")

        if (ARA_USE_NDI)
            list(APPEND main_activity "  private NsdManager m_nsdManager\;
        ")
        endif()


    # class OnCreate
        list(APPEND main_activity "
      @Override
      protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState)\;
        setContentView(R.layout.activity_main)\;

        m_activity = this\;
        surfaceView = (GLSurfaceView) findViewById(R.id.surfaceview)\;
    ")
        if (ARA_USE_NDI)
            list(APPEND main_activity "    m_nsdManager = (NsdManager)getSystemService(Context.NSD_SERVICE)\;
    ")
        endif()

        list(APPEND main_activity "   // Set up touch listener.
        gestureDetector =
            new GestureDetector(
                this,
                new GestureDetector.SimpleOnGestureListener() {
                    @Override
                    public boolean onSingleTapUp(final MotionEvent e) {
                        surfaceView.queueEvent(() -> JniInterface.onTouched(e.getX(), e.getY()))\;
                        return true\;
                    }

                    @Override
                    public boolean onDown(MotionEvent e) {
                        surfaceView.queueEvent(() -> JniInterface.onTouchDown(e.getX(), e.getY()))\;
                        return true\;
                    }

                    @Override
                    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
                        surfaceView.queueEvent(() -> JniInterface.onScroll(e2.getX(), e2.getY()))\;
                        return true\;
                    }
                })\;

        surfaceView.setOnTouchListener((View v, MotionEvent event) -> gestureDetector.onTouchEvent(event))\;

        // Set up renderer.
        surfaceView.setPreserveEGLContextOnPause(true)\;
        surfaceView.setEGLContextClientVersion(3)\;
        surfaceView.setEGLConfigChooser(8, 8, 8, 8, 16, 0)\; // Alpha used for plane blending.
        surfaceView.setRenderer(this)\;
        surfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY)\;
        surfaceView.setWillNotDraw(false)\;

        JniInterface.assetManager = getAssets()\;
        nativeApplication = JniInterface.createNativeApplication(getAssets(), getFilesDir().getAbsolutePath())\;
    //    JniInterface.setExternalDataPath(Environment.getExternalStorageDirectory().getAbsolutePath() )\;
        DisplayMetrics m = new DisplayMetrics()\;
        getWindowManager().getDefaultDisplay().getRealMetrics(m)\;
        JniInterface.setDisplayDensity(m.density, m.widthPixels, m.heightPixels, m.xdpi, m.ydpi)\;

        setImmersiveSticky()\;
        View decorView = getWindow().getDecorView()\;
        decorView.setOnSystemUiVisibilityChangeListener((visibility) -> {
          if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) {
            setImmersiveSticky()\;
          }
        })\;

        // Forces screen to max brightness.
        WindowManager.LayoutParams layout = getWindow().getAttributes()\;
        layout.screenBrightness = 1.f\;
        getWindow().setAttributes(layout)\;

        // Prevents screen from dimming/locking.
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)\;
      }\n\n")

        # planeStatusCheckingHandler = new Handler()\;

        # class OnResume
        list(APPEND main_activity "  @Override
      protected void onResume() {
        super.onResume()\;
        // ARCore requires camera permissions to operate. If we did not yet obtain runtime
        // permission on Android M and above, now is a good time to ask the user for it.
        if (!CameraPermissionHelper.hasCameraPermission(this)) {
          CameraPermissionHelper.requestCameraPermission(this)\;
          return\;
        }

        try {
          JniInterface.onResume(getApplicationContext(), this)\;
          surfaceView.onResume()\;
        } catch (Exception e) {
          Log.e(TAG, \"Exception creating session\", e)\;
          return\;
        }

        // Listen to display changed events to detect 180° rotation, which does not cause a config
        // change or view resize
        getSystemService(DisplayManager.class).registerDisplayListener(this, null)\;
      }\n\n")

        # class OnStart
        list(APPEND main_activity "  @Override
      public void onStart() {
        super.onStart()\;
        JniInterface.onStart()\;
      }\n\n")

        # class OnPause
        list(APPEND main_activity "  @Override
      public void onPause() {
        super.onPause()\;
        surfaceView.onPause()\;
        JniInterface.onPause()\;

        getSystemService(DisplayManager.class).unregisterDisplayListener(this)\;
      }\n\n")

        # class OnDestroy
        list(APPEND main_activity "  @Override
      public void onDestroy() {
        super.onDestroy()\;

        // Synchronized to avoid racing onDrawFrame.
        synchronized (this) {
          JniInterface.destroyNativeApplication()\;
          nativeApplication = 0\;
        }
      }\n\n")

        # class OnFocusChanged
        list(APPEND main_activity "  @Override
      public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus)\;
        if (hasFocus) {
          // Standard Android full-screen functionality.
          setImmersiveSticky()\;
          getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)\;
        }
      }\n\n")

        # class OnFocusCreated
        list(APPEND main_activity "  @Override
      public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        GLES20.glClearColor(0.1f, 0.1f, 0.1f, 1.0f)\;
        JniInterface.onGlSurfaceCreated()\;
      }\n\n")

        # class onSurfaceChanged
        list(APPEND main_activity "  @Override
      public void onSurfaceChanged(GL10 gl, int width, int height) {
        viewportWidth = width\;
        viewportHeight = height\;
        int displayRotation = getWindowManager().getDefaultDisplay().getRotation()\;
        JniInterface.onDisplayGeometryChanged(displayRotation, viewportWidth, viewportHeight)\;
      }\n\n")

        # class onSurfaceChanged
        list(APPEND main_activity "  @Override
      public void onDrawFrame(GL10 gl) {
        // Synchronized to avoid racing onDestroy.
        synchronized (this) {
          if (nativeApplication == 0) {
            return\;
          }
          if (viewportChanged) {
            int displayRotation = getWindowManager().getDefaultDisplay().getRotation()\;
            JniInterface.onDisplayGeometryChanged(displayRotation, viewportWidth, viewportHeight)\;
            viewportChanged = false\;
          }
          JniInterface.onGlSurfaceDrawFrame()\;
        }
      }\n\n")

        # class onRequestPermissionsResult
        list(APPEND main_activity "   @Override
      public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
        super.onRequestPermissionsResult(requestCode, permissions, results)\;
        if (!CameraPermissionHelper.hasCameraPermission(this)) {
          Toast.makeText(this, \"Camera permission is needed to run this application\", Toast.LENGTH_LONG).show()\;
          if (!CameraPermissionHelper.shouldShowRequestPermissionRationale(this)) {
            // Permission denied with checking \"Do not ask again\".
            CameraPermissionHelper.launchPermissionSettings(this)\;
          }
          finish()\;
        }
      }\n\n")

        # class onRequestPermissionsResult
        list(APPEND main_activity "  private void setImmersiveSticky() {
        getWindow()
        .getDecorView()
        .setSystemUiVisibility(
        View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_FULLSCREEN
            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY)\;
      }

      public static void fixOrientation(int ori) {
        m_activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE)\;
      }

      public static void resetOrientation() {
        m_activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED)\;
      }

    ")
        # class DisplayListener methods
        list(APPEND main_activity "  @Override
      public void onDisplayAdded(int displayId) {}

      @Override
      public void onDisplayRemoved(int displayId) {}

      @Override
      public void onDisplayChanged(int displayId) {
        viewportChanged = true\;
      }\n\n")

        # class end
        list(APPEND main_activity "}\n\n")
        FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/java/eu/zeitkunst/${PROJECT_NAME}/${PROJECT_NAME}Activity.java ${main_activity}) # write it
    endif ()
endmacro()
