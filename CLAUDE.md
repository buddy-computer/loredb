# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

LoreDB is a modern, embeddable property graph database engine written in C++23. It's designed for high performance, extensibility, and robust concurrency with ACID transactions.

## Build System & Commands

**C++23 Standard**: LoreDB now uses C++23 standard for improved performance and modern language features.

### Quick Build Commands
```bash
# Automated setup (recommended first time)
./setup.sh

# Manual build with optimizations
./build.sh --cpu apple-m4 --release

# Debug build for development
./build.sh --debug

# Clean build
./build.sh --clean
```

### C++23 Compiler Support
- **Apple Clang**: Uses `-std=c++2b` flag (automatically detected)
- **GCC 11+**: Uses `-std=c++23` flag
- **Clang 14+**: Uses `-std=c++23` flag

### Testing Commands
```bash
# Run all tests
./build/tests

# Run tests with CTest
cd build && ctest --output-on-failure

# Run specific test suites
./build/tests --gtest_filter="GraphStoreTest.*"
./build/tests --gtest_filter="CypherParserTest.*"
```

### Benchmarking
```bash
# Run performance benchmarks
./build/benchmarks

# Run CLI
./build/loredb-cli [database_file]
```

### CMake Direct Usage
```bash
# Configure (requires vcpkg)
cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build --config Release

# Enable static analysis
cmake -B build -DENABLE_STATIC_ANALYSIS=ON
```

## Architecture Overview

### Core Components

**Storage Layer** (`src/storage/`):
- `PageStore`: Abstract page-based storage interface
- `FilePageStore`: Memory-mapped file storage with persistence
- `GraphStore`: High-level graph operations (nodes, edges, properties)
- `WALManager`: Write-ahead logging for crash recovery and durability
- `SimpleIndexManager`: Property and adjacency indexing

**Query Engine** (`src/query/`):
- `QueryExecutor`: High-level query API for CRUD operations
- `cypher/Parser`: Cypher query language parser using PEG (Parsing Expression Grammar)
- `cypher/Executor`: Query execution engine with AST evaluation
- `cypher/ExpressionEvaluator`: Expression evaluation for Cypher queries
- `Planner`: Query optimization and execution planning

**Transaction System** (`src/transaction/`):
- `MVCCManager`: Multi-Version Concurrency Control for snapshot isolation
- `LockManager`: Lock acquisition and deadlock detection
- `TransactionManager`: Transaction lifecycle management

**Utilities** (`src/util/`):
- `Logger`: Structured logging with spdlog integration
- `expected.h`: Error handling using tl::expected (monadic error handling)
- `varint.cpp`: Variable-length integer encoding
- `crc32.cpp`: CRC32 checksums for data integrity

### Key Design Patterns

1. **Error Handling**: Uses `util::expected<T, Error>` for composable error handling
2. **MVCC**: Snapshot isolation with versioned records
3. **Memory Management**: RAII with smart pointers throughout
4. **Concurrency**: Lock-free data structures from Intel TBB where possible
5. **Persistence**: WAL-based durability with crash recovery

## Development Guidelines

### Code Style
- C++23 standard with modern features (concepts, ranges, std::expected)
- Follow existing naming conventions (snake_case for variables/functions)
- Use `util::expected<T, Error>` for error-prone operations (will migrate to `std::expected`)
- Prefer smart pointers and RAII for resource management
- Use structured logging with LOG_INFO, LOG_ERROR, LOG_PERFORMANCE macros
- **C++23 Note**: When using `unique_ptr` with forward-declared types, implement constructors/destructors in .cpp files

### Testing Approach
- GoogleTest framework for unit tests
- Test organization mirrors src/ structure
- Each major component has dedicated test files
- Use mocking with GoogleMock for dependencies
- Performance tests use Google Benchmark

### Dependencies (vcpkg managed)
- `tl-expected`: Monadic error handling
- `fmt`: String formatting
- `spdlog`: Structured logging
- `gtest`: Unit testing framework
- `benchmark`: Performance benchmarks
- `pegtl`: PEG parsing for Cypher
- `tbb`: Intel Threading Building Blocks

## Platform Specifics

### Apple Silicon Optimization
The build system is optimized for Apple Silicon with specific CPU targets:
- `apple-m1`, `apple-m2`, `apple-m3`, `apple-m4`
- Profile-Guided Optimization (PGO) support
- Link-Time Optimization (LTO) enabled in release builds

### Build Configurations
- **Debug**: `-g -O0` with AddressSanitizer and UndefinedBehaviorSanitizer
- **Release**: `-O3` with CPU-specific optimizations and LTO
- **Static Analysis**: clang-tidy and cppcheck integration

### C++23 Migration Status
- **Phase 1 Complete**: Updated build system to C++23 standard
- **Current Status**: All 87 tests passing, CLI and benchmarks working
- **Next Phases**: Replace `tl::expected` with `std::expected`, implement `std::flat_map`, add C++23 performance optimizations

## Common Development Patterns

### Error Handling Example
```cpp
auto result = storage->create_node(properties);
if (!result) {
    LOG_ERROR("Failed to create node: {}", result.error().message);
    return util::unexpected(result.error());
}
NodeId id = result.value();
```

### Transaction Usage
```cpp
auto tx = mvcc_manager->begin_transaction();
auto result = graph_store->create_node(properties, tx);
if (result) {
    mvcc_manager->commit_transaction(tx);
} else {
    mvcc_manager->abort_transaction(tx);
}
```

## Performance Considerations

- WAL writes are batched for better I/O performance
- MVCC uses versioned records to avoid blocking readers
- Indexes use B+ trees for range queries
- Memory-mapped files for efficient storage access
- Lock-free data structures where possible (Intel TBB)

## CLI Usage

The interactive CLI supports:
- Node/edge creation: `create-node name=Alice,age=30`
- Cypher queries: `cypher MATCH (n) RETURN n.name`
- Graph traversal commands
- Database introspection commands

## File Structure Notes

- Tests mirror the src/ directory structure
- Benchmarks are in `/benchmarks/` directory
- CMake builds all targets: `loredb` (static library), `loredb-cli`, `tests`, `benchmarks`
- vcpkg.json defines C++ dependencies
- build.sh and setup.sh provide automated build workflows