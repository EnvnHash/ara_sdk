macro (create_custom_glsurfaceview
        #app_type
)
    get_property(PACKAGE_URL TARGET ${PROJECT_NAME} PROPERTY ARASDK_PACKAGE_URL)
    get_property(PACKAGE_NAME TARGET ${PROJECT_NAME} PROPERTY ARASDK_PACKAGE_NAME)
    get_property(app_type TARGET ${PROJECT_NAME} PROPERTY ARASDK_APP_TYPE)
    
    if (${app_type} EQUAL 1)
        replace_dot_with_char(${PACKAGE_URL} "/" package_url_slashes)
        replace_dot_with_char(${PACKAGE_NAME} "/" package_name_slashes)

        set(source_code)

        list(APPEND source_code "package ${PACKAGE_URL}.${PACKAGE_NAME}\;\n
import static android.opengl.EGL15.EGL_OPENGL_ES3_BIT\;

import android.content.Context\;
import android.opengl.GLSurfaceView\;
import android.view.WindowManager\;
import javax.microedition.khronos.egl.EGL10\;
import javax.microedition.khronos.egl.EGLContext\;
import javax.microedition.khronos.egl.EGLConfig\;
import javax.microedition.khronos.egl.EGLDisplay\;
import javax.microedition.khronos.egl.EGLSurface\;
import javax.microedition.khronos.opengles.GL10\;
import android.util.Log\;

public class CustomGLSurfaceView extends GLSurfaceView {
    private static final String TAG = CustomGLSurfaceView.class.getSimpleName()\;

    public Renderer mRenderer\;
    public EGLSurface mSurface\;
    public static EGLDisplay mDisplay\;
    public static EGLContext mContext\;

    public CustomGLSurfaceView(Context context) {
        super(context)\;

        setEGLContextFactory(new ContextFactory())\;
        setEGLConfigChooser(new ConfigChooser(8, 8, 8, 8, 16, 0))\;
        mRenderer = new Renderer()\;
        setRenderer(mRenderer)\;
        setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY)\;
        setPreserveEGLContextOnPause(true)\;
        setWillNotDraw(false)\;
    }

    public void setNativeApp(long nativeApp) {
        mRenderer.nativeApplication = nativeApp\;
    }

    public void setWindowManager(WindowManager windowManager) {
        mRenderer.mWindowManager = windowManager\;
    }

    public void setViewPortChanges(Boolean val) {
        mRenderer.mViewportChanged = val\;
    }

    private static class ContextFactory implements GLSurfaceView.EGLContextFactory {
        private static int EGL_CONTEXT_CLIENT_VERSION = 0x3098\;

        public EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig eglConfig) {
            mDisplay = display\;
            Log.w(TAG, \"creating OpenGL ES 3.0 context\")\;
            checkEglError(\"Before eglCreateContext\", egl)\;

            int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL10.EGL_NONE}\;
            mContext = egl.eglCreateContext(display, eglConfig, EGL10.EGL_NO_CONTEXT, attrib_list)\;

            checkEglError(\"After eglCreateContext\", egl)\;
            return mContext\;
        }

        public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context) {
            mDisplay = null\;
            egl.eglDestroyContext(display, context)\;
        }
    }

    private static void checkEglError(String prompt, EGL10 egl) {
        int error\;
        while ((error = egl.eglGetError()) != EGL10.EGL_SUCCESS) {
            Log.e(TAG, String.format(\"%s: EGL error: 0x%x\", prompt, error))\;
        }
    }

    private static class ConfigChooser implements GLSurfaceView.EGLConfigChooser {
        public ConfigChooser(int r, int g, int b, int a, int depth, int stencil) {
            mRedSize = r\;
            mGreenSize = g\;
            mBlueSize = b\;
            mAlphaSize = a\;
            mDepthSize = depth\;
            mStencilSize = stencil\;
        }

        /* This EGL config specification is used to specify 2.0 rendering.
         * We use a minimum size of 8 bits for red/green/blue, but will
         * perform actual matching in chooseConfig() below.
         */
        private static int[] s_configAttribs2 =
            {
                EGL10.EGL_RED_SIZE, 8,
                EGL10.EGL_GREEN_SIZE, 8,
                EGL10.EGL_BLUE_SIZE, 8,
                EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                EGL10.EGL_NONE
            }\;

        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
            // Get the number of minimally matching EGL configurations
            int[] num_config = new int[1]\;
            egl.eglChooseConfig(display, s_configAttribs2, null, 0, num_config)\;

            int numConfigs = num_config[0]\;
            if (numConfigs <= 0) {
                throw new IllegalArgumentException(\"No configs match configSpec\")\;
            }

            // Allocate then read the array of minimally matching EGL configs
            EGLConfig[] configs = new EGLConfig[numConfigs]\;
            egl.eglChooseConfig(display, s_configAttribs2, configs, numConfigs, num_config)\;
            return chooseConfig(egl, display, configs)\; // Now return the best one
        }

        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display, EGLConfig[] configs) {
            for(EGLConfig config : configs) {
                int d = findConfigAttrib(egl, display, config, EGL10.EGL_DEPTH_SIZE, 0)\;
                int s = findConfigAttrib(egl, display, config, EGL10.EGL_STENCIL_SIZE, 0)\;

                // We need at least mDepthSize and mStencilSize bits
                if (d < mDepthSize || s < mStencilSize) {
                    continue\;
                }

                // We want an *exact* match for red/green/blue/alpha
                int r = findConfigAttrib(egl, display, config, EGL10.EGL_RED_SIZE, 0)\;
                int g = findConfigAttrib(egl, display, config, EGL10.EGL_GREEN_SIZE, 0)\;
                int b = findConfigAttrib(egl, display, config, EGL10.EGL_BLUE_SIZE, 0)\;
                int a = findConfigAttrib(egl, display, config, EGL10.EGL_ALPHA_SIZE, 0)\;

                if (r == mRedSize && g == mGreenSize && b == mBlueSize && a == mAlphaSize) {
                    return config\;
                }
            }
            return null\;
        }

        private int findConfigAttrib(EGL10 egl, EGLDisplay display, EGLConfig config, int attribute, int defaultValue) {
            if (egl.eglGetConfigAttrib(display, config, attribute, mValue)) {
                return mValue[0]\;
            }
            return defaultValue\;
        }

        // Subclasses can adjust these values:
        protected int mRedSize\;
        protected int mGreenSize\;
        protected int mBlueSize\;
        protected int mAlphaSize\;
        protected int mDepthSize\;
        protected int mStencilSize\;
        private int[] mValue = new int[1]\;
    }

    private static class Renderer implements GLSurfaceView.Renderer {
        private int viewportWidth\;
        private int viewportHeight\;
        public long nativeApplication\;
        private boolean mViewportChanged = false\;
        protected WindowManager mWindowManager\;

        public void onDrawFrame(GL10 gl) {
            // Synchronized to avoid racing onDestroy.
            synchronized (this) {
                if (nativeApplication == 0) {
                    return\;
                }
                if (mViewportChanged) {
                    int displayRotation = mWindowManager.getDefaultDisplay().getRotation()\;
                    JniInterface.onDisplayGeometryChanged(displayRotation, viewportWidth, viewportHeight)\;
                    mViewportChanged = false\;
                }
                JniInterface.onGlSurfaceDrawFrame()\;
            }
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            viewportWidth = width\;
            viewportHeight = height\;
            int displayRotation = mWindowManager.getDefaultDisplay().getRotation()\;
            JniInterface.onDisplayGeometryChanged(displayRotation, viewportWidth, viewportHeight)\;
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            JniInterface.onGlSurfaceCreated()\;
        }
    }
}
    ")
        FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/java/${package_url_slashes}/${package_name_slashes}/CustomGLSurfaceView.java ${source_code}) # write it
    endif ()
endmacro()