#include "repl.h"
#include "../util/logger.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    loredb::util::Logger::init();
    
    if (argc > 1 && (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v")) {
        std::cout << "loredb version 0.1.0" << std::endl;
        return 0;
    }

    std::string db_path = "loredb.db";
    if (argc > 1) {
        db_path = argv[1];
    }
    
    LOG_INFO("Starting loredb CLI - database: {}", db_path);
    
    loredb::cli::REPL repl(db_path);
    repl.run();
    
    return 0;
}