
elegooslicer_add_cmake_project(elegoolink
    GIT_REPOSITORY      https://github.com/wujie-elegoo/elegoolink.git
    GIT_TAG             v0.0.10


    DEPENDS 
        dep_ixwebsocket 
        dep_PahoMqttCpp
    CMAKE_ARGS 
        -DBUILD_EXAMPLES=OFF
        -DBUILD_TESTS=OFF
        -DBUILD_EXECUTABLE=OFF
        -DBUILD_SHARED_LIBS=OFF
)