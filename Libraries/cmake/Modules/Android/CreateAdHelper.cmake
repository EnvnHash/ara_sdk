macro (create_ad_helper)
    get_property(PACKAGE_URL TARGET ${PROJECT_NAME} PROPERTY ARASDK_PACKAGE_URL)
    get_property(PACKAGE_NAME TARGET ${PROJECT_NAME} PROPERTY ARASDK_PACKAGE_NAME)

    set(code)
    replace_dot_with_char(${PACKAGE_URL} "/" package_url_slashes)
    replace_dot_with_char(${PACKAGE_NAME} "/" package_name_slashes)

    if (${app_type} EQUAL 0)
        list(APPEND code "package ${PACKAGE_URL}.${PACKAGE_NAME}\;\n
import android.app.NativeActivity\;

public class AdHelper {
}")
        FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/java/${package_url_slashes}/${package_name_slashes}/MainActivity.java ${code}) # write it
    else ()
        # package
        list(APPEND code "package ${PACKAGE_URL}.${PACKAGE_NAME}\;\n\n")

        list(APPEND code "import android.app.Activity\;
import android.util.Log\;
import android.view.View\;
import android.view.ViewGroup\;
import android.widget.RelativeLayout\;
import androidx.annotation.NonNull\;
import com.google.android.gms.ads.AdError\;
import com.google.android.gms.ads.AdRequest\;
import com.google.android.gms.ads.FullScreenContentCallback\;
import com.google.android.gms.ads.LoadAdError\;
import com.google.android.gms.ads.interstitial.InterstitialAd\;
import com.google.android.gms.ads.interstitial.InterstitialAdLoadCallback\;
import com.google.android.gms.ads.MobileAds\;
import com.google.android.gms.ads.AdView\;
import com.google.android.gms.ads.AdSize\;
import com.google.android.gms.ads.AdRequest\;

")

        # class begin
        list(APPEND code "public class AdHelper {
    private static final String TAG = ${PROJECT_NAME}Activity.class.getSimpleName()\;

    private InterstitialAd mInterstitialAd\;
    private final Activity activity\;
    private static AdView m_stdAdView\;
")

        # class ctor
        list(APPEND code "
    public AdHelper(Activity activity) {
        this.activity = activity\;
    }\;
")

        # class laodStdAd
        list(APPEND code "
    public void loadStandardAd(String adUnitId, RelativeLayout parentLayout) {
        new Thread(() -> {
            MobileAds.initialize(activity, initializationStatus -> {})\;
        }).start()\;

        m_stdAdView = new AdView(activity)\;
        m_stdAdView.setAdUnitId(adUnitId)\;
        m_stdAdView.setAdSize(AdSize.getCurrentOrientationAnchoredAdaptiveBannerAdSize(activity, 360))\;

        RelativeLayout adContainerView = new RelativeLayout(activity)\;
        RelativeLayout.LayoutParams layoutParams = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.MATCH_PARENT,
                RelativeLayout.LayoutParams.MATCH_PARENT
        )\;
        adContainerView.setLayoutParams(layoutParams)\;
        adContainerView.removeAllViews()\;

        RelativeLayout.LayoutParams adViewLayout =
                new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)\;
        adViewLayout.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM)\;
        adContainerView.addView(m_stdAdView, adViewLayout)\;

        //relativeLayout.addView(adContainerView)\;
        parentLayout.addView(adContainerView)\;

        AdRequest adRequest = new AdRequest.Builder().build()\;
        m_stdAdView.loadAd(adRequest)\;
        m_stdAdView.setVisibility(View.INVISIBLE)\;
    }

    public void showStandardAd(boolean val) {
        if (m_stdAdView == null) {
            return\;
        }

        if (val) {
            m_stdAdView.setVisibility(View.VISIBLE)\;
        } else {
            m_stdAdView.setVisibility(View.INVISIBLE)\;
        }
    }
")

        # class interstitial ad
        list(APPEND code "
    public void loadInterstitialAd(String adUnitId) {
        InterstitialAd.load(
            activity,
            adUnitId,
            new AdRequest.Builder().build(),
            new InterstitialAdLoadCallback() {
                @Override
                public void onAdLoaded(@NonNull InterstitialAd interstitialAd) {
                    Log.d(TAG, \"[debug] Interstitial loaded successfully\")\;
                    mInterstitialAd = interstitialAd\;

                    interstitialAd.setFullScreenContentCallback(
                        new FullScreenContentCallback() {
                          @Override
                          public void onAdDismissedFullScreenContent() {
                            Log.d(TAG, \"[debug] The ad was dismissed.\")\;
                            mInterstitialAd = null\;
                          }

                          @Override
                          public void onAdFailedToShowFullScreenContent(AdError adError) {
                            Log.d(TAG, \"[debug] The ad failed to show.\")\;
                            mInterstitialAd = null\;
                          }

                          @Override
                          public void onAdShowedFullScreenContent() {
                            Log.d(TAG, \"[debug] The ad was shown.\")\;
                          }

                          @Override
                          public void onAdImpression() {
                            Log.d(TAG, \"[debug] The ad recorded an impression.\")\;
                          }

                          @Override
                          public void onAdClicked() {
                            Log.d(TAG, \"[debug] The ad was clicked.\")\;
                          }
                        })\;
                }

                @Override
                public void onAdFailedToLoad(@NonNull LoadAdError loadAdError) {
                    Log.d(TAG, \"[debug] Interstitial ad failed to load: \" + loadAdError.getMessage())\;
                    mInterstitialAd = null\;
                }
            })\;
    }

    public boolean showInterstitialAd() {
        if (mInterstitialAd != null) {
            mInterstitialAd.show(activity)\;
            return true\;
        } else {
            Log.d(\"AdHelper\", \"The interstitial ad wasn't ready yet.\")\;
            return false\;
        }
    }
")

        # class end
        list(APPEND code "}\n\n")
        FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/java/${package_url_slashes}/${package_name_slashes}/AdHelper.java ${code}) # write it
    endif ()
endmacro()
