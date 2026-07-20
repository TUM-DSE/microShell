#include <unordered_map>
#include <vector>
#include <optional>
#include <functional>
#include "worker_pool.hpp"

class VirtualWorkerManager {
public:
    // Take a non‑const reference because we need to assign tasks
    explicit VirtualWorkerManager(WorkerPool& pool) : pool_(pool) {}

    // Set (or change) which actual worker this virtual worker maps to.
    // Ensures one‑to‑one mapping (an actual can belong to at most one virtual).
    void map(int virtual_id, size_t actual_id) {
        // If actual is already used by another virtual, unmap that virtual first
        if (auto it = actual_to_virtual_.find(actual_id); it != actual_to_virtual_.end()) {
            int old_virtual = it->second;
            virtual_to_actual_.erase(old_virtual);
            actual_to_virtual_.erase(it);
        }
        // If this virtual already mapped, remove its old actual mapping
        if (auto it = virtual_to_actual_.find(virtual_id); it != virtual_to_actual_.end()) {
            size_t old_actual = it->second;
            actual_to_virtual_.erase(old_actual);
            it->second = actual_id;
        } else {
            virtual_to_actual_[virtual_id] = actual_id;
        }
        actual_to_virtual_[actual_id] = virtual_id;
    }

    // Remove mapping for a virtual worker
    void unmap(int virtual_id) {
        if (auto it = virtual_to_actual_.find(virtual_id); it != virtual_to_actual_.end()) {
            size_t actual = it->second;
            actual_to_virtual_.erase(actual);
            virtual_to_actual_.erase(it);
        }
    }

    bool is_mapped(int virtual_id) const {
        return virtual_to_actual_.count(virtual_id) > 0;
    }

    // Virtual worker is free if:
    // - Not mapped, OR
    // - Its mapped actual worker is not busy
    bool is_free(int virtual_id) const {
        auto it = virtual_to_actual_.find(virtual_id);
        if (it == virtual_to_actual_.end()) return true;   // unmapped → free
        return !pool_.is_busy(it->second);
    }

    bool is_busy(int virtual_id) const {
        return !is_free(virtual_id);
    }

    std::optional<size_t> current_actual(int virtual_id) const {
        auto it = virtual_to_actual_.find(virtual_id);
        if (it == virtual_to_actual_.end()) return std::nullopt;
        return it->second;
    }

    std::optional<int> current_virtual(size_t actual_id) const {
        auto it = actual_to_virtual_.find(actual_id);
        if (it == actual_to_virtual_.end()) return std::nullopt;
        return it->second;
    }

    // Remap a virtual worker to any free actual worker that is not already mapped
    bool remap_to_free(int virtual_id) {
        for (size_t i = 0; i < pool_.worker_count(); ++i) {
            if (!pool_.is_busy(i) && !actual_to_virtual_.count(i)) {
                map(virtual_id, i);
                return true;
            }
        }
        return false;
    }

    // Assign a task to a virtual worker.
    // If unmapped, tries to remap to a free actual worker first.
    bool assign_task(int virtual_id, std::function<void()> task) {
        if (!is_mapped(virtual_id)) {
            if (!remap_to_free(virtual_id))
                return false;   // no free actual worker available
        }
        size_t actual_id = virtual_to_actual_[virtual_id];
        return pool_.assign_task(actual_id, std::move(task));
    }

    // Get all free actual workers (regardless of mapping)
    std::vector<size_t> free_actuals() const {
        std::vector<size_t> result;
        for (size_t i = 0; i < pool_.worker_count(); ++i) {
            if (!pool_.is_busy(i)) result.push_back(i);
        }
        return result;
    }

    // Direct actual worker status queries
    bool is_actual_busy(size_t actual_id) const {
        return pool_.is_busy(actual_id);
    }

    bool is_actual_free(size_t actual_id) const {
        return !pool_.is_busy(actual_id);
    }

    // Convenience: initialise from a map actual_id -> (virtual_id+1)
    // Each virtual gets the first actual worker encountered.
    size_t init_from_actual_to_vplus1(const std::unordered_map<int, int>& actual_to_vplus1) {
        std::unordered_map<int, size_t> first_actual_for_virtual;
        for (const auto& [actual, v_plus_one] : actual_to_vplus1) {
            int virtual_id = v_plus_one - 1;
            if (first_actual_for_virtual.find(virtual_id) == first_actual_for_virtual.end()) {
                first_actual_for_virtual[virtual_id] = static_cast<size_t>(actual);
            }
        }
        for (const auto& [vid, actual] : first_actual_for_virtual) {
            map(vid, actual);
        }
        return first_actual_for_virtual.size();
    }

    // Alias for the same method, to match your code
    size_t map_from_actual_to_virtual(const std::unordered_map<int, int>& actual_to_vplus1) {
        return init_from_actual_to_vplus1(actual_to_vplus1);
    }

private:
    WorkerPool& pool_;   // non‑const because assign_task() modifies the pool
    std::unordered_map<int, size_t> virtual_to_actual_;
    std::unordered_map<size_t, int> actual_to_virtual_;
};