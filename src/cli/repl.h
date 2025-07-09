/// \file repl.h
/// \brief Command-line REPL for interactive graph database usage.
/// \author wiki-graph contributors
/// \ingroup cli
#pragma once

#include "../storage/file_page_store.h"
#include "../storage/graph_store.h"
#include "../storage/simple_index_manager.h"
#include "../query/executor.h"
#include <memory>
#include <string>

namespace graphdb::cli {

/**
 * @class REPL
 * @brief Interactive command-line interface for the graph database.
 *
 * Provides command parsing, help, and dispatch for node/edge operations, queries, and graph analytics.
 */
class REPL {
public:
    /**
     * @brief Construct a REPL for the given database path.
     * @param db_path Path to the database file.
     */
    explicit REPL(const std::string& db_path);
    /** Destructor. */
    ~REPL();
    
    void run();
    
private:
    void print_banner();
    void print_help();
    void process_command(const std::string& command);
    
    // Command handlers
    void cmd_create_node(const std::string& args);
    void cmd_create_edge(const std::string& args);
    void cmd_get_node(const std::string& args);
    void cmd_get_edge(const std::string& args);
    void cmd_find_nodes(const std::string& args);
    void cmd_find_edges(const std::string& args);
    void cmd_adjacent(const std::string& args);
    void cmd_path(const std::string& args);
    void cmd_count(const std::string& args);
    void cmd_stats(const std::string& args);
    void cmd_backlinks(const std::string& args);
    void cmd_outlinks(const std::string& args);
    void cmd_related(const std::string& args);
    void cmd_suggest(const std::string& args);
    void cmd_help(const std::string& args);
    void cmd_exit(const std::string& args);
    
    // Helper functions
    void print_query_result(const query::QueryResult& result);
    std::vector<std::string> tokenize(const std::string& str);
    std::vector<storage::Property> parse_properties(const std::string& props_str);
    
    std::unique_ptr<storage::FilePageStore> page_store_;
    std::shared_ptr<storage::GraphStore> graph_store_;
    std::shared_ptr<storage::SimpleIndexManager> index_manager_;
    std::unique_ptr<query::QueryExecutor> query_executor_;
    
    bool running_;
};

}  // namespace graphdb::cli