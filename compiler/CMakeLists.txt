# SP Compiler Library
add_library(sp_compiler
    src/diagnostics/SourceLocation.cpp
    src/diagnostics/DiagnosticEngine.cpp
    src/lexer/TokenType.cpp
    src/lexer/Token.cpp
)

target_include_directories(sp_compiler
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(sp_compiler PUBLIC cxx_std_20)

if(LLVM_FOUND)
    llvm_map_components_to_libnames(llvm_libs core support native codegen)
    target_link_libraries(sp_compiler PUBLIC ${llvm_libs})
    target_compile_definitions(sp_compiler PUBLIC SP_HAS_LLVM=1)
else()
    target_compile_definitions(sp_compiler PUBLIC SP_HAS_LLVM=0)
endif()
