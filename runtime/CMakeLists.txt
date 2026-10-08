# SP Runtime Library
find_package(Threads REQUIRED)

add_library(sp_runtime
    src/Runtime.cpp
    src/threading/Concurrency.cpp
    src/threading/Channel.cpp
    src/threading/ThreadPool.cpp
)

target_include_directories(sp_runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(sp_runtime PUBLIC cxx_std_20)
target_link_libraries(sp_runtime PUBLIC Threads::Threads)

