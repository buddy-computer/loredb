#include "lock_manager.h"
#include <algorithm>

namespace loredb::transaction {

bool LockManager::lock(TransactionId tx_id, ResourceId resource_id, LockMode mode) {
    std::unique_lock<std::mutex> lock(mutex_);

    auto& requests = lock_table_[resource_id];

    // Check for conflicts
    auto has_conflict = [&] {
        return std::any_of(requests.begin(), requests.end(), [&](const auto& req) {
            if (req->tx_id == tx_id) return false; // Same transaction
            if (mode == LockMode::EXCLUSIVE || req->mode == LockMode::EXCLUSIVE) {
                return true;
            }
            return false;
        });
    };
    
    while(has_conflict()) {
        // Build waits-for graph
        waits_for_graph_[tx_id].clear();
        for(const auto& req : requests) {
            if (req->tx_id != tx_id && (mode == LockMode::EXCLUSIVE || req->mode == LockMode::EXCLUSIVE)) {
                 waits_for_graph_[tx_id].push_back(req->tx_id);
            }
        }
        
        if (detect_deadlock(tx_id)) {
            waits_for_graph_.erase(tx_id);
            return false; // Deadlock
        }
        cv_.wait(lock);
    }

    // Grant lock
    requests.push_back(std::make_shared<LockRequest>(LockRequest{tx_id, mode, true}));
    waits_for_graph_.erase(tx_id);
    return true;
}

void LockManager::unlock(TransactionId tx_id, ResourceId resource_id) {
    std::unique_lock<std::mutex> lock(mutex_);

    auto it = lock_table_.find(resource_id);
    if (it != lock_table_.end()) {
        auto& requests = it->second;
        requests.erase(std::remove_if(requests.begin(), requests.end(),
                                      [tx_id](const auto& req) { return req->tx_id == tx_id; }),
                       requests.end());

        cv_.notify_all();
    }
}

void LockManager::unlock_all(TransactionId tx_id) {
    std::unique_lock<std::mutex> lock(mutex_);

    for (auto& pair : lock_table_) {
        auto& requests = pair.second;
        requests.erase(std::remove_if(requests.begin(), requests.end(),
                                      [tx_id](const auto& req) { return req->tx_id == tx_id; }),
                       requests.end());
    }

    waits_for_graph_.erase(tx_id);

    cv_.notify_all();
}

bool LockManager::detect_deadlock(TransactionId start_tx) {
    std::unordered_map<TransactionId, bool> visited;
    std::unordered_map<TransactionId, bool> recursion_stack;

    for (const auto& pair : waits_for_graph_) {
        visited[pair.first] = false;
        recursion_stack[pair.first] = false;
    }

    return has_cycle(start_tx, visited, recursion_stack);
}

bool LockManager::has_cycle(TransactionId u, std::unordered_map<TransactionId, bool>& visited, std::unordered_map<TransactionId, bool>& recursion_stack) {
    if(visited.find(u) == visited.end()) return false; // Not in graph

    visited[u] = true;
    recursion_stack[u] = true;

    for (TransactionId v : waits_for_graph_[u]) {
        if (!visited[v]) {
            if (has_cycle(v, visited, recursion_stack)) {
                return true;
            }
        } else if (recursion_stack[v]) {
            return true; // Cycle detected
        }
    }

    recursion_stack[u] = false;
    return false;
}

} // namespace loredb::transaction 