#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <atomic>
#include "dfg.hpp"

namespace pipeline {

using namespace fpga;
using namespace std;
using namespace std::chrono;

// Forward declarations
class Pipeline;
class Buffer;

// Type definitions for node and buffer configurations
enum class BufferMode {
    RW, // Read-Write
    R,  // Read-only
    W   // Write-only
};

typedef struct {
    std::string name;
    std::string type;
} node_def_t;

typedef struct {
    std::string name;
    size_t size;
    BufferMode mode;
} buffer_def_t;

// Pipeline definition struct
typedef struct {
    std::string name;
    std::vector<node_def_t> nodes;
    size_t num_nodes;
    std::vector<buffer_def_t> buffers;
    size_t num_buffers;
    std::vector<std::string> flow;
    size_t num_flow;
} pipeline_def_t;

/**
 * Buffer class - High-level wrapper around the low-level Buffer
 */
class Buffer {
private:
    std::string name;
    size_t size;
    BufferMode mode;
    dfg::Buffer* raw_buffer;
    
public:
    Buffer(const std::string& name, size_t size, BufferMode mode, dfg::Buffer* raw_buffer)
        : name(name), size(size), mode(mode), raw_buffer(raw_buffer) {}
    
    ~Buffer() = default; // Managed by Pipeline
    
    // Getters
    std::string get_name() const { return name; }
    size_t get_size() const { return size; }
    BufferMode get_mode() const { return mode; }
    dfg::Buffer* get_raw_buffer() const { return raw_buffer; }
    
    // Update the buffer size (for current operation)
    void set_size(size_t new_size) { size = new_size; }
};

/**
 * Pipeline class - High-level wrapper around the low-level DFG
 */
class Pipeline {
private:
    std::string name;
    dfg::DFG* dfg;
    std::vector<dfg::Node*> nodes;
    std::unordered_map<std::string, dfg::Node*> node_map;
    std::unordered_map<std::string, Buffer*> buffers;
    std::vector<std::string> flow;
    
    // Capability management
    dfg::Capability* dfg_root_capability;
    std::unordered_map<std::string, dfg::Capability*> node_capabilities;
    std::unordered_map<std::string, dfg::Capability*> buffer_capabilities;
    std::unordered_map<std::string, dfg::Capability*> connection_capabilities;
    
    // Debug level for controlling verbosity
    int debug_level = 1; // 0=minimal, 1=normal, 2=verbose
    
    // Initialize the pipeline from a definition
    void initialize(const pipeline_def_t& def);
    
public:
    // Constructor with definition
    Pipeline(const pipeline_def_t& def);
    
    // Destructor
    ~Pipeline();
    
    // Get buffer by name
    Buffer* get_buffer(const std::string& buffer_name);
    
    // Get node by name
    dfg::Node* get_node(const std::string& node_name);
    
    // Get buffer capability
    dfg::Capability* get_buffer_capability(const std::string& buffer_name) const;
    
    // Get node capability
    dfg::Capability* get_node_capability(const std::string& node_name) const;
    
    // Execute the pipeline with optional current buffer size
    void execute(size_t current_size = 0);
    
    // Release all resources
    void release();
    
    // Clear completion counters
    void clear_completed();
    
    // Get the underlying DFG for capability checks
    dfg::DFG* get_dfg() const { return dfg; }
    
    // Get the DFG root capability
    dfg::Capability* get_dfg_root_capability() const { return dfg_root_capability; }
    
    // Set debug level
    void set_debug_level(int level) { debug_level = level; }
    
    // -------------------- Enhanced Capability Operations --------------------
    
    /**
     * Create a capability for a pipeline component
     * @param component_name Name of the component (node or buffer)
     * @param cap_space_id ID for the new capability space
     * @param access Permission flags for the capability
     * @param allow_transitive_delegation Whether to grant transitive delegation permission
     * @return Pointer to the created capability or nullptr on failure
     */
    dfg::Capability* create_capability(const std::string& component_name, 
        const std::string& cap_space_id,
        uint32_t access,
        bool allow_transitive_delegation = false) {
        if (!dfg) return nullptr;

        if (allow_transitive_delegation) {
        access |= dfg::CapabilityPermission::TRANSITIVE_DELEGATE;
        }

        return dfg::create_capability(dfg, name, component_name, cap_space_id, access, allow_transitive_delegation);
    }
    
    /**
     * Delegate a capability from one component to another
     * @param src_component Source component with the capability
     * @param cap_space_id ID for the new capability space
     * @param access Permission flags for the capability
     * @param allow_transitive_delegation Whether to grant transitive delegation permission
     * @return Pointer to the delegated capability or nullptr on failure
     */
    dfg::Capability* delegate_capability(const std::string& src_component, 
        const std::string& cap_space_id,
        uint32_t access,
        bool allow_transitive_delegation = false) {
        if (!dfg) return nullptr;

        if (allow_transitive_delegation) {
        access |= dfg::CapabilityPermission::TRANSITIVE_DELEGATE;
        }

        return dfg::delegate_capability(dfg, name, src_component, cap_space_id, access, allow_transitive_delegation);
    }
    
    /**
     * Check if a component has transitive delegation capability
     * @param component_name Name of the component to check
     * @return True if the component can delegate delegation rights
     */
    bool can_component_delegate_delegation(const std::string& component_name) const {
        if (!dfg) return false;
        
        // For nodes
        auto node_cap_it = node_capabilities.find(component_name);
        if (node_cap_it != node_capabilities.end() && node_cap_it->second) {
            return node_cap_it->second->can_delegate_delegation();
        }
        
        // For buffers
        auto buffer_cap_it = buffer_capabilities.find(component_name);
        if (buffer_cap_it != buffer_capabilities.end() && buffer_cap_it->second) {
            return buffer_cap_it->second->can_delegate_delegation();
        }
        
        return false;
    }
    
    /**
     * Transfer ownership of capabilities between components
     * @param src_component Source component
     * @param dst_component Destination component
     * @return True if successful, false otherwise
     */
    bool transfer_ownership(const std::string& src_component, 
                           const std::string& dst_component) {
        if (!dfg) return false;
        return dfg::transfer_ownership(dfg, name, src_component, dst_component);
    }
    
    /**
     * Revoke a capability
     * @param component_name Component that owns the capability
     * @param cap_space_id ID of the capability space to revoke
     * @return True if successful, false otherwise
     */
    bool revoke_capability(const std::string& component_name, 
                          const std::string& cap_space_id) {
        if (!dfg) return false;
        return dfg::revoke_capability(dfg, name, component_name, cap_space_id);
    }
    
    /**
     * Set an expiration time for a capability
     * @param component_name Component that owns the capability
     * @param cap_space_id ID of the capability space
     * @param timeout Timeout in seconds
     * @return True if successful, false otherwise
     */
    bool expire_capability(const std::string& component_name, 
                          const std::string& cap_space_id,
                          uint32_t timeout) {
        if (!dfg) return false;
        return dfg::expire_capability(dfg, name, component_name, cap_space_id, timeout);
    }
    
    /**
     * Reset all capabilities for a component
     * @param component_name Component to reset
     * @return True if successful, false otherwise
     */
    bool reset_capabilities(const std::string& component_name) {
        if (!dfg) return false;
        return dfg::reset_capabilities(dfg, name, component_name);
    }
    
    /**
     * Print the capability tree for this pipeline
     */
    void print_capability_tree() const {
        if (dfg) {
            std::cout << "Capability Tree for Pipeline '" << name << "':" << std::endl;
            dfg::print_capability_tree(dfg);
        }
    }
};

// -------------------- Implementation of Pipeline methods --------------------

Pipeline::Pipeline(const pipeline_def_t& def) {
    initialize(def);
}

Pipeline::~Pipeline() {
    release();
}

void Pipeline::initialize(const pipeline_def_t& def) {
    name = def.name;
    flow = def.flow;
    
    // Create the DFG
    dfg = dfg::create_dfg(name);
    if (!dfg) {
        std::cerr << "Error: Failed to create DFG for pipeline " << name << std::endl;
        return;
    }
    
    // Get the DFG root capability
    dfg_root_capability = dfg->get_root_capability();
    if (!dfg_root_capability) {
        std::cerr << "Error: Failed to get root capability for pipeline " << name << std::endl;
        return;
    }
    
    // Print capability tree for debugging if verbose
    if (debug_level > 1) {
        std::cout << "Pipeline initialization - Initial capability tree:" << std::endl;
        dfg->print_capability_tree(dfg_root_capability);
    }
    
    // Create nodes with proper capability handling - UPDATED for new dfg API
    nodes.reserve(def.num_nodes);
    for (size_t i = 0; i < def.num_nodes; i++) {
        const auto& node_def = def.nodes[i];
        if (debug_level > 0) {
            std::cout << "Creating node: " << node_def.name << " (index " << i << ")" << std::endl;
        }
        
        // CRITICAL CHANGE: Updated function call for new dfg API
        dfg::Node* node = dfg::create_node(dfg, i, node_def.name);
        if (node) {
            nodes.push_back(node);
            node_map[node_def.name] = node;
            
            // Store node capability with TRANSITIVE_DELEGATE permission
            std::string node_cap_id = node_def.name + "_cap";
            
            // Find the existing capability
            dfg::Capability* node_cap = dfg->find_capability(node_cap_id, dfg_root_capability);
            
            if (node_cap) {
                // Add the TRANSITIVE_DELEGATE permission using root capability
                uint32_t new_perms = node_cap->get_permissions() | dfg::CapabilityPermission::TRANSITIVE_DELEGATE;
                
                // Create a new capability with updated permissions
                dfg::Capability* enhanced_cap = dfg::create_capability(
                    dfg, name, node_def.name, node_cap_id + "_enhanced",
                    new_perms, true);
                
                if (enhanced_cap) {
                    // Replace the original capability with enhanced one
                    node_capabilities[node_def.name] = enhanced_cap;
                    if (debug_level > 1) {
                        std::cout << "Successfully enhanced capability for node " << node_def.name 
                                  << " with TRANSITIVE_DELEGATE" << std::endl;
                    }
                } else {
                    node_capabilities[node_def.name] = node_cap;
                    std::cerr << "Warning: Could not enhance capability for node " << node_def.name << std::endl;
                }
            } else {
                std::cerr << "Warning: Could not find capability for node " << node_def.name << std::endl;
            }
        } else {
            std::cerr << "Error: Failed to create node " << node_def.name << std::endl;
        }
    }
    
    // Create buffers with proper capability handling - UPDATED for new dfg API
    for (const auto& buffer_def : def.buffers) {
        if (debug_level > 0) {
            std::cout << "Creating buffer: " << buffer_def.name << " (size " << buffer_def.size << ")" << std::endl;
        }
        
        // CRITICAL CHANGE: Updated function call for new dfg API
        dfg::Buffer* raw_buffer = dfg::create_buffer(dfg, buffer_def.size, buffer_def.name);
        if (raw_buffer) {
            Buffer* buffer = new Buffer(
                buffer_def.name, 
                buffer_def.size, 
                buffer_def.mode, 
                raw_buffer
            );
            buffers[buffer_def.name] = buffer;
            
            // Store buffer capability with TRANSITIVE_DELEGATE permission
            std::string buffer_cap_id = buffer_def.name + "_cap";
            
            // Find the existing capability
            dfg::Capability* buffer_cap = dfg->find_capability(buffer_cap_id, dfg_root_capability);
            
            if (buffer_cap) {
                // Add the TRANSITIVE_DELEGATE permission using root capability
                uint32_t new_perms = buffer_cap->get_permissions() | dfg::CapabilityPermission::TRANSITIVE_DELEGATE;
                
                // Create a new capability with updated permissions
                dfg::Capability* enhanced_cap = dfg::create_capability(
                    dfg, name, buffer_def.name, buffer_cap_id + "_enhanced",
                    new_perms, true);
                
                if (enhanced_cap) {
                    // Replace the original capability with enhanced one
                    buffer_capabilities[buffer_def.name] = enhanced_cap;
                    if (debug_level > 1) {
                        std::cout << "Successfully enhanced capability for buffer " << buffer_def.name 
                                  << " with TRANSITIVE_DELEGATE" << std::endl;
                    }
                } else {
                    buffer_capabilities[buffer_def.name] = buffer_cap;
                    std::cerr << "Warning: Could not enhance capability for buffer " << buffer_def.name << std::endl;
                }
            } else {
                std::cerr << "Warning: Could not find capability for buffer " << buffer_def.name << std::endl;
            }
        } else {
            std::cerr << "Error: Failed to create buffer " << buffer_def.name << std::endl;
        }
    }
    
    // Connect edges according to flow with improved error handling
    for (size_t i = 0; i < def.num_flow - 1; i++) {
        const std::string& src = def.flow[i];
        const std::string& dst = def.flow[i + 1];
        
        if (debug_level > 0) {
            std::cout << "Connecting: " << src << " -> " << dst << std::endl;
        }
        
        // Find source and destination
        auto src_node_it = node_map.find(src);
        auto dst_node_it = node_map.find(dst);
        
        if (src_node_it != node_map.end() && dst_node_it != node_map.end()) {
            // Both are nodes, connect them using the global connect_edges function
            // that performs capability delegation
            if (dfg::connect_edges(src, dst, dfg, 0, 6, true)) {
                if (debug_level > 0) {
                    std::cout << "Successfully connected nodes: " << src << " -> " << dst << std::endl;
                }
                
                // Store connection capabilities
                std::string conn_source_cap_id = src + "_to_" + dst + "_src";
                std::string conn_target_cap_id = src + "_to_" + dst + "_dest";
                
                dfg::Capability* conn_source_cap = dfg->find_capability(conn_source_cap_id, dfg_root_capability);
                dfg::Capability* conn_target_cap = dfg->find_capability(conn_target_cap_id, dfg_root_capability);
                
                if (conn_source_cap) {
                    connection_capabilities[conn_source_cap_id] = conn_source_cap;
                } else if (debug_level > 0) {
                    std::cerr << "Warning: Could not find source connection capability" << std::endl;
                }
                
                if (conn_target_cap) {
                    connection_capabilities[conn_target_cap_id] = conn_target_cap;
                } else if (debug_level > 0) {
                    std::cerr << "Warning: Could not find target connection capability" << std::endl;
                }
                
                // Configure node edges for physical connection
                dfg::Capability* src_cap = node_capabilities[src];
                dfg::Capability* dst_cap = node_capabilities[dst];
                
                if (src_cap) {
                    // For node-to-node connection, source writes to offset 6
                    src_node_it->second->connect_edges(0, 6, src_cap, true);
                } else if (debug_level > 0) {
                    std::cerr << "Warning: Could not get source node capability for physical connection" << std::endl;
                }
                
                if (dst_cap) {
                    // For node-to-node connection, destination reads from offset 6
                    dst_node_it->second->connect_edges(6, 0, dst_cap, true);
                } else if (debug_level > 0) {
                    std::cerr << "Warning: Could not get destination node capability for physical connection" << std::endl;
                }
            } else {
                std::cerr << "Error: Failed to connect nodes: " << src << " -> " << dst << std::endl;
            }
        }
        else if (src_node_it != node_map.end()) {
            // Source is a node, destination might be a buffer
            auto buf_it = buffers.find(dst);
            if (buf_it != buffers.end()) {
                // Connect node to buffer with capability delegation
                if (dfg::connect_edges(src, dst, dfg, 0, 0, true)) {
                    if (debug_level > 0) {
                        std::cout << "Successfully connected node to buffer: " << src << " -> " << dst << std::endl;
                    }
                    
                    // Store connection capabilities
                    std::string conn_source_cap_id = src + "_to_" + dst + "_src";
                    std::string conn_target_cap_id = src + "_to_" + dst + "_dest";
                    
                    dfg::Capability* conn_source_cap = dfg->find_capability(conn_source_cap_id, dfg_root_capability);
                    dfg::Capability* conn_target_cap = dfg->find_capability(conn_target_cap_id, dfg_root_capability);
                    
                    if (conn_source_cap) {
                        connection_capabilities[conn_source_cap_id] = conn_source_cap;
                    }
                    
                    if (conn_target_cap) {
                        connection_capabilities[conn_target_cap_id] = conn_target_cap;
                    }
                    
                    // Configure node for output (physical connection)
                    dfg::Capability* src_cap = node_capabilities[src];
                    if (src_cap) {
                        // For node-to-buffer connection, node writes to offset 0
                        src_node_it->second->connect_edges(0, 0, src_cap, true);
                    } else if (debug_level > 0) {
                        std::cerr << "Warning: Could not get node capability for node-to-buffer connection" << std::endl;
                    }
                } else {
                    std::cerr << "Error: Failed to connect node to buffer: " << src << " -> " << dst << std::endl;
                }
            } else {
                std::cerr << "Error: Destination component not found: " << dst << std::endl;
            }
        }
        else if (dst_node_it != node_map.end()) {
            // Destination is a node, source might be a buffer
            auto buf_it = buffers.find(src);
            if (buf_it != buffers.end()) {
                // Connect buffer to node with capability delegation
                if (dfg::connect_edges(src, dst, dfg, 0, 0, true)) {
                    if (debug_level > 0) {
                        std::cout << "Successfully connected buffer to node: " << src << " -> " << dst << std::endl;
                    }
                    
                    // Store connection capabilities
                    std::string conn_source_cap_id = src + "_to_" + dst + "_src";
                    std::string conn_target_cap_id = src + "_to_" + dst + "_dest";
                    
                    dfg::Capability* conn_source_cap = dfg->find_capability(conn_source_cap_id, dfg_root_capability);
                    dfg::Capability* conn_target_cap = dfg->find_capability(conn_target_cap_id, dfg_root_capability);
                    
                    if (conn_source_cap) {
                        connection_capabilities[conn_source_cap_id] = conn_source_cap;
                    }
                    
                    if (conn_target_cap) {
                        connection_capabilities[conn_target_cap_id] = conn_target_cap;
                    }
                    
                    // Configure node for input (physical connection)
                    dfg::Capability* dst_cap = node_capabilities[dst];
                    if (dst_cap) {
                        // For buffer-to-node connection, node reads from offset 0
                        dst_node_it->second->connect_edges(0, 0, dst_cap, true);
                    } else if (debug_level > 0) {
                        std::cerr << "Warning: Could not get node capability for buffer-to-node connection" << std::endl;
                    }
                } else {
                    std::cerr << "Error: Failed to connect buffer to node: " << src << " -> " << dst << std::endl;
                }
            } else {
                std::cerr << "Error: Source component not found: " << src << std::endl;
            }
        }
        // If neither are nodes, they might be buffer-to-buffer (not common)
        else {
            auto src_buf_it = buffers.find(src);
            auto dst_buf_it = buffers.find(dst);
            
            if (src_buf_it != buffers.end() && dst_buf_it != buffers.end()) {
                // Connect buffer to buffer with capability delegation
                if (dfg::connect_edges(src, dst, dfg, 0, 0, true)) {
                    if (debug_level > 0) {
                        std::cout << "Successfully connected buffer to buffer: " << src << " -> " << dst << std::endl;
                    }
                    
                    // Store connection capabilities
                    std::string conn_source_cap_id = src + "_to_" + dst + "_src";
                    std::string conn_target_cap_id = src + "_to_" + dst + "_dest";
                    
                    dfg::Capability* conn_source_cap = dfg->find_capability(conn_source_cap_id, dfg_root_capability);
                    dfg::Capability* conn_target_cap = dfg->find_capability(conn_target_cap_id, dfg_root_capability);
                    
                    if (conn_source_cap) {
                        connection_capabilities[conn_source_cap_id] = conn_source_cap;
                    }
                    
                    if (conn_target_cap) {
                        connection_capabilities[conn_target_cap_id] = conn_target_cap;
                    }
                } else {
                    std::cerr << "Error: Failed to connect buffer to buffer: " << src << " -> " << dst << std::endl;
                }
            } else {
                std::cerr << "Error: Cannot find source or destination component: " << src << " -> " << dst << std::endl;
            }
        }
    }
    
    // Configure IO switches for nodes based on position in flow with reliable error handling
    if (nodes.size() >= 2) {
        if (debug_level > 0) {
            std::cout << "Configuring IO switches for nodes..." << std::endl;
        }
        
        // First node configuration
        dfg::Capability* first_node_cap = node_capabilities[nodes[0]->get_id()];
        if (first_node_cap) {
            nodes[0]->set_io_switch(dfg::IODevs::Inter_2_TO_DTU_1, first_node_cap);
            if (debug_level > 1) {
                std::cout << "Set first node IO switch to Inter_2_TO_DTU_1" << std::endl;
            }
        } else if (debug_level > 0) {
            std::cerr << "Warning: Could not get capability for first node IO switch configuration" << std::endl;
        }
        
        // Last node configuration
        dfg::Capability* last_node_cap = node_capabilities[nodes[nodes.size()-1]->get_id()];
        if (last_node_cap) {
            nodes[nodes.size()-1]->set_io_switch(dfg::IODevs::Inter_2_TO_HOST_1, last_node_cap);
            if (debug_level > 1) {
                std::cout << "Set last node IO switch to Inter_2_TO_HOST_1" << std::endl;
            }
        } else if (debug_level > 0) {
            std::cerr << "Warning: Could not get capability for last node IO switch configuration" << std::endl;
        }
        
        // Middle nodes configuration
        for (size_t i = 1; i < nodes.size() - 1; i++) {
            dfg::Capability* node_cap = node_capabilities[nodes[i]->get_id()];
            if (node_cap) {
                nodes[i]->set_io_switch(dfg::IODevs::Inter_3_TO_DTU_2, node_cap);
                if (debug_level > 1) {
                    std::cout << "Set middle node " << i << " IO switch to Inter_3_TO_DTU_2" << std::endl;
                }
            } else if (debug_level > 0) {
                std::cerr << "Warning: Could not get capability for middle node " << i << " IO switch configuration" << std::endl;
            }
        }
    }
    else if (nodes.size() == 1) {
        // Single node configuration
        dfg::Capability* node_cap = node_capabilities[nodes[0]->get_id()];
        if (node_cap) {
            nodes[0]->set_io_switch(dfg::IODevs::Inter_3_TO_HOST_0, node_cap);
            if (debug_level > 1) {
                std::cout << "Set single node IO switch to Inter_3_TO_HOST_0" << std::endl;
            }
        } else if (debug_level > 0) {
            std::cerr << "Warning: Could not get capability for single node IO switch configuration" << std::endl;
        }
    }
    
    // Set operation type for all nodes with proper error handling
    if (debug_level > 0) {
        std::cout << "Setting node operations..." << std::endl;
    }
    
    for (auto* node : nodes) {
        dfg::Capability* node_cap = node_capabilities[node->get_id()];
        if (node_cap) {
            node->set_operation(dfg::CoyoteOper::LOCAL_TRANSFER, node_cap);
            if (debug_level > 1) {
                std::cout << "Set node " << node->get_id() << " operation to LOCAL_TRANSFER" << std::endl;
            }
        } else if (debug_level > 0) {
            std::cerr << "Warning: Could not get capability for node " << node->get_id() << " operation configuration" << std::endl;
        }
    }
    
    // Print final capability tree for debugging
    if (debug_level > 1) {
        std::cout << "Pipeline initialization - Final capability tree:" << std::endl;
        dfg->print_capability_tree(dfg_root_capability);
    }
}

Buffer* Pipeline::get_buffer(const std::string& buffer_name) {
    if (buffer_name.empty()) {
        std::cerr << "Error: Empty buffer name in get_buffer" << std::endl;
        return nullptr;
    }
    
    auto it = buffers.find(buffer_name);
    if (it != buffers.end()) {
        return it->second;
    }
    
    if (debug_level > 0) {
        std::cerr << "Error: Buffer not found: " << buffer_name << std::endl;
    }
    return nullptr;
}

dfg::Capability* Pipeline::get_buffer_capability(const std::string& buffer_name) const {
    if (buffer_name.empty()) {
        std::cerr << "Error: Empty buffer name in get_buffer_capability" << std::endl;
        return nullptr;
    }
    
    auto it = buffer_capabilities.find(buffer_name);
    if (it != buffer_capabilities.end()) {
        return it->second;
    }
    
    if (debug_level > 0) {
        std::cerr << "Error: Buffer capability not found: " << buffer_name << std::endl;
    }
    return nullptr;
}

dfg::Node* Pipeline::get_node(const std::string& node_name) {
    if (node_name.empty()) {
        std::cerr << "Error: Empty node name in get_node" << std::endl;
        return nullptr;
    }
    
    auto it = node_map.find(node_name);
    if (it != node_map.end()) {
        return it->second;
    }
    
    if (debug_level > 0) {
        std::cerr << "Error: Node not found: " << node_name << std::endl;
    }
    return nullptr;
}

dfg::Capability* Pipeline::get_node_capability(const std::string& node_name) const {
    if (node_name.empty()) {
        std::cerr << "Error: Empty node name in get_node_capability" << std::endl;
        return nullptr;
    }
    
    auto it = node_capabilities.find(node_name);
    if (it != node_capabilities.end()) {
        return it->second;
    }
    
    if (debug_level > 0) {
        std::cerr << "Error: Node capability not found: " << node_name << std::endl;
    }
    return nullptr;
}

void Pipeline::clear_completed() {
    // Clear completion counters for all nodes with proper error handling
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
        } else if (debug_level > 0) {
            std::cerr << "Warning: Could not get capability for node " << node->get_id() 
                      << " to clear completion" << std::endl;
            
            // Try with root capability as fallback
            if (dfg_root_capability) {
                try {
                    node->clear_completed(dfg_root_capability);
                } catch (const std::exception& e) {
                    std::cerr << "Exception clearing completion counter with root capability: " 
                              << e.what() << std::endl;
                }
            }
        }
    }
}

void Pipeline::execute(size_t current_size) {
    // Force cleanup from previous runs
    clear_completed();
        
    // Create fresh scatter-gather entries each time
    std::vector<dfg::sgEntry> sg(nodes.size());
    
    // Initialize all SG entries with proper values
    for (size_t i = 0; i < nodes.size(); i++) {
        // Complete reset of SG entry
        memset(&sg[i], 0, sizeof(dfg::sgEntry));
        
        // Explicitly set buffer sizes for all entries using the provided current_size
        if (current_size > 0) {
            sg[i].local.src_len = current_size;
            sg[i].local.dst_len = current_size;
        }
        
        // Set default stream mode
        sg[i].local.src_stream = 1; // HOST_STREAM  
        sg[i].local.dst_stream = 1; // HOST_STREAM
    }

    // Set up scatter-gather entries for each node
    for (size_t i = 0; i < nodes.size(); i++) {
        // Try to find input and output buffers for this node in the flow
        std::string node_name;
        for (const auto& pair : node_map) {
            if (pair.second == nodes[i]) {
                node_name = pair.first;
                break;
            }
        }
        
        // Find this node's position in the flow
        auto node_pos = std::find(flow.begin(), flow.end(), node_name);
        if (node_pos == flow.end()) continue;
        
        // Find potential input buffer (before this node)
        std::string input_buffer_name;
        if (node_pos != flow.begin()) {
            auto prev = node_pos - 1;
            if (node_map.find(*prev) == node_map.end()) {
                // It's not a node, must be a buffer
                input_buffer_name = *prev;
            }
        }
        
        // Find potential output buffer (after this node)
        std::string output_buffer_name;
        if (node_pos + 1 != flow.end()) {
            auto next = node_pos + 1;
            if (node_map.find(*next) == node_map.end()) {
                // It's not a node, must be a buffer
                output_buffer_name = *next;
            }
        }
        
        // Configure SG entry with buffer information
        if (!input_buffer_name.empty() && buffers.find(input_buffer_name) != buffers.end()) {
            Buffer* input_buffer = buffers[input_buffer_name];
            dfg::Capability* buffer_cap = buffer_capabilities[input_buffer_name];
            
            if (buffer_cap && input_buffer && input_buffer->get_raw_buffer()) {
                // Get buffer memory with capability
                void* buffer_memory = input_buffer->get_raw_buffer()->get_memory(buffer_cap);
                if (buffer_memory) {
                    // Use direct memory pointer for src_addr
                    sg[i].local.src_addr = buffer_memory;
                    // Force src_len to be exactly current_size if provided
                    sg[i].local.src_len = current_size > 0 ? current_size : input_buffer->get_size();
                    sg[i].local.src_stream = 1; // HOST_STREAM
                }
            }
        }
        
        if (!output_buffer_name.empty() && buffers.find(output_buffer_name) != buffers.end()) {
            Buffer* output_buffer = buffers[output_buffer_name];
            dfg::Capability* buffer_cap = buffer_capabilities[output_buffer_name];
            
            if (buffer_cap && output_buffer && output_buffer->get_raw_buffer()) {
                // Get buffer memory with capability
                void* buffer_memory = output_buffer->get_raw_buffer()->get_memory(buffer_cap);
                if (buffer_memory) {
                    // Use direct memory pointer for dst_addr
                    sg[i].local.dst_addr = buffer_memory;
                    // Force dst_len to be exactly current_size if provided
                    sg[i].local.dst_len = current_size > 0 ? current_size : output_buffer->get_size();
                    sg[i].local.dst_stream = 1; // HOST_STREAM
                }
            }
        }
        
        // If both input and output are the same buffer (R/W mode)
        if (input_buffer_name == output_buffer_name && !input_buffer_name.empty()) {
            sg[i].local.src_addr = sg[i].local.dst_addr;
            sg[i].local.src_len = sg[i].local.dst_len;
        }
        
        // Set offsets based on position in flow
        if (i == 0) {
            // First node
            sg[i].local.offset_r = 0;
            sg[i].local.offset_w = 6;
        } 
        else if (i == nodes.size() - 1) {
            // Last node
            sg[i].local.offset_r = 6;
            sg[i].local.offset_w = 0;
        }
        else {
            // Middle node
            sg[i].local.offset_r = 6;
            sg[i].local.offset_w = 6;
        }
    }
        
    // Execute with fresh SG entries
    dfg::Node* node_array[nodes.size()];
    for (size_t i = 0; i < nodes.size(); i++) {
        node_array[i] = nodes[i];
    }
    
    // Execute the graph with DFG root capability
    dfg->execute_graph(node_array, nodes.size(), sg.data(), dfg_root_capability);
    
    // Add a timeout to the wait loop
    if (!nodes.empty()) {
        int timeout_counter = 0;
        const int max_timeout = 10000000; // Reasonable maximum wait time
        
        // Get capability for the last node
        dfg::Capability* last_node_cap = node_capabilities[nodes.back()->get_id()];
        
        if (last_node_cap) {
            while(nodes.back()->check_completed(last_node_cap, dfg::CoyoteOper::LOCAL_WRITE) != 1) {
                timeout_counter++;
                
                // Check for stall or timeout
                if (dfg->is_stalled(dfg_root_capability) || timeout_counter > max_timeout) {
                    if (timeout_counter > max_timeout) {
                        std::cerr << "Warning: Execution timed out after " << timeout_counter << " attempts" << std::endl;
                    }
                    else {
                        std::cerr << "Warning: Execution stalled" << std::endl;
                    }
                    break; // Exit the loop instead of throwing
                }
            }
        }
    }
}

void Pipeline::release() {
    // Add static atomic flag to prevent double release
    static std::atomic<bool> already_releasing(false);
    
    // Try to set the flag, return if already set (already releasing)
    bool expected = false;
    if (!already_releasing.compare_exchange_strong(expected, true)) {
        if (debug_level > 0) {
            std::cout << "Pipeline release already in progress, skipping duplicate call." << std::endl;
        }
        return;
    }
    
    try {
        if (debug_level > 0) {
            std::cout << "Releasing pipeline resources..." << std::endl;
        }
        
        // First make sure all nodes are idle
        for (auto* node : nodes) {
            if (!node) continue;
            
            dfg::Capability* node_cap = node_capabilities[node->get_id()];
            if (node_cap) {
                try {
                    // Send a hardware reset signal or equivalent
                    node->clear_completed(node_cap);
                } catch (const std::exception& e) {
                    std::cerr << "Exception clearing completion counter during release: "
                            << e.what() << std::endl;
                }
            }
        }
        
        // Set the DFG to stalled state to prevent new operations
        if (dfg && dfg_root_capability) {
            dfg->set_stalled(true, dfg_root_capability);
        }
        
        // Safely disconnect all edges according to flow
        for (size_t i = 0; i < flow.size() - 1; i++) {
            dfg::disconnect_edges(flow[i], flow[i + 1], dfg);
        }
        
        // Sleep to give hardware time to finish any in-progress operations
        usleep(10000); // 10ms
        
        // Safely free buffers
        for (auto& buffer_pair : buffers) {
            if (buffer_pair.second) {
                delete buffer_pair.second;
            }
        }
        
        // Clear all maps
        node_capabilities.clear();
        buffer_capabilities.clear();
        connection_capabilities.clear();
        nodes.clear();
        node_map.clear();
        buffers.clear();
        
        // Release DFG resources last
        if (dfg) {
            try {
                dfg::release_resources(dfg);
            } catch (const std::exception& e) {
                std::cerr << "Exception during DFG resource release: " << e.what() << std::endl;
            }
            dfg = nullptr;
        }
        
        dfg_root_capability = nullptr;
        if (debug_level > 0) {
            std::cout << "Pipeline resources released successfully" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Critical error during pipeline release: " << e.what() << std::endl;
    }
    
    // Reset the releasing flag to allow future releases (if object is recreated)
    already_releasing.store(false);
}

// -------------------- Helper Functions --------------------

// Add a node to the pipeline definition
void add_node(pipeline_def_t& def, const std::string& name, const std::string& type) {
    if (name.empty()) {
        std::cerr << "Error: Empty node name in add_node" << std::endl;
        return;
    }

    // Check for duplicate node
    for (const auto& node : def.nodes) {
        if (node.name == name) {
            std::cerr << "Warning: Node '" << name << "' already exists in pipeline definition" << std::endl;
            return;
        }
    }

    node_def_t node;
    node.name = name;
    node.type = type;
    def.nodes.push_back(node);
    def.num_nodes = def.nodes.size();
}

// Add a buffer to the pipeline definition
void add_buffer(pipeline_def_t& def, const std::string& name, size_t size, BufferMode mode) {
    if (name.empty()) {
        std::cerr << "Error: Empty buffer name in add_buffer" << std::endl;
        return;
    }

    if (size == 0) {
        std::cerr << "Error: Zero buffer size in add_buffer for " << name << std::endl;
        return;
    }

    // Check for duplicate buffer
    for (const auto& buffer : def.buffers) {
        if (buffer.name == name) {
            std::cerr << "Warning: Buffer '" << name << "' already exists in pipeline definition" << std::endl;
            return;
        }
    }

    buffer_def_t buffer;
    buffer.name = name;
    buffer.size = size;
    buffer.mode = mode;
    def.buffers.push_back(buffer);
    def.num_buffers = def.buffers.size();
}

// Add a component to the flow
void add_to_flow(pipeline_def_t& def, const std::string& component_name) {
    if (component_name.empty()) {
        std::cerr << "Error: Empty component name in add_to_flow" << std::endl;
        return;
    }

    // Check if component exists as node or buffer
    bool exists = false;

    for (const auto& node : def.nodes) {
        if (node.name == component_name) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        for (const auto& buffer : def.buffers) {
            if (buffer.name == component_name) {
                exists = true;
                break;
            }
        }
    }

    if (!exists) {
        std::cerr << "Warning: Component '" << component_name 
                << "' does not exist as node or buffer in pipeline definition" << std::endl;
    }

    def.flow.push_back(component_name);
    def.num_flow = def.flow.size();
}

// -------------------- Factory Functions --------------------

// Create a pipeline from a definition
Pipeline* create_pipeline(const pipeline_def_t& def) {
    if (def.name.empty()) {
        std::cerr << "Error: Empty pipeline name in create_pipeline" << std::endl;
        return nullptr;
    }

    if (def.nodes.empty()) {
        std::cerr << "Error: No nodes defined in create_pipeline for " << def.name << std::endl;
        return nullptr;
    }

    if (def.buffers.empty()) {
        std::cerr << "Error: No buffers defined in create_pipeline for " << def.name << std::endl;
        return nullptr;
    }

    if (def.flow.empty()) {
        std::cerr << "Error: No flow defined in create_pipeline for " << def.name << std::endl;
        return nullptr;
    }

    try {
        return new Pipeline(def);
    } catch (const std::exception& e) {
        std::cerr << "Exception during pipeline creation: " << e.what() << std::endl;
        return nullptr;
    }
}

// Write data to a pipeline buffer
void write_buffer_data(Buffer* buffer, void* data, size_t size) {
    if (!buffer) {
        std::cerr << "Error: Null buffer in write_buffer_data" << std::endl;
        return;
    }

    if (!buffer->get_raw_buffer()) {
        std::cerr << "Error: Null raw buffer in write_buffer_data for " << buffer->get_name() << std::endl;
        return;
    }

    if (!data) {
        std::cerr << "Error: Null data in write_buffer_data for " << buffer->get_name() << std::endl;
        return;
    }

    if (size == 0 || size > buffer->get_size()) {
        std::cerr << "Error: Invalid size " << size << " in write_buffer_data for " 
                << buffer->get_name() << " (buffer size: " << buffer->get_size() << ")" << std::endl;
        return;
    }

    // Use dfg::write_buffer from original code
    dfg::write_buffer(buffer->get_raw_buffer(), data, size);
}

// Read data from a pipeline buffer
void read_buffer_data(Buffer* buffer, void* data, size_t size) {
    if (!buffer) {
        std::cerr << "Error: Null buffer in read_buffer_data" << std::endl;
        return;
    }

    if (!buffer->get_raw_buffer()) {
        std::cerr << "Error: Null raw buffer in read_buffer_data for " << buffer->get_name() << std::endl;
        return;
    }

    if (!data) {
        std::cerr << "Error: Null destination data in read_buffer_data for " << buffer->get_name() << std::endl;
        return;
    }

    if (size == 0 || size > buffer->get_size()) {
        std::cerr << "Error: Invalid size " << size << " in read_buffer_data for " 
                << buffer->get_name() << " (buffer size: " << buffer->get_size() << ")" << std::endl;
        return;
    }

    // Get the raw buffer memory
    void* buffer_memory = dfg::read_buffer(buffer->get_raw_buffer());
    if (buffer_memory) {
        memcpy(data, buffer_memory, size);
    }
}

// Get a buffer from the pipeline by name
Buffer* get_pipeline_buffer(Pipeline* pipeline, const std::string& buffer_name) {
    if (!pipeline) {
        std::cerr << "Error: Null pipeline in get_pipeline_buffer" << std::endl;
        return nullptr;
    }

    if (buffer_name.empty()) {
        std::cerr << "Error: Empty buffer name in get_pipeline_buffer" << std::endl;
        return nullptr;
    }

    return pipeline->get_buffer(buffer_name);
}

// Get a buffer capability from the pipeline
dfg::Capability* get_pipeline_buffer_capability(Pipeline* pipeline, const std::string& buffer_name) {
    if (!pipeline) {
        std::cerr << "Error: Null pipeline in get_pipeline_buffer_capability" << std::endl;
        return nullptr;
    }

    if (buffer_name.empty()) {
        std::cerr << "Error: Empty buffer name in get_pipeline_buffer_capability" << std::endl;
        return nullptr;
    }

    return pipeline->get_buffer_capability(buffer_name);
}

// Get a node from the pipeline by name
dfg::Node* get_pipeline_node(Pipeline* pipeline, const std::string& node_name) {
    if (!pipeline) {
        std::cerr << "Error: Null pipeline in get_pipeline_node" << std::endl;
        return nullptr;
    }

    if (node_name.empty()) {
        std::cerr << "Error: Empty node name in get_pipeline_node" << std::endl;
        return nullptr;
    }

    return pipeline->get_node(node_name);
}

// Get a node capability from the pipeline
dfg::Capability* get_pipeline_node_capability(Pipeline* pipeline, const std::string& node_name) {
    if (!pipeline) {
        std::cerr << "Error: Null pipeline in get_pipeline_node_capability" << std::endl;
        return nullptr;
    }

    if (node_name.empty()) {
        std::cerr << "Error: Empty node name in get_pipeline_node_capability" << std::endl;
        return nullptr;
    }

    return pipeline->get_node_capability(node_name);
}

// Execute a pipeline with current buffer size
void execute_pipeline(Pipeline* pipeline, size_t current_size = 0) {
    if (!pipeline) {
        std::cerr << "Error: Null pipeline in execute_pipeline" << std::endl;
        return;
    }

    try {
        pipeline->execute(current_size);
    } catch (const std::exception& e) {
        std::cerr << "Exception during pipeline execution: " << e.what() << std::endl;
    }
}

// Clear completion counters in a pipeline
void clear_pipeline_completed(Pipeline* pipeline) {
    if (!pipeline) {
        std::cerr << "Error: Null pipeline in clear_pipeline_completed" << std::endl;
        return;
    }

    try {
        pipeline->clear_completed();
    } catch (const std::exception& e) {
        std::cerr << "Exception during clearing pipeline completion counters: " << e.what() << std::endl;
    }
}

void release_pipeline(Pipeline* pipeline) {
    if (!pipeline) {
        std::cerr << "Warning: Null pipeline in release_pipeline" << std::endl;
        return;
    }

    try {
        // Release the pipeline resources
        pipeline->release();
        
        // Delete the pipeline object silently without triggering a second release
        delete pipeline;
    } catch (const std::exception& e) {
        std::cerr << "Exception during pipeline release: " << e.what() << std::endl;
        // Still try to delete as a last resort
        try {
            delete pipeline;
        } catch (...) {
            std::cerr << "Failed to delete pipeline after release exception" << std::endl;
        }
    }
}

// -------------------- Enhanced Capability Operations Factory Functions --------------------

/**
 * Create a capability for a pipeline component
 * @param pipeline The pipeline
 * @param component_name Name of the component (node or buffer)
 * @param cap_space_id ID for the new capability space
 * @param access Permission flags for the capability
 * @param allow_transitive_delegation Whether to grant transitive delegation permission
 * @return Pointer to the created capability or nullptr on failure
 */
dfg::Capability* create_pipeline_capability(Pipeline* pipeline, 
                                     const std::string& component_name,
                                     const std::string& cap_space_id,
                                     uint32_t access,
                                     bool allow_transitive_delegation = false) {
    if (!pipeline) {
        std::cerr << "Error: Null pipeline in create_pipeline_capability" << std::endl;
        return nullptr;
    }

    if (component_name.empty() || cap_space_id.empty()) {
        std::cerr << "Error: Empty name in create_pipeline_capability" << std::endl;
        return nullptr;
    }

    return pipeline->create_capability(component_name, cap_space_id, access, allow_transitive_delegation);
}

/**
 * Delegate a capability from one component to another
 * @param pipeline The pipeline
 * @param src_component Source component with the capability
 * @param cap_space_id ID for the new capability space
 * @param access Permission flags for the capability
 * @param allow_transitive_delegation Whether to grant transitive delegation permission
 * @return Pointer to the delegated capability or nullptr on failure
 */
dfg::Capability* delegate_pipeline_capability(Pipeline* pipeline,
                                       const std::string& src_component,
                                       const std::string& cap_space_id,
                                       uint32_t access,
                                       bool allow_transitive_delegation = false) {
    if (!pipeline) {
        std::cerr << "Error: Null pipeline in delegate_pipeline_capability" << std::endl;
        return nullptr;
    }

    if (src_component.empty() || cap_space_id.empty()) {
        std::cerr << "Error: Empty name in delegate_pipeline_capability" << std::endl;
        return nullptr;
    }

    return pipeline->delegate_capability(src_component, cap_space_id, access, allow_transitive_delegation);
}

/**
 * Check if a component has transitive delegation rights
 * @param pipeline The pipeline
 * @param component_name The component to check
 * @return True if the component can delegate delegation rights
 */
bool can_component_delegate_delegation(Pipeline* pipeline, const std::string& component_name) {
    if (!pipeline) {
        std::cerr << "Error: Null pipeline in can_component_delegate_delegation" << std::endl;
        return false;
    }

    if (component_name.empty()) {
        std::cerr << "Error: Empty component name in can_component_delegate_delegation" << std::endl;
        return false;
    }

    return pipeline->can_component_delegate_delegation(component_name);
}

/**
 * Transfer ownership of capabilities between components
 * @param pipeline The pipeline
 * @param src_component Source component
 * @param dst_component Destination component
 * @return True if successful, false otherwise
 */
bool transfer_pipeline_ownership(Pipeline* pipeline,
                           const std::string& src_component,
                           const std::string& dst_component) {
    if (!pipeline) {
        std::cerr << "Error: Null pipeline in transfer_pipeline_ownership" << std::endl;
        return false;
    }

    if (src_component.empty() || dst_component.empty()) {
        std::cerr << "Error: Empty name in transfer_pipeline_ownership" << std::endl;
        return false;
    }

    return pipeline->transfer_ownership(src_component, dst_component);
}

/**
 * Revoke a capability
 * @param pipeline The pipeline
 * @param component_name Component that owns the capability
 * @param cap_space_id ID of the capability space to revoke
 * @return True if successful, false otherwise
 */
bool revoke_pipeline_capability(Pipeline* pipeline,
                          const std::string& component_name,
                          const std::string& cap_space_id) {
    if (!pipeline) {
        std::cerr << "Error: Null pipeline in revoke_pipeline_capability" << std::endl;
        return false;
    }

    if (component_name.empty() || cap_space_id.empty()) {
        std::cerr << "Error: Empty name in revoke_pipeline_capability" << std::endl;
        return false;
    }

    return pipeline->revoke_capability(component_name, cap_space_id);
}

/**
 * Set an expiration time for a capability
 * @param pipeline The pipeline
 * @param component_name Component that owns the capability
 * @param cap_space_id ID of the capability space
 * @param timeout Timeout in seconds
 * @return True if successful, false otherwise
 */
bool expire_pipeline_capability(Pipeline* pipeline,
                          const std::string& component_name,
                          const std::string& cap_space_id,
                          uint32_t timeout) {
    if (!pipeline) {
        std::cerr << "Error: Null pipeline in expire_pipeline_capability" << std::endl;
        return false;
    }

    if (component_name.empty() || cap_space_id.empty()) {
        std::cerr << "Error: Empty name in expire_pipeline_capability" << std::endl;
        return false;
    }

    if (timeout == 0) {
        std::cerr << "Error: Zero timeout in expire_pipeline_capability" << std::endl;
        return false;
    }

    return pipeline->expire_capability(component_name, cap_space_id, timeout);
}

/**
 * Reset all capabilities for a component
 * @param pipeline The pipeline
 * @param component_name Component to reset
 * @return True if successful, false otherwise
 */
bool reset_pipeline_capabilities(Pipeline* pipeline,
                           const std::string& component_name) {
    if (!pipeline) {
        std::cerr << "Error: Null pipeline in reset_pipeline_capabilities" << std::endl;
        return false;
    }

    if (component_name.empty()) {
        std::cerr << "Error: Empty component name in reset_pipeline_capabilities" << std::endl;
        return false;
    }

    return pipeline->reset_capabilities(component_name);
}

/**
 * Print the capability tree for a pipeline
 * @param pipeline The pipeline
 */
void print_pipeline_capability_tree(Pipeline* pipeline) {
    if (!pipeline) {
        std::cerr << "Error: Null pipeline in print_pipeline_capability_tree" << std::endl;
        return;
    }

    pipeline->print_capability_tree();
}

} // namespace pipeline