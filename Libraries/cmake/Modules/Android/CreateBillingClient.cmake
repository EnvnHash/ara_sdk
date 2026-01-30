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
    private BillingClient billingClient\;
    private PurchasesUpdatedListener purchasesUpdatedListener = new PurchasesUpdatedListener() {
        @Override
        public void onPurchasesUpdated(BillingResult billingResult, List<Purchase> purchases) {
            // To be implemented in a later section.
        }
    }\;
    private PendingPurchasesParams purchasesParams\;

    public BillingManager(Context context) {
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
            public void onBillingSetupFinished(BillingResult billingResult) {
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
                        .setProductType(BillingClient.ProductType.SUBS)
                        .build()))
            .build()\;

        billingClient.queryProductDetailsAsync(
            queryProductDetailsParams,
            new ProductDetailsResponseListener() {
                public void onProductDetailsResponse(BillingResult billingResult, @NonNull QueryProductDetailsResult queryProductDetailsResult) {
                    if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                       for (ProductDetails productDetails : queryProductDetailsResult.getProductDetailsList()) {
                         // Process success retrieved product details here.
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
    public void purchaseProduct(String productId) {
        Log.i(TAG, \"[debug] purchaseProduct: \")\;
    /*
        if (billingClient.isReady()) {
            FlowParams flowParams = FlowParams.newBuilder()
                .setProductId(productId)
                .build()\;

            LaunchBillingFlowParams launchBillingFlowParams = LaunchBillingFlowParams.newBuilder()
                .setSkuDetails(null) // You can pass SKU details here if you have them cached
                .setFlowParams(flowParams)
                .build()\;

            billingClient.launchBillingFlow(launchBillingFlowParams, (BillingResult billingResult) -> {
                if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                    Log.d(TAG, \"[debug] Purchase flow launched successfully\")\;
                } else {
                    Log.e(TAG, \"[debug] Purchase flow launch failed: \" + billingResult.getResponseCode())\;
                }
            })\;
        } else {
            Log.w(TAG, \"[debug] Billing not ready.\");
        }
        */
    }
/*
    // Handle Purchases (This is the most important part!)
    public void handlePurchases(List<Purchase> purchases) {
        for (Purchase purchase : purchases) {
            if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
                Log.d(TAG, \"[debug] Purchase successful: \" + purchase.getOrderId())\;

                // 1. Unlock the feature/content associated with this product
                unlockContent(purchase.getSku())\;

                // 2. Acknowledge the purchase (VERY IMPORTANT!)
                acknowledgePurchase(purchase)\;
            } else if (purchase.getPurchaseState() == Purchase.PurchaseState.PENDING) {
                Log.i(TAG, \"[debug] Purchase pending: \" + purchase.getOrderId())\;
            } else {
                Log.w(TAG, \"[debug] Purchase failed or cancelled: \" + purchase.getOrderId())\;
            }
        }
    }

    private void unlockContent(String sku) {
        // Implement your logic to unlock the purchased content here.
        // This could involve enabling a feature, granting access to levels, etc.
        Log.d(TAG, \"[debug] Unlocking content for SKU: \" + sku)\;
    }

    private void acknowledgePurchase(Purchase purchase) {
        billingClient.acknowledgePurchaseAsync(purchase.getPurchaseToken(), (BillingResult billingResult) -> {
            if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                Log.d(TAG, \"[debug] Purchase acknowledged successfully.\")\;
            } else {
                Log.e(TAG, \"[debug] Failed to acknowledge purchase: \" + billingResult.getResponseCode())\;
            }
        })\;
    }
*/
    // Disconnect the billing client when your activity/app is closing
    public void disconnectBillingClient() {
        billingClient.endConnection()\;
    }
}")
        FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/java/${package_url_slashes}/${package_name_slashes}/BillingManager.java ${billing_client}) # write it
    endif ()
endmacro()