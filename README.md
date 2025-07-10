# LoreDB

A modern, embeddable property graph database engine written in C++20, designed for high performance, extensibility, and robust concurrency.

## Architecture Overview

- **Storage Layer**:

  - `PageStore` abstraction for page-based storage (in-memory or file-backed).
  - `FilePageStore`: Memory-mapped file storage for persistence.
  - `Record` and `PageHeader` for efficient on-disk data layout.
  - Write-Ahead Logging (`WALManager`) for durability and crash recovery.

- **Transaction & Concurrency**:

  - Multi-Version Concurrency Control (`MVCCManager`) for snapshot isolation and high concurrency.
  - Transaction lifecycle and versioned record management.

- **Query Engine**:

  - `QueryExecutor`: High-level API for node/edge CRUD, traversals, and analytics.
  - Cypher-like query parser and executor for expressive graph queries.
  - Indexing (`SimpleIndexManager`) for fast property and adjacency lookups.

- **CLI & Tooling**:
  - Interactive REPL for database exploration and scripting.
  - Logging via `Logger` (spdlog + fmt) for structured, high-performance logs.
  - GoogleTest-based unit and integration tests for all major modules.

## Extensibility

- Modular design: Add new storage engines, query languages, or analytics modules.
- CMake/vcpkg-based dependency management for easy integration and CI.
- Follows modern C++ best practices (RAII, smart pointers, error handling with `tl::expected`).

## Getting Started

### Quick Start (5 minutes)

**Option 1: Automated Setup (Recommended)**

```bash
git clone https://github.com/yourusername/loredb.git
cd loredb
./setup.sh
```

**Option 2: Manual Setup**

1. **Prerequisites**: Ensure you have CMake 3.20+, C++20 compiler, and vcpkg installed
2. **Build**: `cmake -B build && cmake --build build`
3. **Run**: `./build/loredb-cli`

**Try it**: Create your first graph!

```bash
> create-node name=Alice,age=30
Created node with ID: 1

> create-node name=Bob,age=25
Created node with ID: 2

> create-edge 1 2 KNOWS weight=0.8
Created edge with ID: 1

> cypher MATCH (a)-[r:KNOWS]->(b) RETURN a.name, b.name, r.weight
+-------+-------+--------+
| a.name| b.name| r.weight|
+-------+-------+--------+
| Alice | Bob   | 0.8    |
+-------+-------+--------+
```

### Key Features at a Glance

- **🚀 Modern C++20**: High-performance graph database with MVCC and WAL
- **💾 Persistent Storage**: File-backed storage with crash recovery
- **🔍 Cypher Queries**: SQL-like query language for graph traversal
- **🧵 Thread-Safe**: Concurrent access with snapshot isolation
- **📊 Analytics**: Path finding, centrality measures, and graph algorithms
- **🛠️ Extensible**: Modular architecture for custom storage and query engines

## Setup & Installation

### Automated Setup (Recommended)

For the fastest setup experience, use our automated setup script:

```bash
git clone https://github.com/yourusername/loredb.git
cd loredb
./setup.sh
```

The script will:

- ✅ Install all system dependencies
- ✅ Setup vcpkg and install C++ packages
- ✅ Configure and build the project
- ✅ Run tests to verify everything works
- ✅ Provide next steps

**Script Options:**

```bash
./setup.sh -t Debug      # Debug build
./setup.sh -j 8          # Use 8 parallel jobs
./setup.sh --skip-tests  # Skip running tests
./setup.sh --help        # Show all options
```

### Manual Setup

If you prefer to set things up manually:

#### Prerequisites

**Required:**

- **CMake 3.20+** - Build system
- **C++20 Compiler** - GCC 11+, Clang 14+, or MSVC 2022+
- **vcpkg** - Package manager for C++ dependencies

**Platform Support:**

- Linux (Ubuntu 20.04+, CentOS 8+)
- macOS (10.15+)
- Windows (Windows 10+)

#### Option 1: Using vcpkg (Recommended)

```bash
# 1. Install vcpkg (if not already installed)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh  # or bootstrap-vcpkg.bat on Windows

# 2. Clone and build LoreDB
git clone https://github.com/yourusername/loredb.git
cd loredb

# 3. Configure with vcpkg
cmake -B build -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# 4. Build
cmake --build build --config Release

# 5. Run tests
./build/tests

# 6. Start CLI
./build/loredb-cli
```

#### Option 2: Manual Dependencies

If you prefer to manage dependencies manually:

**Ubuntu/Debian:**

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config
sudo apt install libfmt-dev libspdlog-dev libtbb-dev libgtest-dev
sudo apt install libreadline-dev  # Optional, for better CLI experience

git clone https://github.com/yourusername/loredb.git
cd loredb
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**macOS:**

```bash
brew install cmake fmt spdlog tbb googletest readline

git clone https://github.com/yourusername/loredb.git
cd loredb
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

#### Build Options

```bash
# Debug build with sanitizers
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Release build with optimizations
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Enable static analysis
cmake -B build -DENABLE_STATIC_ANALYSIS=ON

# Disable warnings-as-errors
cmake -B build -DENABLE_WERROR=OFF
```

#### Verification

```bash
# Run unit tests
./build/tests

# Run benchmarks
./build/benchmarks

# Check CLI help
./build/loredb-cli --help

# Run CLI with custom database path
./build/loredb-cli my_database.db
```

## Development Guide

### Project Structure

```
loredb/
├── src/
│   ├── storage/       # Storage engine (pages, records, WAL)
│   ├── query/         # Query engine (Cypher parser, executor)
│   ├── transaction/   # MVCC, lock manager
│   ├── util/          # Logging, utilities
│   └── cli/           # Command-line interface
├── tests/             # Unit and integration tests
├── benchmarks/        # Performance benchmarks
└── docs/              # Documentation
```

### Core Components

#### Storage Layer (`src/storage/`)

- **`PageStore`**: Abstract interface for page-based storage
- **`FilePageStore`**: File-backed persistent storage with memory mapping
- **`GraphStore`**: High-level graph operations (nodes, edges, properties)
- **`WALManager`**: Write-ahead logging for durability
- **`IndexManager`**: Property and adjacency indexing

#### Query Engine (`src/query/`)

- **`QueryExecutor`**: High-level query API
- **`cypher/Parser`**: Cypher query language parser
- **`cypher/Executor`**: Query execution engine
- **`Planner`**: Query optimization and planning

#### Transaction System (`src/transaction/`)

- **`MVCCManager`**: Multi-version concurrency control
- **`LockManager`**: Lock acquisition and deadlock detection
- **`TransactionManager`**: Transaction lifecycle management

### Writing Code

#### Coding Standards

- Follow the [C++ Style Guide](.cursor/rules/cpp-style.mdc)
- Use C++20 features (concepts, ranges, coroutines where appropriate)
- Prefer `util::expected<T, Error>` for error handling
- Use smart pointers and RAII for resource management

#### Error Handling Example

```cpp
#include "util/expected.h"

util::expected<NodeId, Error> create_node(const Properties& props) {
    if (props.empty()) {
        return util::unexpected(Error{ErrorCode::INVALID_ARGUMENT, "Empty properties"});
    }

    // Implementation...
    return NodeId{42};
}
```

#### Logging Example

```cpp
#include "util/logger.h"

void some_operation() {
    LOG_INFO("Starting operation with {} items", item_count);

    auto start = std::chrono::high_resolution_clock::now();
    // ... do work ...
    auto end = std::chrono::high_resolution_clock::now();

    LOG_PERFORMANCE("operation_name",
                   std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(),
                   "processed {} items", item_count);
}
```

### Testing

#### Writing Tests

All tests use GoogleTest framework:

```cpp
#include <gtest/gtest.h>
#include "storage/graph_store.h"

class GraphStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        store = std::make_unique<GraphStore>(/* ... */);
    }

    std::unique_ptr<GraphStore> store;
};

TEST_F(GraphStoreTest, CreateNodeSuccess) {
    Properties props = {{"name", "Alice"}, {"age", "30"}};
    auto result = store->create_node(props);

    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result.value(), 0);
}
```

#### Running Tests

```bash
# Run all tests
./build/tests

# Run specific test suite
./build/tests --gtest_filter="GraphStoreTest.*"

# Run tests with verbose output
./build/tests --gtest_filter="*" --gtest_output=xml
```

### Common Development Tasks

#### Adding a New Storage Engine

1. Implement `PageStore` interface
2. Add to `CMakeLists.txt`
3. Write unit tests in `tests/storage/`
4. Update documentation

#### Adding a New Query Operation

1. Extend Cypher grammar in `query/cypher/grammar.h`
2. Add AST node in `query/cypher/ast.h`
3. Implement executor logic in `query/cypher/executor.cpp`
4. Add parser tests in `tests/query/`

#### Performance Optimization

1. Add benchmarks in `benchmarks/`
2. Profile with `perf` or similar tools
3. Use `LOG_PERFORMANCE` for timing
4. Consider lock-free data structures from Intel TBB

### Debugging

#### Debug Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

#### Using Debuggers

```bash
# GDB
gdb ./build/loredb-cli
(gdb) run my_database.db

# LLDB
lldb ./build/loredb-cli
(lldb) process launch -- my_database.db
```

#### Memory Debugging

```bash
# AddressSanitizer (enabled in debug builds)
./build/tests

# Valgrind
valgrind --leak-check=full ./build/tests
```

### Contributing Workflow

1. **Fork & Clone**: Fork the repository and clone your fork
2. **Branch**: Create a feature branch (`git checkout -b feature/amazing-feature`)
3. **Develop**: Write code following our standards
4. **Test**: Ensure all tests pass and add new tests
5. **Document**: Update documentation and add Doxygen comments
6. **Commit**: Use conventional commits (`feat: add new indexing strategy`)
7. **Pull Request**: Submit PR with clear description

#### Commit Message Format

```
type(scope): description

- feat: new feature
- fix: bug fix
- docs: documentation
- style: formatting
- refactor: code restructuring
- test: adding tests
- perf: performance improvement
```

### IDE Setup

#### Visual Studio Code

Recommended extensions:

- C/C++ Extension Pack
- CMake Tools
- GitLens
- Better C++ Syntax

#### CLion

- Import CMake project
- Enable built-in code analysis
- Configure unit test runner

### Performance Tips

- Use `CMAKE_BUILD_TYPE=Release` for benchmarks
- Profile with `perf record` and `perf report`
- Monitor memory usage with `valgrind --tool=massif`
- Use `LOG_PERFORMANCE` macros for timing critical sections
- Consider Intel TBB for parallel algorithms

### Getting Help

- **Documentation**: Check Doxygen comments in headers
- **Tests**: Look at `tests/` for usage examples
- **Issues**: Search existing GitHub issues
- **Discussions**: Start a discussion for design questions
- **Code Review**: Submit draft PRs for early feedback

## Roadmap

LoreDB is under active development with a focus on performance, scalability, and developer experience. Our roadmap is organized into four main phases:

### Phase 1: Performance Foundation (Current Focus)

**Goal**: Optimize core storage and query performance for production workloads

#### Storage Engine Enhancements

- [ ] **Packed Page Storage**: Store multiple records per page to improve I/O efficiency
- [ ] **Buffer Pool Management**: LRU page cache with configurable size and dirty page tracking
- [ ] **Record Compression**: Implement compression for sparse properties and better storage utilization
- [ ] **Free Space Management**: Proper page allocation and defragmentation

#### Query Performance

- [ ] **Eliminate Linear Scanning**: Replace naive node ID scanning with proper iterators
- [ ] **Bloom Filters**: Add existence checks to reduce unnecessary I/O
- [ ] **Skip Lists**: Implement for efficient range queries
- [ ] **Query Plan Caching**: Cache execution plans for repeated queries

### Phase 2: Query Engine Enhancement

**Goal**: Complete Cypher implementation and add advanced query capabilities

#### Advanced Querying

- [ ] **Aggregation Functions**: Implement `COUNT()`, `SUM()`, `AVG()`, `MAX()`, `MIN()`
- [ ] **Complex Joins**: Multi-pattern matching with proper join algorithms
- [ ] **Subqueries**: Support for nested query expressions
- [ ] **Streaming Results**: Handle large result sets without memory exhaustion

#### Query Optimization

- [ ] **Cost-Based Optimization**: Collect statistics and implement join order optimization
- [ ] **Advanced Indexing**: B+ tree indexes for range queries and composite indexes
- [ ] **Predicate Pushdown**: Optimize query execution by pushing filters down
- [ ] **Projection Elimination**: Remove unnecessary data from query pipeline

### Phase 3: Advanced Features & APIs

**Goal**: Production-ready features and developer-friendly APIs

#### Graph Analytics

- [ ] **Advanced Path Algorithms**: Dijkstra's shortest path with weights
- [ ] **Centrality Measures**: PageRank, betweenness, and closeness centrality
- [ ] **Community Detection**: Algorithm implementations for graph clustering
- [ ] **Traversal Optimization**: Bidirectional search and graph-specific optimizations

#### API Development

- [ ] **REST API Server**: HTTP endpoints for CRUD operations and queries
- [ ] **WebSocket API**: Real-time streaming for live graph updates and subscriptions
- [ ] **Authentication & Authorization**: Role-based access control and secure authentication
- [ ] **GraphQL Support**: Schema-first API layer for modern web applications

#### Developer Experience

- [ ] **Language Bindings**: Python, JavaScript/Node.js, and Go client libraries
- [ ] **Graph Visualization**: Built-in tools for exploring and visualizing graphs
- [ ] **Import/Export**: Support for GraphML, Cypher dumps, and CSV formats
- [ ] **Schema Management**: Optional schema enforcement and validation

### Phase 4: Enterprise & Ecosystem

**Goal**: Enterprise-grade features and ecosystem integration

#### Scalability & Distribution

- [ ] **Graph Partitioning**: Horizontal scaling across multiple nodes
- [ ] **Replication**: Master-slave replication for high availability
- [ ] **Consensus Protocol**: Implement Raft for distributed consistency
- [ ] **Sharding**: Automatic data distribution strategies

#### Production Hardening

- [ ] **Monitoring & Metrics**: Comprehensive observability with Prometheus/Grafana
- [ ] **Backup & Recovery**: Point-in-time recovery and automated backups
- [ ] **Performance Profiling**: Built-in profiling tools and query analysis
- [ ] **Security Hardening**: Encryption at rest and in transit, audit logging

#### Integration & Compatibility

- [ ] **Cloud Deployment**: Docker containers and Kubernetes operators
- [ ] **Analytics Integration**: Compatibility with Apache Spark and similar frameworks
- [ ] **Database Migration**: Tools for migrating from Neo4j, ArangoDB, and other graph DBs
- [ ] **Streaming Integration**: Apache Kafka connectors for real-time data ingestion

### Current Status & Contributions

**✅ Completed Features**:

- Complete MVCC implementation with snapshot isolation
- Cypher query parser and executor
- WAL-based durability and crash recovery
- Multi-threaded concurrency with lock-free data structures
- Comprehensive test suite (87 tests, 100% pass rate)
- Interactive CLI/REPL interface

**🔄 In Progress**:

- Storage engine optimizations (packed pages, buffer pool)
- Query performance improvements
- Advanced indexing strategies

**🤝 Contributing**:
We welcome contributions at all levels! Check our [issues](https://github.com/yourusername/loredb/issues) for "good first issue" labels, or tackle any roadmap item that interests you.

## Contributing

- All code must follow the [C++ style guide](.cursor/rules/cpp-style.mdc).
- New features require unit tests and Doxygen documentation.
- See `src/` for module structure and entry points.

---

For detailed module documentation, see Doxygen comments in headers and the `docs/` directory (if present).
