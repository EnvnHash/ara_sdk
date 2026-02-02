include(Android/CreateCppJniInterface)

macro(create_app_cpp_sources
        #use_ad_mob use_billing
)
    create_cpp_jni_interface(
            #${use_ad_mob} ${use_billing}
    )
endmacro()