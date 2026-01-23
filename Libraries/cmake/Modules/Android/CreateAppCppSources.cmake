include(Android/CreateCppJniInterface)

macro(create_app_cpp_sources use_ad_mob)
    create_cpp_jni_interface(${use_ad_mob})
endmacro()