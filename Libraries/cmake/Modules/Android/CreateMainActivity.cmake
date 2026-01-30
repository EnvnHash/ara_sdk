macro (create_main_activity app_type ADMOB_UNIT_ID ADMOB_AD_TYPE USE_ANDROID_BILLING)
    set(main_activity)
    replace_dot_with_char(${PACKAGE_URL} "/" package_url_slashes)
    replace_dot_with_char(${PACKAGE_NAME} "/" package_name_slashes)

    if (${app_type} EQUAL 0)
        list(APPEND main_activity "package ${PACKAGE_URL}.${PACKAGE_NAME}\;\n
import android.app.NativeActivity\;

public class MainActivity extends NativeActivity {
}")
        FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/java/${package_url_slashes}/${package_name_slashes}/MainActivity.java ${main_activity}) # write it
    else ()
        # package
        list(APPEND main_activity "package ${PACKAGE_URL}.${PACKAGE_NAME}\;\n\n")

        # imports
        if (ARA_USE_NDI)
            list(APPEND main_activity "import android.net.nsd.NsdManager\;
import android.content.Context\;
    ")
        endif()

        list(APPEND main_activity "import android.content.pm.ActivityInfo\;
import android.hardware.display.DisplayManager\;
import android.os.Bundle\;
import android.util.DisplayMetrics\;
import android.util.Log\;
import android.view.GestureDetector\;
import android.view.ScaleGestureDetector\;
import android.view.MotionEvent\;
import android.view.View\;
import android.view.ViewGroup\;
import android.view.WindowManager\;
import android.widget.RelativeLayout\;
import androidx.annotation.NonNull\;
import androidx.appcompat.app.AppCompatActivity\;
    
")

        if (NOT "${ADMOB_UNIT_ID}" STREQUAL "")
            list(APPEND main_activity "import ${PACKAGE_URL}.${PACKAGE_NAME}.AdHelper\;

")
        endif ()

        # class begin
        list(APPEND main_activity "public class ${PROJECT_NAME}Activity extends AppCompatActivity implements DisplayManager.DisplayListener {
      private static final String TAG = ${PROJECT_NAME}Activity.class.getSimpleName()\;

      private CustomGLSurfaceView surfaceView\;

      // Opaque native pointer to the native application instance.
      private GestureDetector gestureDetector\;
      private ScaleGestureDetector scaleGestureDetector\;
      private static AppCompatActivity m_activity\;
      private boolean isScaling = false\;
")

        if (NOT "${ADMOB_UNIT_ID}" STREQUAL "")
            list(APPEND main_activity "      private static AdHelper m_adHelper\;
")
        endif ()

        if (${USE_ANDROID_BILLING})
            list(APPEND main_activity "      private static BillingManager m_billingMan;\;
")
        endif ()

        if (ARA_USE_NDI)
            list(APPEND main_activity "  private NsdManager m_nsdManager\;
")
        endif()
        
    # class OnCreate
        list(APPEND main_activity "
      @Override
      protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState)\;
        setTheme(R.style.Theme_Main)\;
        setContentView(R.layout.activity_main)\;

        RelativeLayout relativeLayout = findViewById(R.id.relativeLayout)\;
        surfaceView = new CustomGLSurfaceView(this)\;
        RelativeLayout.LayoutParams glSurfaceViewLayoutParams =
                  new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)\;
        glSurfaceViewLayoutParams.addRule(RelativeLayout.ALIGN_PARENT_TOP)\;
        relativeLayout.addView(surfaceView, glSurfaceViewLayoutParams)\;
        m_activity = this\;
")

        if (ARA_USE_NDI)
            list(APPEND main_activity "    m_nsdManager = (NsdManager)getSystemService(Context.NSD_SERVICE)\;
")
        endif()

        list(APPEND main_activity "
        // Set up touch listener.
        gestureDetector =
            new GestureDetector(
                this,
                new GestureDetector.SimpleOnGestureListener() {
                    @Override
                    public boolean onSingleTapUp(@NonNull final MotionEvent e) {
                        surfaceView.queueEvent(() -> JniInterface.onTouched(e.getX(), e.getY()))\;
                        return true\;
                    }

                    @Override
                    public boolean onDown(@NonNull MotionEvent e) {
                        surfaceView.queueEvent(() -> JniInterface.onTouchDown(e.getX(), e.getY()))\;
                        return true\;
                    }

                    @Override
                    public boolean onScroll(MotionEvent e1, @NonNull MotionEvent e2, float distanceX, float distanceY) {
                        if (!isScaling) {
                            surfaceView.queueEvent(() -> JniInterface.onScroll(e2.getX(), e2.getY()))\;
                        }
                        return true\;
                    }
                })\;

        // Set up scale gesture listener
        scaleGestureDetector = new ScaleGestureDetector(this, new ScaleGestureDetector.SimpleOnScaleGestureListener() {
            @Override
            public boolean onScaleBegin(@NonNull ScaleGestureDetector detector) {
                isScaling = true\;
                surfaceView.queueEvent(JniInterface::onScaleBegin)\;
                return true\; // Return true to indicate that this listener accepts the event.
            }

            @Override
            public boolean onScale(@NonNull ScaleGestureDetector detector) {
                float val = detector.getScaleFactor()\;
                float fX = detector.getFocusX()\;
                float fY = detector.getFocusY()\;
                surfaceView.queueEvent(() -> JniInterface.onScale(val, fX, fY))\;
                return true\;
            }
        })\;

        View.OnTouchListener touchListener = new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                gestureDetector.onTouchEvent(event)\;
                scaleGestureDetector.onTouchEvent(event)\;
                boolean detectedUp = event.getAction() == MotionEvent.ACTION_UP\;

                if ((detectedUp || event.getAction() == MotionEvent.ACTION_CANCEL)
                    && event.getPointerCount() <= 1 && isScaling) {
                    isScaling = false\;
                    surfaceView.queueEvent(JniInterface::onScaleEnd)\;
                } else if(detectedUp) {
                    surfaceView.queueEvent(JniInterface::onTouchUp)\;
                }

                return true\;
            }
        }\;

        surfaceView.setOnTouchListener(touchListener)\;

        JniInterface.assetManager = getAssets()\;

        surfaceView.setNativeApp(JniInterface.createNativeApplication(getAssets(), getFilesDir().getAbsolutePath()))\;
        surfaceView.setWindowManager(getWindowManager())\;

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
")

      if (NOT "${ADMOB_UNIT_ID}" STREQUAL "")
            list (APPEND main_activity "
        m_adHelper = new AdHelper(this)\;")
      endif ()
        
      if ("${ADMOB_AD_TYPE}" STREQUAL "")
          list (APPEND main_activity "
        m_adHelper.loadStandardAd(\"${ADMOB_UNIT_ID}\", relativeLayout)\;
")
      elseif ("${ADMOB_AD_TYPE}" STREQUAL "interstitial")
          list (APPEND main_activity "
        m_adHelper.loadInterstitialAd(\"${ADMOB_UNIT_ID}\")\;
")
      endif ()

      if (${USE_ANDROID_BILLING})
           list (APPEND main_activity "
        m_billingMan = new BillingManager(this)\;
")
      endif ()

      list (APPEND main_activity "
      }\n\n")

        # planeStatusCheckingHandler = new Handler()\;

        # class OnResume
        list(APPEND main_activity "      @Override
      protected void onResume() {
        super.onResume()\;
        ")

        if (ARA_USE_ARCORE)
            list(APPEND main_activity " // ARCore requires camera permissions to operate. If we did not yet obtain runtime
        // permission on Android M and above, now is a good time to ask the user for it.
        if (!CameraPermissionHelper.hasCameraPermission(this)) {
          CameraPermissionHelper.requestCameraPermission(this)\;
          return\;
        }
        ")
        endif ()

        list(APPEND main_activity "try {
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
        list(APPEND main_activity "      @Override
      public void onStart() {
        super.onStart()\;
        JniInterface.onStart()\;
      }\n\n")

        # class OnPause
        list(APPEND main_activity "      @Override
      public void onPause() {
        super.onPause()\;
        surfaceView.onPause()\;
        JniInterface.onPause()\;

        getSystemService(DisplayManager.class).unregisterDisplayListener(this)\;
      }\n\n")

        # class OnDestroy
        list(APPEND main_activity "      @Override
      public void onDestroy() {
        super.onDestroy()\;

        // Synchronized to avoid racing onDrawFrame.
        synchronized (this) {
          JniInterface.destroyNativeApplication()\;
          surfaceView.setNativeApp(0)\;
        }
      }\n\n")

        # class OnFocusChanged
        list(APPEND main_activity "      @Override
      public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus)\;
        if (hasFocus) {
          // Standard Android full-screen functionality.
          setImmersiveSticky()\;
          getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)\;
        }
      }\n\n")

        # class onRequestPermissionsResult
        if (ARA_USE_ARCORE)
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
        endif()

        # class loadAd
        if (NOT "${ADMOB_UNIT_ID}" STREQUAL "")
            list(APPEND main_activity "      public static void loadAd() {
        m_activity.runOnUiThread(() -> {")

            if ("${ADMOB_AD_TYPE}" STREQUAL "")
                list(APPEND main_activity "
            m_adHelper.showStandardAd(true)\;")
            elseif ("${ADMOB_AD_TYPE}" STREQUAL "interstitial")
                list(APPEND main_activity "
            m_adHelper.showInterstitialAd()\;")
            endif ()

            list(APPEND main_activity "
        })\;
      }

")
        endif ()

        if (${USE_ANDROID_BILLING})
            list(APPEND main_activity "      public static void startPayFlow() {
        m_activity.runOnUiThread(() -> {
            m_billingMan.purchaseProduct(\"1\")\;
        })\;
      }

")
        endif()

        # class onRequestPermissionsResult
        list(APPEND main_activity "      private void setImmersiveSticky() {
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
        surfaceView.setViewPortChanges(true)\;
      }\n\n")

        # class end
        list(APPEND main_activity "}\n\n")
        FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/java/${package_url_slashes}/${package_name_slashes}/${PROJECT_NAME}Activity.java ${main_activity}) # write it
    endif ()
endmacro()
