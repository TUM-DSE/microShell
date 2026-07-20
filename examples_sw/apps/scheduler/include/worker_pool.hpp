#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <chrono>

class WorkerPool {
public:
    WorkerPool(size_t worker_count = 8) 
        : workers(worker_count),
          busy_flags(worker_count),
          completion_flags(worker_count),
          running(true) {
        
        // Initialize flags
        for (size_t i = 0; i < worker_count; ++i) {
            busy_flags[i] = false;
            completion_flags[i] = false;
        }
        
        // Create worker threads
        for (size_t i = 0; i < worker_count; ++i) {
            workers[i] = std::thread(&WorkerPool::worker_loop, this, i);
        }
    }

    ~WorkerPool() {
        // Signal shutdown
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            running = false;
        }
        condition.notify_all();
        
        // Join all threads
        for (auto& worker : workers) {
            if (worker.joinable()) worker.join();
        }
    }

    // Assign a new task to an available worker
    bool assign_task(size_t worker_id, std::function<void()> task) {
        if (worker_id >= workers.size()) return false;
        if (busy_flags[worker_id]) return false;  // Worker busy
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            // Reset completion flag for new task
            completion_flags[worker_id] = false;
            
            // Assign task directly to worker
            worker_tasks[worker_id] = task;
            busy_flags[worker_id] = true;
        }
        
        // Notify the specific worker
        condition.notify_all();
        return true;
    }

    // Check if a worker has completed their task
    bool is_completed(size_t worker_id) const {
        return completion_flags[worker_id];
    }

    // Check if a worker is busy
    bool is_busy(size_t worker_id) const {
        return busy_flags[worker_id];
    }

    // Get first available worker ID
    ssize_t get_available_worker() const {
        for (size_t i = 0; i < workers.size(); ++i) {
            if (!busy_flags[i]) return i;
        }
        return -1;  // All busy
    }

    size_t worker_count() const { return workers.size(); }


private:
    void worker_loop(size_t worker_id) {
        while (running) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                
                // Wait for task or shutdown
                condition.wait(lock, [&] {
                    return !running || worker_tasks.count(worker_id);
                });
                
                if (!running) return;
                
                // Get assigned task
                task = worker_tasks[worker_id];
                worker_tasks.erase(worker_id);
            }
            
            // Execute the task
            task();
            
            // Update status flags
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                busy_flags[worker_id] = false;
                completion_flags[worker_id] = true;
            }
        }
    }

    // Worker threads
    std::vector<std::thread> workers;
    
    // Task storage
    std::unordered_map<size_t, std::function<void()>> worker_tasks;
    
    // Status flags
    std::vector<std::atomic<bool>> busy_flags;
    std::vector<std::atomic<bool>> completion_flags;
    
    // Synchronization
    mutable std::mutex queue_mutex;
    std::condition_variable condition;
    bool running;
};

// // Usage example
// int main() {
//     WorkerPool pool(8);

//     // Task generator
//     auto create_task = [](int id) {
//         return [id] {
//             std::cout << "Worker " << id << " starting task\n";
//             std::this_thread::sleep_for(std::chrono::milliseconds(500));
//             std::cout << "Worker " << id << " completed task\n";
//         };
//     };

//     // Assign initial tasks
//     for (int i = 0; i < 8; ++i) {
//         pool.assign_task(i, create_task(i));
//     }

//     // Main loop
//     while (true) {
//         // Check for completed workers
//         bool all_completed = true;
//         for (int i = 0; i < 8; ++i) {
//             if (pool.is_busy(i) && !pool.is_completed(i)) {
//                 all_completed = false;
//             }
//         }
//         if (all_completed) break;
        
//         // Find available workers for new tasks
//         for (int i = 0; i < 8; ++i) {
//             if (pool.is_completed(i)) {
//                 std::cout << "Worker " << i << " is free, assigning new task\n";
//                 pool.assign_task(i, create_task(i + 10));
//             }
//         }
        
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }

//     std::cout << "All tasks completed!\n";
//     return 0;
// }