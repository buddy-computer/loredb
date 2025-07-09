#include <benchmark/benchmark.h>
#include "../src/query/executor.h"
#include "../src/storage/file_page_store.h"
#include "../src/storage/graph_store.h"
#include "../src/storage/simple_index_manager.h"
#include <random>
#include <string>
#include <vector>
#include <unistd.h>

using namespace loredb::storage;
using namespace loredb::query;

class QueryBenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        // Create temporary file for testing
        std::string db_path = "/tmp/loredb_query_benchmark_" + std::to_string(getpid()) + ".db";
        
        auto page_store = std::make_unique<FilePageStore>(db_path);
        graph_store_ = std::make_shared<GraphStore>(std::move(page_store));
        index_manager_ = std::make_shared<SimpleIndexManager>();
        query_executor_ = std::make_unique<QueryExecutor>(graph_store_, index_manager_);
        
        // Initialize random number generator
        rng_.seed(42);
        
        // Create test dataset
        setup_test_data();
    }
    
    void TearDown(const benchmark::State& state) override {
        query_executor_.reset();
        index_manager_.reset();
        graph_store_.reset();
        
        // Clean up temporary file
        std::string db_path = "/tmp/loredb_query_benchmark_" + std::to_string(getpid()) + ".db";
        unlink(db_path.c_str());
    }
    
    void setup_test_data() {
        // Create documents with various properties
        for (size_t i = 0; i < 1000; ++i) {
            std::vector<Property> properties;
            properties.emplace_back("title", PropertyValue{std::string("Document_") + std::to_string(i)});
            properties.emplace_back("type", PropertyValue{std::string("document")});
            properties.emplace_back("category", PropertyValue{std::string("Category_") + std::to_string(i % 10)});
            properties.emplace_back("author", PropertyValue{std::string("Author_") + std::to_string(i % 50)});
            properties.emplace_back("content_length", PropertyValue{static_cast<int64_t>(rng_() % 10000)});
            
            auto result = graph_store_->create_node(properties);
            if (result.has_value()) {
                document_ids_.push_back(result.value());
                
                // Index properties
                for (const auto& prop : properties) {
                    if (std::holds_alternative<std::string>(prop.value)) {
                        index_manager_->index_node_property(result.value(), prop.key, 
                                                          std::get<std::string>(prop.value));
                    }
                }
            }
        }
        
        // Create links between documents
        std::uniform_int_distribution<size_t> node_dist(0, document_ids_.size() - 1);
        for (size_t i = 0; i < 5000; ++i) {
            NodeId from_node = document_ids_[node_dist(rng_)];
            NodeId to_node = document_ids_[node_dist(rng_)];
            
            if (from_node != to_node) {
                std::vector<Property> properties;
                properties.emplace_back("type", PropertyValue{std::string("link")});
                properties.emplace_back("context", PropertyValue{std::string("Context_") + std::to_string(i)});
                
                auto result = graph_store_->create_edge(from_node, to_node, "links_to", properties);
                if (result.has_value()) {
                    link_ids_.push_back(result.value());
                    
                    // Update adjacency lists
                    index_manager_->add_edge_to_adjacency(from_node, to_node, result.value());
                    
                    // Index edge properties
                    for (const auto& prop : properties) {
                        if (std::holds_alternative<std::string>(prop.value)) {
                            index_manager_->index_edge_property(result.value(), prop.key, 
                                                              std::get<std::string>(prop.value));
                        }
                    }
                }
            }
        }
    }

protected:
    std::shared_ptr<GraphStore> graph_store_;
    std::shared_ptr<SimpleIndexManager> index_manager_;
    std::unique_ptr<QueryExecutor> query_executor_;
    std::mt19937 rng_;
    std::vector<NodeId> document_ids_;
    std::vector<EdgeId> link_ids_;
};

// Basic query benchmarks
BENCHMARK_DEFINE_F(QueryBenchmarkFixture, GetNodeById)(benchmark::State& state) {
    std::uniform_int_distribution<size_t> node_dist(0, document_ids_.size() - 1);
    
    for (auto _ : state) {
        NodeId node_id = document_ids_[node_dist(rng_)];
        auto result = query_executor_->get_node_by_id(node_id);
        if (!result.has_value()) {
            state.SkipWithError("Failed to get node by ID");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, GetNodeById);

BENCHMARK_DEFINE_F(QueryBenchmarkFixture, GetEdgeById)(benchmark::State& state) {
    std::uniform_int_distribution<size_t> edge_dist(0, link_ids_.size() - 1);
    
    for (auto _ : state) {
        EdgeId edge_id = link_ids_[edge_dist(rng_)];
        auto result = query_executor_->get_edge_by_id(edge_id);
        if (!result.has_value()) {
            state.SkipWithError("Failed to get edge by ID");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, GetEdgeById);

// Property-based query benchmarks
BENCHMARK_DEFINE_F(QueryBenchmarkFixture, GetNodesByProperty)(benchmark::State& state) {
    std::uniform_int_distribution<int> category_dist(0, 9);
    
    for (auto _ : state) {
        std::string category = "Category_" + std::to_string(category_dist(rng_));
        auto result = query_executor_->get_nodes_by_property("category", category);
        if (!result.has_value()) {
            state.SkipWithError("Failed to get nodes by property");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, GetNodesByProperty);

BENCHMARK_DEFINE_F(QueryBenchmarkFixture, GetEdgesByProperty)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = query_executor_->get_edges_by_property("type", "link");
        if (!result.has_value()) {
            state.SkipWithError("Failed to get edges by property");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, GetEdgesByProperty);

// Graph traversal benchmarks
BENCHMARK_DEFINE_F(QueryBenchmarkFixture, GetAdjacentNodes)(benchmark::State& state) {
    std::uniform_int_distribution<size_t> node_dist(0, document_ids_.size() - 1);
    
    for (auto _ : state) {
        NodeId node_id = document_ids_[node_dist(rng_)];
        auto result = query_executor_->get_adjacent_nodes(node_id);
        if (!result.has_value()) {
            state.SkipWithError("Failed to get adjacent nodes");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, GetAdjacentNodes);

BENCHMARK_DEFINE_F(QueryBenchmarkFixture, GetOutgoingEdges)(benchmark::State& state) {
    std::uniform_int_distribution<size_t> node_dist(0, document_ids_.size() - 1);
    
    for (auto _ : state) {
        NodeId node_id = document_ids_[node_dist(rng_)];
        auto result = query_executor_->get_outgoing_edges(node_id);
        if (!result.has_value()) {
            state.SkipWithError("Failed to get outgoing edges");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, GetOutgoingEdges);

BENCHMARK_DEFINE_F(QueryBenchmarkFixture, GetIncomingEdges)(benchmark::State& state) {
    std::uniform_int_distribution<size_t> node_dist(0, document_ids_.size() - 1);
    
    for (auto _ : state) {
        NodeId node_id = document_ids_[node_dist(rng_)];
        auto result = query_executor_->get_incoming_edges(node_id);
        if (!result.has_value()) {
            state.SkipWithError("Failed to get incoming edges");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, GetIncomingEdges);

// Path finding benchmarks
BENCHMARK_DEFINE_F(QueryBenchmarkFixture, FindShortestPath)(benchmark::State& state) {
    std::uniform_int_distribution<size_t> node_dist(0, document_ids_.size() - 1);
    
    for (auto _ : state) {
        NodeId from_node = document_ids_[node_dist(rng_)];
        NodeId to_node = document_ids_[node_dist(rng_)];
        
        if (from_node != to_node) {
            auto result = query_executor_->find_shortest_path(from_node, to_node);
            if (!result.has_value()) {
                state.SkipWithError("Failed to find shortest path");
            }
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, FindShortestPath);

BENCHMARK_DEFINE_F(QueryBenchmarkFixture, FindPathsWithLength)(benchmark::State& state) {
    std::uniform_int_distribution<size_t> node_dist(0, document_ids_.size() - 1);
    
    for (auto _ : state) {
        NodeId from_node = document_ids_[node_dist(rng_)];
        NodeId to_node = document_ids_[node_dist(rng_)];
        
        if (from_node != to_node) {
            auto result = query_executor_->find_paths_with_length(from_node, to_node, 3);
            if (!result.has_value()) {
                state.SkipWithError("Failed to find paths with length");
            }
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, FindPathsWithLength);

// Document-specific benchmarks
BENCHMARK_DEFINE_F(QueryBenchmarkFixture, GetDocumentBacklinks)(benchmark::State& state) {
    std::uniform_int_distribution<size_t> node_dist(0, document_ids_.size() - 1);
    
    for (auto _ : state) {
        NodeId document_id = document_ids_[node_dist(rng_)];
        auto result = query_executor_->get_document_backlinks(document_id);
        if (!result.has_value()) {
            state.SkipWithError("Failed to get document backlinks");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, GetDocumentBacklinks);

BENCHMARK_DEFINE_F(QueryBenchmarkFixture, GetDocumentOutlinks)(benchmark::State& state) {
    std::uniform_int_distribution<size_t> node_dist(0, document_ids_.size() - 1);
    
    for (auto _ : state) {
        NodeId document_id = document_ids_[node_dist(rng_)];
        auto result = query_executor_->get_document_outlinks(document_id);
        if (!result.has_value()) {
            state.SkipWithError("Failed to get document outlinks");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, GetDocumentOutlinks);

BENCHMARK_DEFINE_F(QueryBenchmarkFixture, FindRelatedDocuments)(benchmark::State& state) {
    std::uniform_int_distribution<size_t> node_dist(0, document_ids_.size() - 1);
    
    for (auto _ : state) {
        NodeId document_id = document_ids_[node_dist(rng_)];
        auto result = query_executor_->find_related_documents(document_id, 10);
        if (!result.has_value()) {
            state.SkipWithError("Failed to find related documents");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, FindRelatedDocuments);

BENCHMARK_DEFINE_F(QueryBenchmarkFixture, SuggestLinksForDocument)(benchmark::State& state) {
    std::uniform_int_distribution<size_t> node_dist(0, document_ids_.size() - 1);
    
    for (auto _ : state) {
        NodeId document_id = document_ids_[node_dist(rng_)];
        auto result = query_executor_->suggest_links_for_document(document_id, "sample content");
        if (!result.has_value()) {
            state.SkipWithError("Failed to suggest links for document");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, SuggestLinksForDocument);

// Batch operation benchmarks
BENCHMARK_DEFINE_F(QueryBenchmarkFixture, BatchGetNodes)(benchmark::State& state) {
    std::uniform_int_distribution<size_t> node_dist(0, document_ids_.size() - 1);
    
    for (auto _ : state) {
        std::vector<NodeId> batch_ids;
        for (size_t i = 0; i < 10; ++i) {
            batch_ids.push_back(document_ids_[node_dist(rng_)]);
        }
        
        auto result = query_executor_->batch_get_nodes(batch_ids);
        if (!result.has_value()) {
            state.SkipWithError("Failed to batch get nodes");
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 10);
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, BatchGetNodes);

BENCHMARK_DEFINE_F(QueryBenchmarkFixture, BatchGetEdges)(benchmark::State& state) {
    std::uniform_int_distribution<size_t> edge_dist(0, link_ids_.size() - 1);
    
    for (auto _ : state) {
        std::vector<EdgeId> batch_ids;
        for (size_t i = 0; i < 10; ++i) {
            batch_ids.push_back(link_ids_[edge_dist(rng_)]);
        }
        
        auto result = query_executor_->batch_get_edges(batch_ids);
        if (!result.has_value()) {
            state.SkipWithError("Failed to batch get edges");
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 10);
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, BatchGetEdges);

// Streaming query benchmarks removed for C++20 compatibility
// TODO: Re-implement with proper coroutine support

// Aggregate benchmarks
BENCHMARK_DEFINE_F(QueryBenchmarkFixture, CountNodes)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = query_executor_->count_nodes();
        if (!result.has_value()) {
            state.SkipWithError("Failed to count nodes");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, CountNodes);

BENCHMARK_DEFINE_F(QueryBenchmarkFixture, CountEdges)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = query_executor_->count_edges();
        if (!result.has_value()) {
            state.SkipWithError("Failed to count edges");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, CountEdges);

BENCHMARK_DEFINE_F(QueryBenchmarkFixture, GetNodeDegreeStats)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = query_executor_->get_node_degree_stats();
        if (!result.has_value()) {
            state.SkipWithError("Failed to get node degree stats");
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(QueryBenchmarkFixture, GetNodeDegreeStats);

// BENCHMARK_MAIN() removed - using benchmark::benchmark_main instead