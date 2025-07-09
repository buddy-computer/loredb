#include "repl.h"
#include "../util/logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>

namespace graphdb::cli {

REPL::REPL(const std::string& db_path) : running_(true) {
    LOG_INFO("Initializing REPL with database: {}", db_path);
    
    page_store_ = std::make_unique<storage::FilePageStore>(db_path);
    graph_store_ = std::make_shared<storage::GraphStore>(std::move(page_store_));
    index_manager_ = std::make_shared<storage::SimpleIndexManager>();
    query_executor_ = std::make_unique<query::QueryExecutor>(graph_store_, index_manager_);
    
    LOG_INFO("REPL initialized successfully");
}

REPL::~REPL() = default;

void REPL::run() {
    print_banner();
    print_help();
    
    std::string line;
    while (running_ && std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        try {
            auto start_time = std::chrono::high_resolution_clock::now();
            process_command(line);
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            
            LOG_PERFORMANCE("command", duration.count() / 1000.0, "cmd: {}", line);
        } catch (const std::exception& e) {
            LOG_ERROR_DETAILED("REPL", "command_error", "command: {} - error: {}", line, e.what());
            std::cerr << "Error: " << e.what() << std::endl;
        }
        
        if (running_) {
            std::cout << "> ";
        }
    }
}

void REPL::print_banner() {
    std::cout << R"(
 _____ _____  ___  _____  _   _  ____  _____ 
|  __ |  __ \/ _ \|  __ \| | | |/ __ \|  __ \
| |  | | |__) | |_| | |  | | |_| | |  | |  | |
| |  | |  _  /|  _  | |  | |  _  | |  | |  | |
| |__| | | \ \| | | | |__| | | | | |__| | |__| |
|_____/|_|  \_\_| |_|_____/|_| |_|\____/|_____/ 
                                                
    Wiki Graph Database CLI
    )" << std::endl;
}

void REPL::print_help() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "  create-node <properties>       - Create a new node" << std::endl;
    std::cout << "  create-edge <from> <to> <label> <properties> - Create a new edge" << std::endl;
    std::cout << "  get-node <id>                  - Get node by ID" << std::endl;
    std::cout << "  get-edge <id>                  - Get edge by ID" << std::endl;
    std::cout << "  find-nodes <key> <value>       - Find nodes by property" << std::endl;
    std::cout << "  find-edges <key> <value>       - Find edges by property" << std::endl;
    std::cout << "  adjacent <id>                  - Get adjacent nodes" << std::endl;
    std::cout << "  path <from> <to>               - Find shortest path" << std::endl;
    std::cout << "  count                          - Count nodes and edges" << std::endl;
    std::cout << "  stats                          - Get database statistics" << std::endl;
    std::cout << "  backlinks <id>                 - Get document backlinks" << std::endl;
    std::cout << "  outlinks <id>                  - Get document outlinks" << std::endl;
    std::cout << "  related <id> [limit]           - Find related documents" << std::endl;
    std::cout << "  suggest <id> <content>         - Suggest links for document" << std::endl;
    std::cout << "  help                           - Show this help" << std::endl;
    std::cout << "  exit                           - Exit the program" << std::endl;
    std::cout << std::endl;
    std::cout << "Properties format: key1=value1,key2=value2" << std::endl;
    std::cout << "> ";
}

void REPL::process_command(const std::string& command) {
    auto tokens = tokenize(command);
    if (tokens.empty()) return;
    
    std::string cmd = tokens[0];
    std::string args;
    if (tokens.size() > 1) {
        args = command.substr(command.find(' ') + 1);
    }
    
    if (cmd == "create-node") {
        cmd_create_node(args);
    } else if (cmd == "create-edge") {
        cmd_create_edge(args);
    } else if (cmd == "get-node") {
        cmd_get_node(args);
    } else if (cmd == "get-edge") {
        cmd_get_edge(args);
    } else if (cmd == "find-nodes") {
        cmd_find_nodes(args);
    } else if (cmd == "find-edges") {
        cmd_find_edges(args);
    } else if (cmd == "adjacent") {
        cmd_adjacent(args);
    } else if (cmd == "path") {
        cmd_path(args);
    } else if (cmd == "count") {
        cmd_count(args);
    } else if (cmd == "stats") {
        cmd_stats(args);
    } else if (cmd == "backlinks") {
        cmd_backlinks(args);
    } else if (cmd == "outlinks") {
        cmd_outlinks(args);
    } else if (cmd == "related") {
        cmd_related(args);
    } else if (cmd == "suggest") {
        cmd_suggest(args);
    } else if (cmd == "help") {
        cmd_help(args);
    } else if (cmd == "exit" || cmd == "quit") {
        cmd_exit(args);
    } else {
        std::cout << "Unknown command: " << cmd << std::endl;
        std::cout << "Type 'help' for available commands." << std::endl;
    }
}

void REPL::cmd_create_node(const std::string& args) {
    if (args.empty()) {
        std::cout << "Usage: create-node <properties>" << std::endl;
        return;
    }
    
    auto properties = parse_properties(args);
    LOG_OPERATION("REPL", "create_node", "properties: {}", args);
    
    auto result = graph_store_->create_node(properties);
    
    if (result.has_value()) {
        storage::NodeId node_id = result.value();
        std::cout << "Created node with ID: " << node_id << std::endl;
        LOG_INFO("Successfully created node {} with {} properties", node_id, properties.size());
        
        // Index properties
        for (const auto& prop : properties) {
            if (std::holds_alternative<std::string>(prop.value)) {
                index_manager_->index_node_property(
                    node_id,
                    prop.key,
                    std::get<std::string>(prop.value)
                );
            }
        }
    } else {
        std::cout << "Failed to create node: " << result.error().message << std::endl;
        LOG_ERROR_DETAILED("REPL", "create_node_failed", "error: {}", result.error().message);
    }
}

void REPL::cmd_create_edge(const std::string& args) {
    auto tokens = tokenize(args);
    if (tokens.size() < 3) {
        std::cout << "Usage: create-edge <from> <to> <label> [properties]" << std::endl;
        return;
    }
    
    try {
        storage::NodeId from_node = std::stoull(tokens[0]);
        storage::NodeId to_node = std::stoull(tokens[1]);
        std::string label = tokens[2];
        
        std::vector<storage::Property> properties;
        if (tokens.size() > 3) {
            std::string props_str = args.substr(args.find(tokens[3]));
            properties = parse_properties(props_str);
        }
        
        auto result = graph_store_->create_edge(from_node, to_node, label, properties);
        
        if (result.has_value()) {
            storage::EdgeId edge_id = result.value();
            std::cout << "Created edge with ID: " << edge_id << std::endl;
            
            // Update adjacency lists
            index_manager_->add_edge_to_adjacency(from_node, to_node, edge_id);
            
            // Index properties
            for (const auto& prop : properties) {
                if (std::holds_alternative<std::string>(prop.value)) {
                    index_manager_->index_edge_property(
                        edge_id,
                        prop.key,
                        std::get<std::string>(prop.value)
                    );
                }
            }
        } else {
            std::cout << "Failed to create edge: " << result.error().message << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Invalid node ID format" << std::endl;
    }
}

void REPL::cmd_get_node(const std::string& args) {
    if (args.empty()) {
        std::cout << "Usage: get-node <id>" << std::endl;
        return;
    }
    
    try {
        storage::NodeId node_id = std::stoull(args);
        auto result = query_executor_->get_node_by_id(node_id);
        
        if (result.has_value()) {
            print_query_result(result.value());
        } else {
            std::cout << "Node not found: " << result.error().message << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Invalid node ID format" << std::endl;
    }
}

void REPL::cmd_get_edge(const std::string& args) {
    if (args.empty()) {
        std::cout << "Usage: get-edge <id>" << std::endl;
        return;
    }
    
    try {
        storage::EdgeId edge_id = std::stoull(args);
        auto result = query_executor_->get_edge_by_id(edge_id);
        
        if (result.has_value()) {
            print_query_result(result.value());
        } else {
            std::cout << "Edge not found: " << result.error().message << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Invalid edge ID format" << std::endl;
    }
}

void REPL::cmd_find_nodes(const std::string& args) {
    auto tokens = tokenize(args);
    if (tokens.size() < 2) {
        std::cout << "Usage: find-nodes <key> <value>" << std::endl;
        return;
    }
    
    std::string key = tokens[0];
    std::string value = tokens[1];
    
    auto result = query_executor_->get_nodes_by_property(key, value);
    
    if (result.has_value()) {
        print_query_result(result.value());
    } else {
        std::cout << "Failed to find nodes: " << result.error().message << std::endl;
    }
}

void REPL::cmd_find_edges(const std::string& args) {
    auto tokens = tokenize(args);
    if (tokens.size() < 2) {
        std::cout << "Usage: find-edges <key> <value>" << std::endl;
        return;
    }
    
    std::string key = tokens[0];
    std::string value = tokens[1];
    
    auto result = query_executor_->get_edges_by_property(key, value);
    
    if (result.has_value()) {
        print_query_result(result.value());
    } else {
        std::cout << "Failed to find edges: " << result.error().message << std::endl;
    }
}

void REPL::cmd_adjacent(const std::string& args) {
    if (args.empty()) {
        std::cout << "Usage: adjacent <id>" << std::endl;
        return;
    }
    
    try {
        storage::NodeId node_id = std::stoull(args);
        auto result = query_executor_->get_adjacent_nodes(node_id);
        
        if (result.has_value()) {
            print_query_result(result.value());
        } else {
            std::cout << "Failed to get adjacent nodes: " << result.error().message << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Invalid node ID format" << std::endl;
    }
}

void REPL::cmd_path(const std::string& args) {
    auto tokens = tokenize(args);
    if (tokens.size() < 2) {
        std::cout << "Usage: path <from> <to>" << std::endl;
        return;
    }
    
    try {
        storage::NodeId from_node = std::stoull(tokens[0]);
        storage::NodeId to_node = std::stoull(tokens[1]);
        
        auto result = query_executor_->find_shortest_path(from_node, to_node);
        
        if (result.has_value()) {
            print_query_result(result.value());
        } else {
            std::cout << "Failed to find path: " << result.error().message << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Invalid node ID format" << std::endl;
    }
}

void REPL::cmd_count(const std::string& args) {
    auto nodes_result = query_executor_->count_nodes();
    auto edges_result = query_executor_->count_edges();
    
    if (nodes_result.has_value() && edges_result.has_value()) {
        std::cout << "Nodes: " << nodes_result.value().rows[0][0] << std::endl;
        std::cout << "Edges: " << edges_result.value().rows[0][0] << std::endl;
    } else {
        std::cout << "Failed to get count statistics" << std::endl;
    }
}

void REPL::cmd_stats(const std::string& args) {
    auto result = query_executor_->get_node_degree_stats();
    
    if (result.has_value()) {
        print_query_result(result.value());
    } else {
        std::cout << "Failed to get statistics: " << result.error().message << std::endl;
    }
}

void REPL::cmd_backlinks(const std::string& args) {
    if (args.empty()) {
        std::cout << "Usage: backlinks <id>" << std::endl;
        return;
    }
    
    try {
        storage::NodeId node_id = std::stoull(args);
        auto result = query_executor_->get_document_backlinks(node_id);
        
        if (result.has_value()) {
            print_query_result(result.value());
        } else {
            std::cout << "Failed to get backlinks: " << result.error().message << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Invalid node ID format" << std::endl;
    }
}

void REPL::cmd_outlinks(const std::string& args) {
    if (args.empty()) {
        std::cout << "Usage: outlinks <id>" << std::endl;
        return;
    }
    
    try {
        storage::NodeId node_id = std::stoull(args);
        auto result = query_executor_->get_document_outlinks(node_id);
        
        if (result.has_value()) {
            print_query_result(result.value());
        } else {
            std::cout << "Failed to get outlinks: " << result.error().message << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Invalid node ID format" << std::endl;
    }
}

void REPL::cmd_related(const std::string& args) {
    auto tokens = tokenize(args);
    if (tokens.empty()) {
        std::cout << "Usage: related <id> [limit]" << std::endl;
        return;
    }
    
    try {
        storage::NodeId node_id = std::stoull(tokens[0]);
        size_t limit = 10;
        
        if (tokens.size() > 1) {
            limit = std::stoull(tokens[1]);
        }
        
        auto result = query_executor_->find_related_documents(node_id, limit);
        
        if (result.has_value()) {
            print_query_result(result.value());
        } else {
            std::cout << "Failed to find related documents: "
                      << result.error().message << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Invalid node ID format" << std::endl;
    }
}

void REPL::cmd_suggest(const std::string& args) {
    auto tokens = tokenize(args);
    if (tokens.size() < 2) {
        std::cout << "Usage: suggest <id> <content>" << std::endl;
        return;
    }
    
    try {
        storage::NodeId node_id = std::stoull(tokens[0]);
        std::string content = args.substr(args.find(tokens[1]));
        
        auto result = query_executor_->suggest_links_for_document(node_id, content);
        
        if (result.has_value()) {
            print_query_result(result.value());
        } else {
            std::cout << "Failed to suggest links: " << result.error().message << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Invalid node ID format" << std::endl;
    }
}

void REPL::cmd_help(const std::string& args) {
    print_help();
}

void REPL::cmd_exit(const std::string& args) {
    std::cout << "Goodbye!" << std::endl;
    running_ = false;
}

void REPL::print_query_result(const query::QueryResult& result) {
    if (result.empty()) {
        std::cout << "No results found." << std::endl;
        return;
    }
    
    // Calculate column widths
    std::vector<size_t> widths(result.columns.size());
    for (size_t i = 0; i < result.columns.size(); ++i) {
        widths[i] = result.columns[i].length();
        for (const auto& row : result.rows) {
            if (i < row.size()) {
                widths[i] = std::max(widths[i], row[i].length());
            }
        }
    }
    
    // Print header
    std::cout << "┌";
    for (size_t i = 0; i < widths.size(); ++i) {
        std::cout << std::string(widths[i] + 2, '-');
        if (i < widths.size() - 1) {
            std::cout << "┬";
        }
    }
    std::cout << "┐" << std::endl;
    
    std::cout << "│";
    for (size_t i = 0; i < result.columns.size(); ++i) {
        std::cout << " " << std::left << std::setw(widths[i]) << result.columns[i] << " ";
        if (i < result.columns.size() - 1) {
            std::cout << "│";
        }
    }
    std::cout << "│" << std::endl;
    
    // Print separator
    std::cout << "├";
    for (size_t i = 0; i < widths.size(); ++i) {
        std::cout << std::string(widths[i] + 2, '-');
        if (i < widths.size() - 1) {
            std::cout << "┼";
        }
    }
    std::cout << "┤" << std::endl;
    
    // Print rows
    for (const auto& row : result.rows) {
        std::cout << "│";
        for (size_t i = 0; i < result.columns.size(); ++i) {
            std::string value = (i < row.size()) ? row[i] : "";
            std::cout << " " << std::left << std::setw(widths[i]) << value << " ";
            if (i < result.columns.size() - 1) {
                std::cout << "│";
            }
        }
        std::cout << "│" << std::endl;
    }
    
    // Print bottom
    std::cout << "└";
    for (size_t i = 0; i < widths.size(); ++i) {
        std::cout << std::string(widths[i] + 2, '-');
        if (i < widths.size() - 1) {
            std::cout << "┴";
        }
    }
    std::cout << "┘" << std::endl;
    
    std::cout << "(" << result.rows.size() << " rows)" << std::endl;
}

std::vector<std::string> REPL::tokenize(const std::string& str) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::vector<storage::Property> REPL::parse_properties(const std::string& props_str) {
    std::vector<storage::Property> properties;
    
    if (props_str.empty()) {
        return properties;
    }
    
    std::istringstream iss(props_str);
    std::string prop;
    
    while (std::getline(iss, prop, ',')) {
        size_t eq_pos = prop.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = prop.substr(0, eq_pos);
            std::string value = prop.substr(eq_pos + 1);
            
            // Trim whitespace
            key.erase(key.begin(), std::find_if(key.begin(), key.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            key.erase(std::find_if(key.rbegin(), key.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), key.end());
            
            value.erase(
                value.begin(),
                std::find_if(
                    value.begin(),
                    value.end(),
                    [](unsigned char ch) { return !std::isspace(ch); }
                )
            );
            value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), value.end());
            
            properties.emplace_back(key, storage::PropertyValue{value});
        }
    }
    
    return properties;
}

}  // namespace graphdb::cli