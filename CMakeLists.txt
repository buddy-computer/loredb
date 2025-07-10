cmake_minimum_required(VERSION 3.20)
project(loredb CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# === Platform and CPU tuning ===
if(APPLE)
  # Target ARM64 on Apple Silicon
  set(CMAKE_OSX_ARCHITECTURES "arm64")
  # Allow choosing CPU variant (apple-m1 or apple-m2)
  set(APPLE_CPU "apple-m1" CACHE STRING "Set Apple Silicon CPU: apple-m1 or apple-m2")
endif()

# === Build options ===
option(ENABLE_STATIC_ANALYSIS "Run static analysis with clang-tidy / cppcheck" ON)
option(ENABLE_WERROR "Treat warnings as errors" ON)
option(USE_PGO "Enable Profile-Guided Optimization" OFF)

# === Compiler warnings ===
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic)
    
    # GCC-specific: disable false positive stringop-overflow warnings with fmt library
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-Wno-stringop-overflow)
        if (CLANG_TIDY_EXE)
            set(CMAKE_CXX_CLANG_TIDY
                "${CLANG_TIDY_EXE};-config=;--extra-arg=-Wno-stringop-overflow") # remove or override
        endif()
    endif()
    
    if (ENABLE_WERROR)
        add_compile_options(-Werror)
    endif()
elseif (MSVC)
    add_compile_options(/W4)
    if (ENABLE_WERROR)
        add_compile_options(/WX)
    endif()
endif()

# === Static analysis ===
if (ENABLE_STATIC_ANALYSIS)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if (CLANG_TIDY_EXE)
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    endif()

    find_program(CPPCHECK_EXE NAMES cppcheck)
    if (CPPCHECK_EXE)
        message(STATUS "cppcheck found: ${CPPCHECK_EXE}")
        set(CMAKE_CXX_CPPCHECK "${CPPCHECK_EXE};--enable=all;--inline-suppr;--std=c++20")
    endif()
endif()

# === Build flags per configuration ===
# Debug: sanitizers and no optimization
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address -fsanitize=undefined")
# Release: high optimization, Apple Silicon tuning, LTO, and optional PGO
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
if(APPLE)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mcpu=${APPLE_CPU} -flto")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -flto")
endif()

# PGO flags
if(USE_PGO)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # First run: use -fprofile-generate, then rerun with -fprofile-use
        add_compile_options(-fprofile-generate)
        # After collecting data, build with -fprofile-use
        # add_compile_options(-fprofile-use)
    endif()
endif()

# === Find dependencies ===
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# vcpkg dependencies
find_package(tl-expected CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)
find_package(pegtl CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(Readline)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# === Core library ===
add_library(loredb STATIC
    src/storage/page_store.cpp
    src/storage/file_page_store.cpp
    src/storage/graph_store.cpp
    src/storage/record.cpp
    src/storage/simple_index_manager.cpp
    src/query/executor.cpp
    src/query/cypher/parser.cpp
    src/query/cypher/executor.cpp
    src/query/cypher/expression_evaluator.cpp
    src/query/planner.cpp
    src/query/execution_plan.cpp
    src/transaction/mvcc.cpp
    src/transaction/mvcc_manager.cpp
    src/transaction/lock_manager.cpp
    src/storage/wal_manager.cpp
    src/util/varint.cpp
    src/util/crc32.cpp
    src/util/logger.cpp
)
target_link_libraries(loredb
    Threads::Threads
    tl::expected
    fmt::fmt
    spdlog::spdlog
    taocpp::pegtl
    TBB::tbb
)
# Enable LTO on the core library
set_target_properties(loredb PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

# === CLI executable ===
add_executable(loredb-cli
    src/cli/main.cpp
    src/cli/repl.cpp
)
if(Readline_FOUND)
    target_link_libraries(loredb-cli loredb Readline::Readline)
    target_compile_definitions(loredb-cli PRIVATE HAVE_READLINE)
else()
    target_link_libraries(loredb-cli loredb)
endif()
# LTO for CLI
set_target_properties(loredb-cli PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

# === Tests ===
add_executable(tests
    tests/storage/test_page_store.cpp
    tests/storage/test_file_page_store.cpp
    tests/storage/test_index_manager.cpp
    tests/storage/test_wal_manager.cpp
    tests/query/test_executor.cpp
    tests/query/test_cypher_parser.cpp
    tests/util/test_varint.cpp
    tests/transaction/test_mvcc.cpp
    tests/transaction/test_mvcc_graph.cpp
)
target_link_libraries(tests
    loredb
    GTest::gtest_main
    GTest::gmock_main
)

# === Benchmarks ===
add_executable(benchmarks
    benchmarks/storage_benchmark.cpp
    benchmarks/query_benchmark.cpp
)
target_link_libraries(benchmarks
    loredb
    benchmark::benchmark
    benchmark::benchmark_main
)

# === Testing ===
enable_testing()
add_test(NAME unit_tests COMMAND tests)
