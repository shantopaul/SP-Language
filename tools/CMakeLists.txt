# SP Formatter Library (sp_fmt)
add_library(sp_fmt
    fmt/Formatter.cpp
)
target_include_directories(sp_fmt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sp_fmt PUBLIC sp_compiler)
target_compile_features(sp_fmt PRIVATE cxx_std_20)

# SP Compiler Executable (spc)
add_executable(spc
    spc/main.cpp
)
target_link_libraries(spc PRIVATE sp_compiler sp_runtime sp_fmt)
target_compile_features(spc PRIVATE cxx_std_20)

# SP Package Manager Core Library (spm_core)
add_library(spm_core
    spm/ProjectConfig.cpp
    spm/SemVer.cpp
    spm/DependencyResolver.cpp
    spm/Lockfile.cpp
    spm/BuildOrchestrator.cpp
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
