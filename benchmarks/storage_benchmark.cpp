#include <benchmark/benchmark.h>
#include "../src/storage/file_page_store.h"
#include "../src/storage/graph_store.h"
#include "../src/storage/simple_index_manager.h"
#include "../src/query/executor.h"
#include <random>
#include <string>
#include <vector>
#include <unistd.h>

using namespace loredb::storage;
using namespace loredb::query;

class BenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        // Create temporary file for testing
        std::string db_path = "/tmp/loredb_benchmark_" + std::to_string(getpid()) + ".db";
        
        auto page_store = std::make_unique<FilePageStore>(db_path);
        graph_store_ = std::make_unique<GraphStore>(std::move(page_store));
        index_manager_ = std::make_unique<SimpleIndexManager>();

        // Create non-owning shared_ptrs so QueryExecutor can use them
        auto gs_shared = std::shared_ptr<GraphStore>(graph_store_.get(), [](GraphStore*){});
        auto idx_shared = std::shared_ptr<SimpleIndexManager>(index_manager_.get(), [](SimpleIndexManager*){});

        query_executor_ = std::make_unique<QueryExecutor>(gs_shared, idx_shared);
        
        // Initialize random number generator
        rng_.seed(42);
    }
    
    void TearDown(const benchmark::State& state) override {
        // Destroy QueryExecutor first (non-owning), then owned objects
        query_executor_.reset();
        graph_store_.reset();
        index_manager_.reset();
        
        // Clean up temporary file
        std::string db_path = "/tmp/loredb_benchmark_" + std::to_string(getpid()) + ".db";
        unlink(db_path.c_str());
    }
    
    std::vector<Property> generate_random_properties(size_t count) {
        std::vector<Property> properties;
        properties.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            std::string key = "prop_" + std::to_string(i);
            std::string value = "value_" + std::to_string(rng_() % 1000);
            properties.emplace_back(key, PropertyValue{value});
        }
        
        return properties;
    }
    
    std::vector<NodeId> create_test_nodes(size_t count) {
        std::vector<NodeId> node_ids;
        node_ids.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            auto properties = generate_random_properties(5);
            auto result = graph_store_->create_node(properties);
            if (result.has_value()) {
                node_ids.push_back(result.value());
            }
        }
        
        return node_ids;
    }
    
    std::vector<EdgeId> create_test_edges(const std::vector<NodeId>& node_ids, size_t count) {
        std::vector<EdgeId> edge_ids;
        edge_ids.reserve(count);
        
        std::uniform_int_distribution<size_t> node_dist(0, node_ids.size() - 1);
        
        for (size_t i = 0; i < count; ++i) {
            NodeId from_node = node_ids[node_dist(rng_)];
            NodeId to_node = node_ids[node_dist(rng_)];
            
            if (from_node != to_node) {
                auto properties = generate_random_properties(3);
                auto result = graph_store_->create_edge(from_node, to_node, "links_to", properties);
                if (result.has_value()) {
                    edge_ids.push_back(result.value());
                }
            }
        }
        
        return edge_ids;
    }

protected:
    std::unique_ptr<GraphStore> graph_store_;
    std::unique_ptr<SimpleIndexManager> index_manager_;
    std::unique_ptr<QueryExecutor> query_executor_;
    std::mt19937 rng_;
};

// Node creation benchmarks
BENCHMARK_DEFINE_F(BenchmarkFixture, NodeCreation)(benchmark::State& state) {
    for (auto _ : state) {
        auto properties = generate_random_properties(5);
        auto result = graph_store_->create_node(properties);
        if (!result.has_value()) {
            state.SkipWithError("Failed to create node");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(BenchmarkFixture, NodeCreation)->Range(1, 1000000);

// Batch node creation benchmark
BENCHMARK_DEFINE_F(BenchmarkFixture, BatchNodeCreation)(benchmark::State& state) {
    const size_t batch_size = state.range(0);
    
    for (auto _ : state) {
        std::vector<std::vector<Property>> node_properties;
        node_properties.reserve(batch_size);
        
        for (size_t i = 0; i < batch_size; ++i) {
            node_properties.push_back(generate_random_properties(5));
        }
        
        std::vector<NodeId> created_nodes;
        auto result = graph_store_->batch_create_nodes(node_properties, created_nodes);
        if (!result.has_value()) {
            state.SkipWithError("Failed to create batch nodes");
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}

BENCHMARK_REGISTER_F(BenchmarkFixture, BatchNodeCreation)->Range(10, 10000);

// Edge creation benchmarks
BENCHMARK_DEFINE_F(BenchmarkFixture, EdgeCreation)(benchmark::State& state) {
    // Pre-create some nodes
    auto node_ids = create_test_nodes(1000);
    std::uniform_int_distribution<size_t> node_dist(0, node_ids.size() - 1);
    
    for (auto _ : state) {
        NodeId from_node = node_ids[node_dist(rng_)];
        NodeId to_node = node_ids[node_dist(rng_)];
        
        if (from_node != to_node) {
            auto properties = generate_random_properties(3);
            auto result = graph_store_->create_edge(from_node, to_node, "links_to", properties);
            if (!result.has_value()) {
                state.SkipWithError("Failed to create edge");
            }
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(BenchmarkFixture, EdgeCreation)->Range(1, 1000000);

// Node lookup benchmarks
BENCHMARK_DEFINE_F(BenchmarkFixture, NodeLookup)(benchmark::State& state) {
    // Pre-create nodes
    auto node_ids = create_test_nodes(state.range(0));
    std::uniform_int_distribution<size_t> node_dist(0, node_ids.size() - 1);
    
    for (auto _ : state) {
        NodeId node_id = node_ids[node_dist(rng_)];
        auto result = graph_store_->get_node(node_id);
        if (!result.has_value()) {
            state.SkipWithError("Failed to get node");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(BenchmarkFixture, NodeLookup)->Range(1000, 100000);

// Edge lookup benchmarks
BENCHMARK_DEFINE_F(BenchmarkFixture, EdgeLookup)(benchmark::State& state) {
    // Pre-create nodes and edges
    auto node_ids = create_test_nodes(1000);
    auto edge_ids = create_test_edges(node_ids, state.range(0));
    std::uniform_int_distribution<size_t> edge_dist(0, edge_ids.size() - 1);
    
    for (auto _ : state) {
        EdgeId edge_id = edge_ids[edge_dist(rng_)];
        auto result = graph_store_->get_edge(edge_id);
        if (!result.has_value()) {
            state.SkipWithError("Failed to get edge");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(BenchmarkFixture, EdgeLookup)->Range(1000, 100000);

// Property Index benchmarks
BENCHMARK_DEFINE_F(BenchmarkFixture, PropertyIndexCreation)(benchmark::State& state) {
    auto node_ids = create_test_nodes(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        NodeId node_id = node_ids[rng_() % node_ids.size()];
        std::string key = "prop_1";
        std::string value = "value_" + std::to_string(rng_() % 1000);
        state.ResumeTiming();

        index_manager_->index_node_property(node_id, key, value);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(BenchmarkFixture, PropertyIndexCreation)->Range(1000, 100000);

BENCHMARK_DEFINE_F(BenchmarkFixture, PropertyIndexLookup)(benchmark::State& state) {
    // Pre-create nodes and index their properties
    const size_t num_nodes = state.range(0);
    auto node_ids = create_test_nodes(num_nodes);
    for (NodeId node_id : node_ids) {
        index_manager_->index_node_property(node_id, "prop_1", "value_123");
    }

    for (auto _ : state) {
        auto result = index_manager_->find_nodes_by_property("prop_1", "value_123");
        if (result.empty()) {
            state.SkipWithError("Failed to find nodes by property");
        }
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(BenchmarkFixture, PropertyIndexLookup)->Range(1000, 100000);


// Adjacency list benchmarks
BENCHMARK_DEFINE_F(BenchmarkFixture, AdjacencyLookup)(benchmark::State& state) {
    // Pre-create nodes and edges
    auto node_ids = create_test_nodes(1000);
    auto edge_ids = create_test_edges(node_ids, 10000);
    std::uniform_int_distribution<size_t> node_dist(0, node_ids.size() - 1);
    
    for (auto _ : state) {
        NodeId node_id = node_ids[node_dist(rng_)];
        auto result = graph_store_->get_adjacent_nodes(node_id);
        if (!result.has_value()) {
            state.SkipWithError("Failed to get adjacent nodes");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(BenchmarkFixture, AdjacencyLookup)->Range(1000, 100000);

// Query benchmarks
BENCHMARK_DEFINE_F(BenchmarkFixture, SimpleQuery)(benchmark::State& state) {
    // Pre-create nodes
    auto node_ids = create_test_nodes(state.range(0));
    
    for (auto _ : state) {
        auto result = query_executor_->count_nodes();
        if (!result.has_value()) {
            state.SkipWithError("Failed to execute query");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(BenchmarkFixture, SimpleQuery)->Range(1000, 100000);

// Path finding benchmarks
BENCHMARK_DEFINE_F(BenchmarkFixture, PathFinding)(benchmark::State& state) {
    // Create a connected graph
    auto node_ids = create_test_nodes(1000);
    auto edge_ids = create_test_edges(node_ids, 5000);
    
    std::uniform_int_distribution<size_t> node_dist(0, node_ids.size() - 1);
    
    for (auto _ : state) {
        NodeId from_node = node_ids[node_dist(rng_)];
        NodeId to_node = node_ids[node_dist(rng_)];
        
        if (from_node != to_node) {
            auto result = query_executor_->find_shortest_path(from_node, to_node);
            if (!result.has_value()) {
                state.SkipWithError("Failed to find path");
            }
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(BenchmarkFixture, PathFinding)->Range(1000, 10000);

// Document-specific benchmarks
BENCHMARK_DEFINE_F(BenchmarkFixture, DocumentBacklinks)(benchmark::State& state) {
    // Create document nodes with links between them
    auto node_ids = create_test_nodes(1000);
    auto edge_ids = create_test_edges(node_ids, 5000);
    
    std::uniform_int_distribution<size_t> node_dist(0, node_ids.size() - 1);
    
    for (auto _ : state) {
        NodeId document_id = node_ids[node_dist(rng_)];
        auto result = query_executor_->get_document_backlinks(document_id);
        if (!result.has_value()) {
            state.SkipWithError("Failed to get document backlinks");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(BenchmarkFixture, DocumentBacklinks)->Range(1000, 10000);

// Comprehensive benchmark simulating real wiki workload
BENCHMARK_DEFINE_F(BenchmarkFixture, WikiWorkload)(benchmark::State& state) {
    // Create initial dataset
    const size_t num_documents = 10000;
    const size_t num_links = 50000;
    
    auto node_ids = create_test_nodes(num_documents);
    auto edge_ids = create_test_edges(node_ids, num_links);
    
    std::uniform_int_distribution<size_t> node_dist(0, node_ids.size() - 1);
    std::uniform_int_distribution<int> operation_dist(0, 99);
    
    for (auto _ : state) {
        int operation = operation_dist(rng_);
        
        if (operation < 80) {
            // 80% read operations
            NodeId node_id = node_ids[node_dist(rng_)];
            auto result = query_executor_->get_document_backlinks(node_id);
            if (!result.has_value()) {
                state.SkipWithError("Failed to get document backlinks");
            }
        } else if (operation < 95) {
            // 15% write operations (new links)
            NodeId from_node = node_ids[node_dist(rng_)];
            NodeId to_node = node_ids[node_dist(rng_)];
            
            if (from_node != to_node) {
                auto properties = generate_random_properties(2);
                auto result = graph_store_->create_edge(from_node, to_node, "links_to", properties);
                // Ignore errors for duplicates
            }
        } else {
            // 5% new document creation
            auto properties = generate_random_properties(5);
            auto result = graph_store_->create_node(properties);
            if (result.has_value()) {
                node_ids.push_back(result.value());
            }
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(BenchmarkFixture, WikiWorkload)->Iterations(100000);

// BENCHMARK_MAIN() removed - using benchmark::benchmark_main instead