`timescale 1ns / 1ps
import lynxTypes::*;
`include "lynx_macros.svh"

module memory_endpoints #(
    parameter integer N_ENDPOINTS = 1
) (
    input logic aclk,
    input logic aresetn,
    
    // Wide control signal input - packed configuration for all endpoints
    // Format per endpoint: [valid][access_rights][bound_addr][base_addr]
    // Per endpoint: 1 + 2 + 64 + 64 = 131 bits
    // For N_ENDPOINTS: 131 * N_ENDPOINTS bits
    input logic [131-1:0] ep_ctrl,
    
    // Endpoint registers output
    output endpoint_reg_t [N_ENDPOINTS-1:0] endpoint_regs
);

    // Bit field definitions for each endpoint configuration
    localparam integer EP_BASE_ADDR_BITS = 48;
    localparam integer EP_BOUND_ADDR_BITS = 48;
    localparam integer EP_ACCESS_BITS = 2;
    localparam integer EP_VALID_BITS = 1;
    localparam integer EP_TOTAL_BITS = EP_BASE_ADDR_BITS + EP_BOUND_ADDR_BITS + EP_ACCESS_BITS + EP_VALID_BITS; // 99
    
    // Bit field offsets within each endpoint configuration
    localparam integer EP_BASE_ADDR_OFFSET = 0;
    localparam integer EP_BOUND_ADDR_OFFSET = EP_BASE_ADDR_OFFSET + EP_BASE_ADDR_BITS;
    localparam integer EP_ACCESS_OFFSET = EP_BOUND_ADDR_OFFSET + EP_BOUND_ADDR_BITS;
    localparam integer EP_VALID_OFFSET = EP_ACCESS_OFFSET + EP_ACCESS_BITS;
    
    // Extract configuration for each endpoint and update registers
    always_ff @(posedge aclk) begin
        if (~aresetn) begin
            // Initialize all endpoints as invalid
            for (int i = 0; i < N_ENDPOINTS; i++) begin
                endpoint_regs[i].valid <= 1'b0;
                endpoint_regs[i].vaddr_base <= '0;
                endpoint_regs[i].vaddr_bound <= '0;
                endpoint_regs[i].access_rights <= 2'b00;
            end
        end
        else begin
            // Update endpoint configurations from packed control signal
            for (int i = 0; i < N_ENDPOINTS; i++) begin
                // Calculate bit range for this endpoint
                
                // Extract fields for endpoint i
                endpoint_regs[i].vaddr_base <= ep_ctrl[EP_BASE_ADDR_OFFSET +: EP_BASE_ADDR_BITS];
                endpoint_regs[i].vaddr_bound <= ep_ctrl[EP_BOUND_ADDR_OFFSET +: EP_BOUND_ADDR_BITS];
                endpoint_regs[i].access_rights <= ep_ctrl[EP_ACCESS_OFFSET +: EP_ACCESS_BITS];
                endpoint_regs[i].valid <= ep_ctrl[EP_VALID_OFFSET +: EP_VALID_BITS];
                
                // VALIDATION: Check that base <= bound for valid endpoints
                if (endpoint_regs[i].valid && 
                    (endpoint_regs[i].vaddr_base > endpoint_regs[i].vaddr_bound)) begin
                    // Invalid range - disable endpoint
                    endpoint_regs[i].valid <= 1'b0;
                end
            end
        end
    end

endmodule