#include "repl.h"
#include "../util/logger.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string db_path = "graphdb.db";
    
    // Parse command line arguments
    if (argc > 1) {
        db_path = argv[1];
    }
    
    try {
        // Initialize logger
        graphdb::util::Logger::init("info", "graphdb-cli.log");
        LOG_INFO("Starting GraphDB CLI - database: {}", db_path);
        
        graphdb::cli::REPL repl(db_path);
        repl.run();
        
        LOG_INFO("GraphDB CLI shutting down");
    } catch (const std::exception& e) {
        LOG_ERROR("Fatal error: {}", e.what());
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}