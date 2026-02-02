macro (create_billing_client app_type product_id)
    set(billing_client)
    replace_dot_with_char(${PACKAGE_URL} "/" package_url_slashes)
    replace_dot_with_char(${PACKAGE_NAME} "/" package_name_slashes)

    if (${app_type} EQUAL 0)
        list(APPEND billing_client "package ${PACKAGE_URL}.${PACKAGE_NAME}\;\n
import android.app.NativeActivity\;

public class BillingManager {
}")
        FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/java/${package_url_slashes}/${package_name_slashes}/BillingClient.java ${billing_client}) # write it
    else ()
        # package
        list(APPEND billing_client "package ${PACKAGE_URL}.${PACKAGE_NAME}\;\n\n")

        list(APPEND billing_client "import android.app.Activity\;
import android.util.Log\;
import android.content.Context\;
import androidx.annotation.NonNull\;
import com.android.billingclient.api.BillingClient\;
import com.android.billingclient.api.BillingClientStateListener\;
import com.android.billingclient.api.BillingFlowParams\;
import com.android.billingclient.api.BillingResult\;
import com.android.billingclient.api.PendingPurchasesParams\;
import com.android.billingclient.api.ProductDetails\;
import com.android.billingclient.api.ProductDetailsResponseListener\;
import com.android.billingclient.api.Purchase\;
import com.android.billingclient.api.PurchasesUpdatedListener\;
import com.android.billingclient.api.QueryProductDetailsParams\;
import com.android.billingclient.api.QueryProductDetailsResult\;
import com.android.billingclient.api.UnfetchedProduct\;
import java.util.List\;
import com.google.common.collect.ImmutableList\;

public class BillingManager {
")

        list(APPEND billing_client "
    private static final String TAG = \"BillingManager\"\;
    private final BillingClient billingClient\;
    private static ProductDetails m_productDetails\;
    private static Context m_context\;
    private static List<ProductDetails> m_productDetailsList\;
    private PurchasesUpdatedListener purchasesUpdatedListener = new PurchasesUpdatedListener() {
        @Override
        public void onPurchasesUpdated(BillingResult billingResult, List<Purchase> purchases) {
            if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK
                && purchases != null) {
                for (Purchase purchase : purchases) {
                    // Process the purchase as described in the next section.
                }
            } else if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.USER_CANCELED) {
                // Handle an error caused by a user canceling the purchase flow.
                Log.i(TAG, \"[debug] BillingResponseCode.USER_CANCELED\")\;
            } else {
                // Handle any other error codes.
                Log.i(TAG, \"[debug] BillingResponse other error\")\;
            }
        }
    }\;
    private PendingPurchasesParams purchasesParams\;

    public BillingManager(Context context) {
        m_context = context\;
        billingClient = BillingClient.newBuilder(context)
            .setListener(purchasesUpdatedListener) // Set listener for purchase updates
            .enablePendingPurchases(PendingPurchasesParams.newBuilder().enableOneTimeProducts().build()) // Alternative approach for newer versions
            .enableAutoServiceReconnection() // Add this line to enable reconnection
            .build()\;

        startBillingFlow()\;
    }

    private void startBillingFlow() {
        billingClient.startConnection(new BillingClientStateListener() {
            @Override
            public void onBillingSetupFinished(@NonNull BillingResult billingResult) {
                if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                    // The BillingClient is ready. You can query purchases here.
                    Log.d(TAG, \"[debug] Billing connected successfully\")\;
                    queryProductDetails()\; // Call this after connection is established
                } else {
                    Log.e(TAG, \"[debug] Billing connection failed: \")\;
                }
            }

            @Override
            public void onBillingServiceDisconnected() {
                // Try to restart the connection on the next request to
                // Google Play by calling the startConnection() method.
            }
        })\;
    }

    // Query Product Details
    public void queryProductDetails() {
        QueryProductDetailsParams queryProductDetailsParams =
        QueryProductDetailsParams.newBuilder()
            .setProductList(
                ImmutableList.of(
                    QueryProductDetailsParams.Product.newBuilder()
                        .setProductId(\"${product_id}\")
                        .setProductType(BillingClient.ProductType.INAPP)
                        .build()))
            .build()\;

        billingClient.queryProductDetailsAsync(
            queryProductDetailsParams,
            new ProductDetailsResponseListener() {
                public void onProductDetailsResponse(@NonNull BillingResult billingResult, @NonNull QueryProductDetailsResult queryProductDetailsResult) {
                    if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                        m_productDetailsList = queryProductDetailsResult.getProductDetailsList()\;
                        for (ProductDetails productDetails : m_productDetailsList) {
                            m_productDetails = productDetails\;
                        }

                        for (UnfetchedProduct unfetchedProduct : queryProductDetailsResult.getUnfetchedProductList()) {
                            // Handle any unfetched products as appropriate.
                        }
                    }
                }
            }
        )\;
    }

    // Launch the Purchase Flow
    public void purchaseProduct() {
        Log.i(TAG, \"[debug] purchaseProduct: \")\;

        List<ProductDetails.OneTimePurchaseOfferDetails> selectedOfferTokens = m_productDetails.getOneTimePurchaseOfferDetailsList()\;

        assert selectedOfferTokens != null\;
        assert selectedOfferTokens.get(0).getOfferToken() != null\;
        ImmutableList<BillingFlowParams.ProductDetailsParams> productDetailsParamsList =
            ImmutableList.of(
                BillingFlowParams.ProductDetailsParams.newBuilder()
                    .setProductDetails(m_productDetails)
                    .setOfferToken(selectedOfferTokens.get(0).getOfferToken())
                    .build()
            )\;

        BillingFlowParams billingFlowParams = BillingFlowParams.newBuilder()
            .setProductDetailsParamsList(productDetailsParamsList)
            .build()\;

        // Launch the billing flow
        BillingResult billingResult = billingClient.launchBillingFlow((Activity) m_context, billingFlowParams)\;
    }

    // Disconnect the billing client when your activity/app is closing
    public void disconnectBillingClient() {
        billingClient.endConnection()\;
    }
}")
        FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/java/${package_url_slashes}/${package_name_slashes}/BillingManager.java ${billing_client}) # write it
    endif ()
endmacro()