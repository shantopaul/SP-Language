# SP Compiler Executable (spc)
add_executable(spc
    spc/main.cpp
)

target_link_libraries(spc PRIVATE sp_compiler sp_runtime)
target_compile_features(spc PRIVATE cxx_std_20)
