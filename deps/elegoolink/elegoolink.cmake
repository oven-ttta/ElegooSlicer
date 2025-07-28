
elegooslicer_add_cmake_project(elegoolink
    GIT_REPOSITORY      https://github.com/wujie-elegoo/elegoolink.git
    GIT_TAG             v0.0.1


    DEPENDS 
        dep_ixwebsocket 
        dep_PahoMqttCpp
    # CMAKE_ARGS 
    #     -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
    #     -DVCPKG_TARGET_TRIPLET=x64-windows-static-md
)