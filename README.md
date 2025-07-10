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

- Build with CMake 3.20+ and vcpkg dependencies.
- Run `loredb-cli` for the interactive shell.
- See `tests/` for usage examples and test coverage.

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
