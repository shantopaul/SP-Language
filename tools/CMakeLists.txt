# SP Formatter Library (sp_fmt)
add_library(sp_fmt
    fmt/Formatter.cpp
)
target_include_directories(sp_fmt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sp_fmt PUBLIC sp_compiler)
target_compile_features(sp_fmt PRIVATE cxx_std_20)

# SP Test Runner Library (sp_test_runner)
add_library(sp_test_runner
    test/TestRunner.cpp
)
target_include_directories(sp_test_runner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sp_test_runner PUBLIC sp_compiler sp_runtime)
target_compile_features(sp_test_runner PRIVATE cxx_std_20)

# SP Documentation Generator Library (sp_doc_generator)
add_library(sp_doc_generator
    doc/DocGenerator.cpp
)
target_include_directories(sp_doc_generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sp_doc_generator PUBLIC sp_compiler)
target_compile_features(sp_doc_generator PRIVATE cxx_std_20)

# SP Compiler Executable (spc)
add_executable(spc
    spc/main.cpp
)
target_link_libraries(spc PRIVATE sp_compiler sp_runtime sp_fmt sp_test_runner sp_doc_generator)
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

# SP Language Server Library (sp_lsp_server)
add_library(sp_lsp_server
    spls/LspServer.cpp
)
target_include_directories(sp_lsp_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sp_lsp_server PUBLIC sp_compiler sp_fmt sp_doc_generator)
target_compile_features(sp_lsp_server PRIVATE cxx_std_20)

# SP Language Server Executable (spls)
add_executable(spls
    spls/main.cpp
)
target_link_libraries(spls PRIVATE sp_lsp_server)
target_compile_features(spls PRIVATE cxx_std_20)
