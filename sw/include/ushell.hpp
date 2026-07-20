#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <atomic>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include "dfg.hpp"

namespace ushell {

using namespace fpga;
using namespace std;

// Forward declarations
class Task;
class Buffer;
class Dataflow;

// ================================================================
// Task Class - Wraps dfg::Node with simple interface
// ================================================================
class Task {
private:
    std::string name;
    std::string type;
    dfg::Node* internal_node;
    dfg::Capability* node_capability;
    Dataflow* parent_dataflow;
    int vfid;
    
public:
    // Port objects for fluent connections
    struct Port {
        Task* parent;
        std::string port_name;
        Port(Task* p, const std::string& name) : parent(p), port_name(name) {}
        
        // Get the parent task name for connections
        std::string get_task_name() const { return parent ? parent->get_name() : ""; }
    };
    
    Port in;  // Input port
    Port out; // Output port
    
    Task(const std::string& task_name, const std::string& task_type = "processing", Dataflow* parent = nullptr);
    ~Task() = default; // Managed by Dataflow
    
    // Getters
    std::string get_name() const { return name; }
    std::string get_type() const { return type; }
    dfg::Node* get_internal_node() const { return internal_node; }
    dfg::Capability* get_capability() const { return node_capability; }
    int get_vfid() const { return vfid; }
    
    // Internal setup (called by Dataflow)
    void set_internal_node(dfg::Node* node) { internal_node = node; }
    void set_capability(dfg::Capability* cap) { node_capability = cap; }
    void set_vfid(int id) { vfid = id; }
};

// ================================================================
// Buffer Class - Wraps host memory with dfg::Buffer
// ================================================================
class Buffer {
private:
    void* host_ptr;
    size_t size;
    bool owns_memory;
    dfg::Buffer* internal_buffer;
    dfg::Capability* buffer_capability;
    std::string buffer_name;
    Dataflow* parent_dataflow;
    
public:
    // Constructor for host memory (auto-detect size for arrays)
    Buffer(void* host_memory, size_t buffer_size = 0, const std::string& name = "", Dataflow* parent = nullptr);
    
    // Constructor for auto-allocated memory
    Buffer(size_t buffer_size, const std::string& name = "", Dataflow* parent = nullptr);
    
    ~Buffer();
    
    // Getters
    void* get_host_ptr() const { return host_ptr; }
    size_t get_size() const { return size; }
    dfg::Buffer* get_internal_buffer() const { return internal_buffer; }
    dfg::Capability* get_capability() const { return buffer_capability; }
    std::string get_name() const { return buffer_name; }
    
    // Data operations using dfg factory functions
    bool write_data(void* data, size_t data_size);
    bool read_data(void* dest, size_t data_size);
    
    // New methods for proper dfg integration
    bool write_from_host(void* data, size_t data_size);
    bool read_to_host(void* dest, size_t data_size);
    
    // Internal setup (called by Dataflow)
    void set_internal_buffer(dfg::Buffer* buffer) { internal_buffer = buffer; }
    void set_capability(dfg::Capability* cap) { buffer_capability = cap; }
    void set_name(const std::string& name) { buffer_name = name; }
};

// ================================================================
// Dataflow Class - Orchestrates using dfg::DFG internally
// ================================================================
class Dataflow {
private:
    struct Connection {
        std::string src_name;
        std::string dst_name;
        std::string src_port;
        std::string dst_port;
        std::string src_type; // "task" or "buffer"
        std::string dst_type; // "task" or "buffer"
    };
    
    std::string dataflow_name;
    std::vector<Connection> connections;
    std::vector<std::unique_ptr<Task>> tasks;
    std::vector<std::unique_ptr<Buffer>> buffers;
    std::unordered_map<std::string, Task*> task_map;
    std::unordered_map<std::string, Buffer*> buffer_map;
    
    // Internal dfg implementation (copied from pipeline.hpp)
    dfg::DFG* dfg;
    std::vector<dfg::Node*> nodes;
    std::unordered_map<std::string, dfg::Node*> node_map;
    std::unordered_map<std::string, dfg::Buffer*> raw_buffers;
    std::vector<std::string> flow;
    
    // Capability management (copied from pipeline.hpp)
    dfg::Capability* dfg_root_capability;
    std::unordered_map<std::string, dfg::Capability*> node_capabilities;
    std::unordered_map<std::string, dfg::Capability*> buffer_capabilities;
    std::unordered_map<std::string, dfg::Capability*> connection_capabilities;
    
    // State management
    bool is_built;
    bool is_checked;
    int debug_level = 1; // 0=minimal, 1=normal, 2=verbose
    
    // Internal build methods (adapted from pipeline.hpp)
    void build_internal_dfg();
    void setup_capabilities();
    void create_internal_nodes();
    void create_internal_buffers();
    void setup_connections();
    void configure_io_switches();
    void setup_operations();
    
    // Connection validation
    bool validate_connections();
    std::vector<std::string> build_execution_flow();
    
    // Auto-sizing and memory management
    size_t detect_buffer_size(void* ptr);
    std::string generate_unique_name(const std::string& prefix);
    
    // Static counters for auto-generated names
    static std::atomic<int> task_counter;
    static std::atomic<int> buffer_counter;
    static std::atomic<int> dataflow_counter;
    
public:
    Dataflow(const std::string& name = "");
    ~Dataflow();
    
    // Factory methods for creating components
    Task& add_task(const std::string& name, const std::string& type = "processing");
    Buffer& add_buffer(void* host_memory, size_t size = 0, const std::string& name = "");
    Buffer& add_buffer(size_t size, const std::string& name = "");
    
    // Fluent connection interface
    Dataflow& to(Buffer& src, Task::Port& dst);
    Dataflow& to(Task::Port& src, Task::Port& dst);
    Dataflow& to(Task::Port& src, Buffer& dst);
    
    // Validation and execution
    bool check();
    void execute(size_t current_size = 0);
    
    // Resource management
    void release();
    void clear_completed();
    
    // Configuration
    void set_debug_level(int level) { debug_level = level; }
    int get_debug_level() const { return debug_level; }
    
    // Access methods
    Task* get_task(const std::string& name);
    Buffer* get_buffer(const std::string& name);
    dfg::DFG* get_dfg() const { return dfg; }
    dfg::Capability* get_root_capability() const { return dfg_root_capability; }
    
    // Advanced capability operations (copied from pipeline.hpp)
    dfg::Capability* create_capability(const std::string& component_name, 
                                     const std::string& cap_space_id,
                                     uint32_t access,
                                     bool allow_transitive_delegation = false);
    dfg::Capability* delegate_capability(const std::string& src_component, 
                                       const std::string& cap_space_id,
                                       uint32_t access,
                                       bool allow_transitive_delegation = false);
    bool can_component_delegate_delegation(const std::string& component_name) const;
    bool transfer_ownership(const std::string& src_component, const std::string& dst_component);
    bool revoke_capability(const std::string& component_name, const std::string& cap_space_id);
    bool expire_capability(const std::string& component_name, const std::string& cap_space_id, uint32_t timeout);
    bool reset_capabilities(const std::string& component_name);
    void print_capability_tree() const;
    
    // Debug and introspection
    void print_graph() const;
    void print_connections() const;
};

// Initialize static counters
std::atomic<int> Dataflow::task_counter{0};
std::atomic<int> Dataflow::buffer_counter{0};
std::atomic<int> Dataflow::dataflow_counter{0};

// ================================================================
// Task Implementation
// ================================================================
Task::Task(const std::string& task_name, const std::string& task_type, Dataflow* parent)
    : name(task_name), type(task_type), internal_node(nullptr), node_capability(nullptr),
      parent_dataflow(parent), vfid(0), in(this, "input"), out(this, "output") {
}

// ================================================================
// Buffer Implementation
// ================================================================
Buffer::Buffer(void* host_memory, size_t buffer_size, const std::string& name, Dataflow* parent)
    : host_ptr(host_memory), size(buffer_size), owns_memory(false), internal_buffer(nullptr),
      buffer_capability(nullptr), buffer_name(name), parent_dataflow(parent) {
    
    // Auto-detect size if not provided (simplified - in real implementation would need more sophisticated detection)
    if (size == 0 && host_ptr) {
        size = 65536; // Default size - in practice would use template specialization for arrays
    }
}

Buffer::Buffer(size_t buffer_size, const std::string& name, Dataflow* parent)
    : size(buffer_size), owns_memory(true), internal_buffer(nullptr),
      buffer_capability(nullptr), buffer_name(name), parent_dataflow(parent) {
    
    // Allocate memory
    host_ptr = malloc(size);
    if (!host_ptr) {
        throw std::runtime_error("Failed to allocate memory for buffer");
    }
    memset(host_ptr, 0, size); // Initialize to zero
}

Buffer::~Buffer() {
    if (owns_memory && host_ptr) {
        free(host_ptr);
    }
}

bool Buffer::write_data(void* data, size_t data_size) {
    if (!data || data_size == 0 || data_size > size) {
        return false;
    }
    
    // Copy to host memory
    memcpy(host_ptr, data, data_size);
    
    // If we have an internal buffer, also write there using dfg factory function
    if (internal_buffer) {
        dfg::write_buffer(internal_buffer, data, data_size);
    }
    
    return true;
}

bool Buffer::read_data(void* dest, size_t data_size) {
    if (!dest || data_size == 0 || data_size > size) {
        return false;
    }
    
    // If we have an internal buffer, read from there first using dfg factory function
    if (internal_buffer) {
        void* device_memory = dfg::read_buffer(internal_buffer);
        if (device_memory) {
            memcpy(dest, device_memory, data_size);
            // Also update host memory
            memcpy(host_ptr, device_memory, data_size);
            return true;
        }
    }
    
    // Fallback to host memory
    memcpy(dest, host_ptr, data_size);
    return true;
}

bool Buffer::write_from_host(void* data, size_t data_size) {
    if (!data || data_size == 0 || data_size > size) {
        return false;
    }
    
    // Copy to host memory first
    memcpy(host_ptr, data, data_size);
    
    // Use dfg factory function for device transfer
    if (internal_buffer) {
        dfg::write_buffer(internal_buffer, data, data_size);
        return true;
    }
    
    return false;
}

bool Buffer::read_to_host(void* dest, size_t data_size) {
    if (!dest || data_size == 0 || data_size > size) {
        return false;
    }
    
    // Use dfg factory function for device read
    if (internal_buffer) {
        void* device_memory = dfg::read_buffer(internal_buffer);
        if (device_memory) {
            memcpy(dest, device_memory, data_size);
            // Also update host memory
            memcpy(host_ptr, device_memory, data_size);
            return true;
        }
    }
    
    // Fallback to host memory
    memcpy(dest, host_ptr, data_size);
    return true;
}

// ================================================================
// Dataflow Implementation
// ================================================================
Dataflow::Dataflow(const std::string& name)
    : dataflow_name(name.empty() ? "dataflow_" + std::to_string(dataflow_counter++) : name),
      dfg(nullptr), dfg_root_capability(nullptr), is_built(false), is_checked(false) {
}

Dataflow::~Dataflow() {
    release();
}

Task& Dataflow::add_task(const std::string& name, const std::string& type) {
    std::string task_name = name.empty() ? "task_" + std::to_string(task_counter++) : name;
    
    // Check for duplicates
    if (task_map.find(task_name) != task_map.end()) {
        throw std::runtime_error("Task '" + task_name + "' already exists");
    }
    
    auto task = std::make_unique<Task>(task_name, type, this);
    Task* task_ptr = task.get();
    task_map[task_name] = task_ptr;
    tasks.push_back(std::move(task));
    
    return *task_ptr;
}

Buffer& Dataflow::add_buffer(void* host_memory, size_t size, const std::string& name) {
    std::string buffer_name = name.empty() ? "buffer_" + std::to_string(buffer_counter++) : name;
    
    // Check for duplicates
    if (buffer_map.find(buffer_name) != buffer_map.end()) {
        throw std::runtime_error("Buffer '" + buffer_name + "' already exists");
    }
    
    auto buffer = std::make_unique<Buffer>(host_memory, size, buffer_name, this);
    Buffer* buffer_ptr = buffer.get();
    buffer_map[buffer_name] = buffer_ptr;
    buffers.push_back(std::move(buffer));
    
    return *buffer_ptr;
}

Buffer& Dataflow::add_buffer(size_t size, const std::string& name) {
    std::string buffer_name = name.empty() ? "buffer_" + std::to_string(buffer_counter++) : name;
    
    // Check for duplicates
    if (buffer_map.find(buffer_name) != buffer_map.end()) {
        throw std::runtime_error("Buffer '" + buffer_name + "' already exists");
    }
    
    auto buffer = std::make_unique<Buffer>(size, buffer_name, this);
    Buffer* buffer_ptr = buffer.get();
    buffer_map[buffer_name] = buffer_ptr;
    buffers.push_back(std::move(buffer));
    
    return *buffer_ptr;
}

Dataflow& Dataflow::to(Buffer& src, Task::Port& dst) {
    Connection conn;
    conn.src_name = src.get_name();
    conn.dst_name = dst.get_task_name();
    conn.src_port = "output";
    conn.dst_port = dst.port_name;
    conn.src_type = "buffer";
    conn.dst_type = "task";
    
    connections.push_back(conn);
    is_built = false; // Force rebuild
    return *this;
}

Dataflow& Dataflow::to(Task::Port& src, Task::Port& dst) {
    Connection conn;
    conn.src_name = src.get_task_name();
    conn.dst_name = dst.get_task_name();
    conn.src_port = src.port_name;
    conn.dst_port = dst.port_name;
    conn.src_type = "task";
    conn.dst_type = "task";
    
    connections.push_back(conn);
    is_built = false; // Force rebuild
    return *this;
}

Dataflow& Dataflow::to(Task::Port& src, Buffer& dst) {
    Connection conn;
    conn.src_name = src.get_task_name();
    conn.dst_name = dst.get_name();
    conn.src_port = src.port_name;
    conn.dst_port = "input";
    conn.src_type = "task";
    conn.dst_type = "buffer";
    
    connections.push_back(conn);
    is_built = false; // Force rebuild
    return *this;
}

bool Dataflow::validate_connections() {
    // Check that all referenced tasks and buffers exist
    for (const auto& conn : connections) {
        if (conn.src_type == "task" && task_map.find(conn.src_name) == task_map.end()) {
            std::cerr << "Error: Source task '" << conn.src_name << "' not found" << std::endl;
            return false;
        }
        if (conn.src_type == "buffer" && buffer_map.find(conn.src_name) == buffer_map.end()) {
            std::cerr << "Error: Source buffer '" << conn.src_name << "' not found" << std::endl;
            return false;
        }
        if (conn.dst_type == "task" && task_map.find(conn.dst_name) == task_map.end()) {
            std::cerr << "Error: Destination task '" << conn.dst_name << "' not found" << std::endl;
            return false;
        }
        if (conn.dst_type == "buffer" && buffer_map.find(conn.dst_name) == buffer_map.end()) {
            std::cerr << "Error: Destination buffer '" << conn.dst_name << "' not found" << std::endl;
            return false;
        }
    }
    
    return true;
}

std::vector<std::string> Dataflow::build_execution_flow() {
    std::vector<std::string> flow;
    std::unordered_map<std::string, bool> visited;
    
    // Simple topological ordering based on connections
    // Start with components that have no incoming connections
    std::unordered_map<std::string, int> incoming_count;
    
    // Initialize counts
    for (const auto& task : tasks) {
        incoming_count[task->get_name()] = 0;
    }
    for (const auto& buffer : buffers) {
        incoming_count[buffer->get_name()] = 0;
    }
    
    // Count incoming connections
    for (const auto& conn : connections) {
        incoming_count[conn.dst_name]++;
    }
    
    // Add components with no incoming connections
    std::queue<std::string> ready;
    for (const auto& pair : incoming_count) {
        if (pair.second == 0) {
            ready.push(pair.first);
        }
    }
    
    // Build flow
    while (!ready.empty()) {
        std::string current = ready.front();
        ready.pop();
        flow.push_back(current);
        
        // Update counts for connected components
        for (const auto& conn : connections) {
            if (conn.src_name == current) {
                incoming_count[conn.dst_name]--;
                if (incoming_count[conn.dst_name] == 0) {
                    ready.push(conn.dst_name);
                }
            }
        }
    }
    
    return flow;
}

bool Dataflow::check() {
    if (is_checked && is_built) {
        return true; // Already validated and built
    }
    
    // Validate connections
    if (!validate_connections()) {
        return false;
    }
    
    // Build execution flow
    flow = build_execution_flow();
    
    // Build internal DFG
    build_internal_dfg();
    
    is_checked = true;
    return is_built;
}

void Dataflow::build_internal_dfg() {
    try {
        // Create the DFG using factory function
        dfg = dfg::create_dfg(dataflow_name);
        if (!dfg) {
            throw std::runtime_error("Failed to create DFG for dataflow " + dataflow_name);
        }
        
        // Get the DFG root capability
        dfg_root_capability = dfg->get_root_capability();
        if (!dfg_root_capability) {
            throw std::runtime_error("Failed to get root capability for dataflow " + dataflow_name);
        }
        
        if (debug_level > 1) {
            std::cout << "Dataflow initialization - Initial capability tree:" << std::endl;
            dfg::print_capability_tree(dfg);
        }
        
        // Create internal nodes and buffers
        create_internal_nodes();
        create_internal_buffers();
        
        // Setup connections
        setup_connections();
        
        // Configure IO switches and operations
        configure_io_switches();
        setup_operations();
        
        is_built = true;
        
        if (debug_level > 1) {
            std::cout << "Dataflow initialization - Final capability tree:" << std::endl;
            dfg::print_capability_tree(dfg);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during dataflow build: " << e.what() << std::endl;
        is_built = false;
    }
}

void Dataflow::create_internal_nodes() {
    nodes.reserve(tasks.size());
    
    for (size_t i = 0; i < tasks.size(); i++) {
        auto& task = tasks[i];
        if (debug_level > 0) {
            std::cout << "Creating node: " << task->get_name() << " (index " << i << ")" << std::endl;
        }
        
        // Create node using dfg factory function - FIXED API CALL
        dfg::Node* node = dfg::create_node(dfg, i, task->get_name());
        if (node) {
            nodes.push_back(node);
            node_map[task->get_name()] = node;
            
            // Update task with internal node
            task->set_internal_node(node);
            task->set_vfid(i);
            
            // Store node capability - FIXED capability finding
            std::string node_cap_id = task->get_name() + "_cap";
            dfg::Capability* node_cap = dfg->find_capability(node_cap_id, dfg_root_capability);
            
            if (node_cap) {
                node_capabilities[task->get_name()] = node_cap;
                task->set_capability(node_cap);
                if (debug_level > 1) {
                    std::cout << "Successfully found capability for node " << task->get_name() << std::endl;
                }
            } else {
                std::cerr << "Warning: Could not find capability for node " << task->get_name() << std::endl;
            }
        } else {
            std::cerr << "Error: Failed to create node " << task->get_name() << std::endl;
        }
    }
}

void Dataflow::create_internal_buffers() {
    for (auto& buffer : buffers) {
        if (debug_level > 0) {
            std::cout << "Creating buffer: " << buffer->get_name() << " (size " << buffer->get_size() << ")" << std::endl;
        }
        
        // Create buffer using dfg factory function - FIXED API CALL
        dfg::Buffer* raw_buffer = dfg::create_buffer(dfg, buffer->get_size(), buffer->get_name());
        if (raw_buffer) {
            raw_buffers[buffer->get_name()] = raw_buffer;
            
            // Update buffer with internal buffer
            buffer->set_internal_buffer(raw_buffer);
            
            // Store buffer capability - FIXED capability finding
            std::string buffer_cap_id = buffer->get_name() + "_cap";
            dfg::Capability* buffer_cap = dfg->find_capability(buffer_cap_id, dfg_root_capability);
            
            if (buffer_cap) {
                buffer_capabilities[buffer->get_name()] = buffer_cap;
                buffer->set_capability(buffer_cap);
                if (debug_level > 1) {
                    std::cout << "Successfully found capability for buffer " << buffer->get_name() << std::endl;
                }
            } else {
                std::cerr << "Warning: Could not find capability for buffer " << buffer->get_name() << std::endl;
            }
        } else {
            std::cerr << "Error: Failed to create buffer " << buffer->get_name() << std::endl;
        }
    }
}

void Dataflow::setup_connections() {
    // Connect edges according to the connection graph using dfg factory functions
    for (const auto& conn : connections) {
        if (debug_level > 0) {
            std::cout << "Connecting: " << conn.src_name << " -> " << conn.dst_name << std::endl;
        }
        
        // Use dfg factory function - FIXED to use factory function
        if (dfg::connect_edges(conn.src_name, conn.dst_name, dfg, 0, 6, true)) {
            if (debug_level > 0) {
                std::cout << "Successfully connected: " << conn.src_name << " -> " << conn.dst_name << std::endl;
            }
            
            // Store connection capabilities - FIXED capability finding
            std::string conn_source_cap_id = conn.src_name + "_to_" + conn.dst_name + "_src";
            std::string conn_target_cap_id = conn.src_name + "_to_" + conn.dst_name + "_dest";
            
            dfg::Capability* conn_source_cap = dfg->find_capability(conn_source_cap_id, dfg_root_capability);
            dfg::Capability* conn_target_cap = dfg->find_capability(conn_target_cap_id, dfg_root_capability);
            
            if (conn_source_cap) {
                connection_capabilities[conn_source_cap_id] = conn_source_cap;
            }
            if (conn_target_cap) {
                connection_capabilities[conn_target_cap_id] = conn_target_cap;
            }
        } else {
            std::cerr << "Error: Failed to connect " << conn.src_name << " -> " << conn.dst_name << std::endl;
        }
    }
}

void Dataflow::configure_io_switches() {
    // Configure IO switches for nodes based on position - FIXED to use factory functions
    if (nodes.size() >= 2) {
        if (debug_level > 0) {
            std::cout << "Configuring IO switches for nodes..." << std::endl;
        }
        
        // First node
        dfg::configure_node_io_switch(nodes[0], dfg::IODevs::Inter_2_TO_CEU_1);
        
        // Last node
        dfg::configure_node_io_switch(nodes[nodes.size()-1], dfg::IODevs::Inter_2_TO_HOST_1);
        
        // Middle nodes
        for (size_t i = 1; i < nodes.size() - 1; i++) {
            dfg::configure_node_io_switch(nodes[i], dfg::IODevs::Inter_3_TO_CEU_2);
        }
    } else if (nodes.size() == 1) {
        // Single node
        dfg::configure_node_io_switch(nodes[0], dfg::IODevs::Inter_3_TO_HOST_0);
    }
}

void Dataflow::setup_operations() {
    // Set operation type for all nodes - FIXED to use factory function
    for (auto* node : nodes) {
        if (node) {
            dfg::set_node_operation(node, dfg::CoyoteOper::LOCAL_TRANSFER);
        }
    }
}

void Dataflow::clear_completed() {
    if (!is_built) return;
    
    // Clear completion counters for all nodes
    for (auto* node : nodes) {
        if (!node) continue;
        
        dfg::Capability* node_cap = node_capabilities[node->get_id()];
        if (node_cap) {
            try {
                node->clear_completed(node_cap);
            } catch (const std::exception& e) {
                std::cerr << "Exception clearing completion counter for node " << node->get_id()
                          << ": " << e.what() << std::endl;
            }
        }
    }
}

void Dataflow::execute(size_t current_size) {
    if (!is_checked || !is_built) {
        if (!check()) {
            throw std::runtime_error("Dataflow validation failed");
        }
    }
    
    // Auto data transfer: host -> device buffers using dfg factory functions.
    // Each buffer may have a different size — clamp the per-buffer write to
    // that buffer's capacity so intermediate buffers (e.g. SHA hash = 32 B)
    // don't overflow when current_size is the much-larger input size.
    for (auto& buffer : buffers) {
        if (buffer->get_internal_buffer()) {
            size_t buf_size = buffer->get_size();
            size_t write_size = current_size > 0
                ? std::min(current_size, buf_size)
                : buf_size;
            dfg::write_buffer(buffer->get_internal_buffer(),
                             buffer->get_host_ptr(),
                             write_size);
        }
    }

    // Clear completion counters
    clear_completed();

    // Create scatter-gather entries
    std::vector<dfg::sgEntry> sg(nodes.size());

    // Initialize SG entries. The default src/dst lengths get set per-buffer
    // below; don't pre-populate with current_size here (would override
    // smaller intermediate-buffer lengths).
    for (size_t i = 0; i < nodes.size(); i++) {
        memset(&sg[i], 0, sizeof(dfg::sgEntry));
        sg[i].local.src_stream = 1; // HOST_STREAM
        sg[i].local.dst_stream = 1; // HOST_STREAM
    }
    
    // Set up scatter-gather entries based on flow
    for (size_t i = 0; i < nodes.size(); i++) {
        std::string node_name = nodes[i]->get_id();
        
        // Find node position in flow
        auto node_pos = std::find(flow.begin(), flow.end(), node_name);
        if (node_pos == flow.end()) continue;
        
        // Find input and output buffers
        std::string input_buffer_name, output_buffer_name;
        
        if (node_pos != flow.begin()) {
            auto prev = node_pos - 1;
            if (buffer_map.find(*prev) != buffer_map.end()) {
                input_buffer_name = *prev;
            }
        }
        
        if (node_pos + 1 != flow.end()) {
            auto next = node_pos + 1;
            if (buffer_map.find(*next) != buffer_map.end()) {
                output_buffer_name = *next;
            }
        }
        
        // Configure SG entry with buffer information using dfg factory functions.
        // src/dst lengths are clamped to each buffer's capacity so a stage that
        // consumes much less than current_size (e.g. RSA input = 32 B hash)
        // doesn't try to DMA current_size bytes through a tiny buffer.
        if (!input_buffer_name.empty()) {
            auto buffer_it = raw_buffers.find(input_buffer_name);

            if (buffer_it != raw_buffers.end()) {
                void* buffer_memory = dfg::read_buffer(buffer_it->second);
                if (buffer_memory) {
                    size_t buf_size = buffer_map[input_buffer_name]->get_size();
                    sg[i].local.src_addr = buffer_memory;
                    sg[i].local.src_len = current_size > 0
                        ? std::min(current_size, buf_size)
                        : buf_size;
                }
            }
        }

        if (!output_buffer_name.empty()) {
            auto buffer_it = raw_buffers.find(output_buffer_name);

            if (buffer_it != raw_buffers.end()) {
                void* buffer_memory = dfg::read_buffer(buffer_it->second);
                if (buffer_memory) {
                    size_t buf_size = buffer_map[output_buffer_name]->get_size();
                    sg[i].local.dst_addr = buffer_memory;
                    sg[i].local.dst_len = current_size > 0
                        ? std::min(current_size, buf_size)
                        : buf_size;
                }
            }
        }
        
        // Set offsets based on position
        if (i == 0) {
            sg[i].local.offset_r = 0;
            sg[i].local.offset_w = 6;
        } else if (i == nodes.size() - 1) {
            sg[i].local.offset_r = 6;
            sg[i].local.offset_w = 0;
        } else {
            sg[i].local.offset_r = 6;
            sg[i].local.offset_w = 6;
        }
    }
    
    // Execute the graph using dfg factory function - FIXED
    dfg::Node* node_array[nodes.size()];
    for (size_t i = 0; i < nodes.size(); i++) {
        node_array[i] = nodes[i];
    }
    
    dfg::execute_graph(dfg, node_array, nodes.size(), sg.data());
    
    // Wait for completion with timeout
    if (!nodes.empty()) {
        int timeout_counter = 0;
        const int max_timeout = 10000000;
        
        dfg::Capability* last_node_cap = node_capabilities[nodes.back()->get_id()];
        if (last_node_cap) {
            while(nodes.back()->check_completed(last_node_cap, dfg::CoyoteOper::LOCAL_TRANSFER) != 1) {
                timeout_counter++;
                if (dfg->is_stalled(dfg_root_capability) || timeout_counter > max_timeout) {
                    if (timeout_counter > max_timeout) {
                        std::cerr << "Warning: Execution timed out" << std::endl;
                    } else {
                        std::cerr << "Warning: Execution stalled" << std::endl;
                    }
                    break;
                }
            }
        }
    }
    
    // Auto data transfer: device -> host buffers using dfg factory functions.
    // Clamp per-buffer like the host->device write loop above. Without this,
    // small intermediates (e.g. SHA hash = 32 B) overflow when current_size
    // is the much-larger input size — the resulting heap overflow shows up
    // as a SIGSEGV at 256 K / 1 MB inputs, or as silent corruption that
    // surfaces later in the cleanup free().
    for (auto& buffer : buffers) {
        if (buffer->get_internal_buffer()) {
            void* device_memory = dfg::read_buffer(buffer->get_internal_buffer());
            if (device_memory) {
                size_t buf_size = buffer->get_size();
                size_t transfer_size = current_size > 0
                    ? std::min(current_size, buf_size)
                    : buf_size;
                memcpy(buffer->get_host_ptr(), device_memory, transfer_size);
            }
        }
    }
}

void Dataflow::release() {
    static std::atomic<bool> already_releasing(false);
    
    bool expected = false;
    if (!already_releasing.compare_exchange_strong(expected, true)) {
        return; // Already releasing
    }
    
    try {
        if (debug_level > 0) {
            std::cout << "Releasing dataflow resources..." << std::endl;
        }
        
        // Clear all internal state
        is_built = false;
        is_checked = false;
        
        // Clear capability maps
        node_capabilities.clear();
        buffer_capabilities.clear();
        connection_capabilities.clear();
        
        // Clear internal collections
        nodes.clear();
        node_map.clear();
        raw_buffers.clear();
        
        // Release DFG using factory function - FIXED
        if (dfg) {
            try {
                dfg::release_resources(dfg);
            } catch (const std::exception& e) {
                std::cerr << "Exception during DFG resource release: " << e.what() << std::endl;
            }
            dfg = nullptr;
        }
        
        dfg_root_capability = nullptr;
        
        // Clear ushell objects (they will clean up automatically)
        task_map.clear();
        buffer_map.clear();
        tasks.clear();
        buffers.clear();
        connections.clear();
        flow.clear();
        
    } catch (const std::exception& e) {
        std::cerr << "Critical error during dataflow release: " << e.what() << std::endl;
    }
    
    already_releasing.store(false);
}

// Access methods
Task* Dataflow::get_task(const std::string& name) {
    auto it = task_map.find(name);
    return it != task_map.end() ? it->second : nullptr;
}

Buffer* Dataflow::get_buffer(const std::string& name) {
    auto it = buffer_map.find(name);
    return it != buffer_map.end() ? it->second : nullptr;
}

// Advanced capability operations using dfg factory functions
dfg::Capability* Dataflow::create_capability(const std::string& component_name, 
                                           const std::string& cap_space_id,
                                           uint32_t access,
                                           bool allow_transitive_delegation) {
    if (!dfg) return nullptr;
    
    // FIXED: Use dfg factory function
    return dfg::create_capability(dfg, dataflow_name, component_name, cap_space_id, access, allow_transitive_delegation);
}

dfg::Capability* Dataflow::delegate_capability(const std::string& src_component, 
                                             const std::string& cap_space_id,
                                             uint32_t access,
                                             bool allow_transitive_delegation) {
    if (!dfg) return nullptr;
    
    // FIXED: Use dfg factory function
    return dfg::delegate_capability(dfg, dataflow_name, src_component, cap_space_id, access, allow_transitive_delegation);
}

bool Dataflow::can_component_delegate_delegation(const std::string& component_name) const {
    // Check nodes
    auto node_cap_it = node_capabilities.find(component_name);
    if (node_cap_it != node_capabilities.end() && node_cap_it->second) {
        return node_cap_it->second->can_delegate_delegation();
    }
    
    // Check buffers
    auto buffer_cap_it = buffer_capabilities.find(component_name);
    if (buffer_cap_it != buffer_capabilities.end() && buffer_cap_it->second) {
        return buffer_cap_it->second->can_delegate_delegation();
    }
    
    return false;
}

bool Dataflow::transfer_ownership(const std::string& src_component, const std::string& dst_component) {
    if (!dfg) return false;
    return dfg::transfer_ownership(dfg, dataflow_name, src_component, dst_component);
}

bool Dataflow::revoke_capability(const std::string& component_name, const std::string& cap_space_id) {
    if (!dfg) return false;
    return dfg::revoke_capability(dfg, dataflow_name, component_name, cap_space_id);
}

bool Dataflow::expire_capability(const std::string& component_name, const std::string& cap_space_id, uint32_t timeout) {
    if (!dfg) return false;
    return dfg::expire_capability(dfg, dataflow_name, component_name, cap_space_id, timeout);
}

bool Dataflow::reset_capabilities(const std::string& component_name) {
    if (!dfg) return false;
    return dfg::reset_capabilities(dfg, dataflow_name, component_name);
}

void Dataflow::print_capability_tree() const {
    if (dfg) {
        std::cout << "Capability Tree for Dataflow '" << dataflow_name << "':" << std::endl;
        dfg::print_capability_tree(dfg);
    }
}

void Dataflow::print_graph() const {
    std::cout << "Dataflow Graph '" << dataflow_name << "':" << std::endl;
    std::cout << "Tasks: ";
    for (const auto& task : tasks) {
        std::cout << task->get_name() << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Buffers: ";
    for (const auto& buffer : buffers) {
        std::cout << buffer->get_name() << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Connections:" << std::endl;
    for (const auto& conn : connections) {
        std::cout << "  " << conn.src_name << " -> " << conn.dst_name << std::endl;
    }
}

void Dataflow::print_connections() const {
    std::cout << "Connections for dataflow '" << dataflow_name << "':" << std::endl;
    for (const auto& conn : connections) {
        std::cout << "  " << conn.src_name << "(" << conn.src_type << ":" << conn.src_port 
                  << ") -> " << conn.dst_name << "(" << conn.dst_type << ":" << conn.dst_port << ")" << std::endl;
    }
}

// ================================================================
// Factory Functions - Additional helper functions for compatibility
// ================================================================

// Write data to a dataflow buffer
void write_dataflow_buffer(Buffer& buffer, void* data, size_t size) {
    buffer.write_from_host(data, size);
}

// Read data from a dataflow buffer
void read_dataflow_buffer(Buffer& buffer, void* dest, size_t size) {
    buffer.read_to_host(dest, size);
}

// Get buffer from dataflow
Buffer* get_dataflow_buffer(Dataflow* dataflow, const std::string& buffer_name) {
    return dataflow ? dataflow->get_buffer(buffer_name) : nullptr;
}

// Create a task
Task task(const std::string& name, const std::string& type = "processing") {
    return Task(name, type, nullptr);
}

// Create a buffer from host memory
Buffer buffer(void* host_ptr, size_t size = 0) {
    return Buffer(host_ptr, size);
}

// Template version for array auto-sizing
template<typename T, size_t N>
Buffer buffer(T (&array)[N]) {
    return Buffer(array, sizeof(array));
}

// Create a buffer with specified size
Buffer buffer(size_t size) {
    return Buffer(size);
}

// Create a dataflow
Dataflow dataflow(const std::string& name = "") {
    return Dataflow(name);
}

} // namespace ushell