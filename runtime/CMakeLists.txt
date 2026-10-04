# SP Runtime Library
add_library(sp_runtime
    src/Runtime.cpp
)

target_include_directories(sp_runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(sp_runtime PUBLIC cxx_std_20)
