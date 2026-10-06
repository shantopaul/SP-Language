# SP Compiler Executable (spc)
add_executable(spc
    spc/main.cpp
)
target_link_libraries(spc PRIVATE sp_compiler sp_runtime)
target_compile_features(spc PRIVATE cxx_std_20)

# SP Package Manager Core Library (spm_core)
add_library(spm_core
    spm/ProjectConfig.cpp
    spm/SemVer.cpp
    spm/DependencyResolver.cpp
    spm/Lockfile.cpp
)
target_include_directories(spm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(spm_core PUBLIC sp_compiler sp_runtime)
target_compile_features(spm_core PUBLIC cxx_std_20)

# SP Package Manager Executable (spm)
add_executable(spm
    spm/main.cpp
)
target_link_libraries(spm PRIVATE spm_core)
target_compile_features(spm PRIVATE cxx_std_20)
