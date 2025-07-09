# Wiki Graph Database

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

## Contributing

- All code must follow the [C++ style guide](.cursor/rules/cpp-style.mdc).
- New features require unit tests and Doxygen documentation.
- See `src/` for module structure and entry points.

---

For detailed module documentation, see Doxygen comments in headers and the `docs/` directory (if present).
