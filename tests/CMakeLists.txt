include(FetchContent)

# Option to use FetchContent for GoogleTest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# For Windows / MinGW
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
if(WIN32)
    set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
    add_compile_definitions(GTEST_HAS_PTHREAD=0)
endif()

FetchContent_MakeAvailable(googletest)

if(TARGET gtest)
    if(WIN32)
        target_compile_definitions(gtest PUBLIC GTEST_HAS_PTHREAD=0)
        target_compile_definitions(gtest_main PUBLIC GTEST_HAS_PTHREAD=0)
    endif()
endif()

find_package(Threads)

add_executable(sp_tests
    diagnostics/DiagnosticEngineTest.cpp
    lexer/TokenTypeTest.cpp
    lexer/LexerTest.cpp
    lexer/GoldenLexerTest.cpp
    parser/ParserTest.cpp
    parser/GoldenParserTest.cpp
    sema/TypeTest.cpp
    sema/SymbolTableTest.cpp
    sema/TypeCheckerTest.cpp
    sema/GoldenSemanticTest.cpp
    ir/IRTest.cpp
    ir/IRBuilderTest.cpp
    codegen/LLVMGeneratorTest.cpp
    codegen/ExecutionTest.cpp
    codegen/VTableBuilderTest.cpp
    codegen/ClassGeneratorTest.cpp
    runtime/RuntimeTest.cpp
    spm/ProjectConfigTest.cpp
    spm/DependencyResolverTest.cpp
    spm/LockfileTest.cpp
    spm/BuildOrchestratorTest.cpp
    fmt/FormatterTest.cpp
    test/TestRunnerTest.cpp
    doc/DocGeneratorTest.cpp
    spls/LspServerTest.cpp
)

target_include_directories(sp_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/tools
)

target_link_libraries(sp_tests
    PRIVATE
        sp_compiler
        sp_runtime
        spm_core
        sp_fmt
        sp_test_runner
        sp_doc_generator
        sp_lsp_server
        GTest::gtest_main
)

if(TARGET Threads::Threads)
    target_link_libraries(sp_tests PRIVATE Threads::Threads)
endif()

if(WIN32 AND MINGW)
    target_link_libraries(sp_tests PRIVATE -lpthread)
endif()

target_include_directories(sp_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(sp_tests PRIVATE cxx_std_20)
target_compile_definitions(sp_tests PRIVATE SP_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

include(GoogleTest)
gtest_discover_tests(sp_tests)
