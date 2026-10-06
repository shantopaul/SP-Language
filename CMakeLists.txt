cmake_minimum_required(VERSION 3.20)

project(SP_Language
    VERSION 0.5.0
    DESCRIPTION "SP Programming Language Compiler and Toolchain"
    HOMEPAGE_URL "https://github.com/shantopaul/SP-Language"
    LANGUAGES C CXX
)

# Standard C++ configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(SP_BUILD_TESTS "Build the SP test suite" ON)
option(SP_ENABLE_LLVM "Enable LLVM code generation backend" ON)
option(SP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# Platform and compiler flags
if(MSVC)
    add_compile_options(/W4 /permissive-)
    if(SP_WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(SP_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# LLVM Configuration
if(SP_ENABLE_LLVM)
    find_package(LLVM 17 CONFIG QUIET)
    if(LLVM_FOUND)
        message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
        message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
        include_directories(${LLVM_INCLUDE_DIRS})
        add_definitions(${LLVM_DEFINITIONS})
    else()
        message(STATUS "LLVM 17+ not found in system paths. LLVM backend features will be prepared with mock/stub backend for early phase development.")
    endif()
endif()

# Core subdirectories
add_subdirectory(compiler)
add_subdirectory(runtime)
add_subdirectory(tools)

# Testing
if(SP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation rules (Distribution package like Java JDK / C++ GCC)
include(GNUInstallDirs)

install(TARGETS spc spm
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(TARGETS sp_compiler sp_runtime
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY runtime/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY compiler/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY runtime/src/
    DESTINATION share/sp/runtime
)

install(DIRECTORY stdlib/
    DESTINATION share/sp/stdlib
)

install(DIRECTORY stdlib/
    DESTINATION stdlib
)
