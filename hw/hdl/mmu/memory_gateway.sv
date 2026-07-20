`timescale 1ns / 1ps
import lynxTypes::*;
`include "lynx_macros.svh"

module memory_gateway #(
    parameter integer N_ENDPOINTS = 1
) (
    input logic aclk,
    input logic aresetn,
    
    // Control interface
    input logic [131-1:0] ep_ctrl,
    
    // Original DMA interfaces (unfiltered inputs)
    metaIntf.s s_rd_req,
    metaIntf.s s_wr_req,
    
    // Filtered DMA interfaces (only authorized requests)
    metaIntf.m m_rd_req,
    metaIntf.m m_wr_req
);

    // Signal declarations
    endpoint_reg_t [N_ENDPOINTS-1:0] endpoint_regs;

    // Instantiate memory endpoints configuration module
    memory_endpoints #(
        .N_ENDPOINTS(N_ENDPOINTS)
    ) inst_endpoints (
        .aclk(aclk),
        .aresetn(aresetn),
        .ep_ctrl(ep_ctrl),
        .endpoint_regs(endpoint_regs)
    );

    // Signal declarations
    logic rd_access_allowed, wr_access_allowed;
    logic violation_detected;

ila_mem_gateway inst_ila_mem_gateway (
    .clk(aclk),
    .probe0(endpoint_regs[0].valid),
    .probe1(endpoint_regs[0].access_rights),
    .probe2(endpoint_regs[0].vaddr_base),
    .probe3(endpoint_regs[0].vaddr_bound),
    .probe4(s_rd_req.data.vaddr),
    .probe5(s_rd_req.data.len),
    .probe6(s_wr_req.data.vaddr),
    .probe7(s_wr_req.data.len),
    .probe8(rd_access_allowed),
    .probe9(wr_access_allowed),
    .probe10(s_rd_req.ready),
    .probe11(s_rd_req.valid),
    .probe12(s_wr_req.ready),
    .probe13(s_wr_req.valid)
);

    // ----------------------------------------------------------------------------------------
    // Security Validation - Overflow-Safe Bounds Checking
    // ----------------------------------------------------------------------------------------
    
    always_comb begin
        // Default to deny access 
        rd_access_allowed = 1'b0;
        wr_access_allowed = 1'b0;
        
        // Read request validation
        if (s_rd_req.valid && s_rd_req.data.len > 0) begin
            // Check against all endpoints 
            for (int i = 0; i < N_ENDPOINTS; i++) begin
                if (endpoint_regs[i].valid && endpoint_regs[i].access_rights[0]) begin
                    // Calculate endpoint size to detect malicious huge lengths
                    logic [VADDR_BITS-1:0] endpoint_size;
                    endpoint_size = endpoint_regs[i].vaddr_bound - endpoint_regs[i].vaddr_base + 1;
                    
                    // Security check: Length cannot exceed endpoint size (prevents underflow attack)
                    if (s_rd_req.data.len <= endpoint_size) begin
                        // Overflow-safe bounds check: both start and end must be within bounds
                        if (s_rd_req.data.vaddr >= endpoint_regs[i].vaddr_base && 
                            s_rd_req.data.vaddr <= (endpoint_regs[i].vaddr_bound - s_rd_req.data.len + 1)) begin
                            rd_access_allowed = 1'b1;
                            break; // Found valid endpoint, allow access
                        end
                    end
                    // If len > endpoint_size, skip this endpoint (implicit denial)
                end
            end
        end
        
        // Write request validation  
        if (s_wr_req.valid && s_wr_req.data.len > 0) begin
            for (int i = 0; i < N_ENDPOINTS; i++) begin
                if (endpoint_regs[i].valid && endpoint_regs[i].access_rights[1]) begin
                    // Calculate endpoint size to detect malicious huge lengths
                    logic [VADDR_BITS-1:0] endpoint_size;
                    // The following line might overflow
                    endpoint_size = endpoint_regs[i].vaddr_bound - endpoint_regs[i].vaddr_base + 1;
                    
                    // Security check: Length cannot exceed endpoint size (prevents underflow attack)
                    if (s_wr_req.data.len <= endpoint_size) begin
                        // Overflow-safe bounds check: both start and end must be within bounds
                        if (s_wr_req.data.vaddr >= endpoint_regs[i].vaddr_base && 
                            s_wr_req.data.vaddr <= (endpoint_regs[i].vaddr_bound - s_wr_req.data.len + 1)) begin
                            wr_access_allowed = 1'b1;
                            break; // Found valid endpoint, allow access
                        end
                    end
                    // If len > endpoint_size, skip this endpoint (implicit denial)
                end
            end
        end
    end

    // ----------------------------------------------------------------------------------------
    // Immediate Drop for Unauthorized Requests
    // ----------------------------------------------------------------------------------------
    
    always_comb begin
        // AUTHORIZED REQUESTS: Forward to downstream
        m_rd_req.valid = s_rd_req.valid && rd_access_allowed;
        m_rd_req.data = s_rd_req.data;
        
        m_wr_req.valid = s_wr_req.valid && wr_access_allowed;
        m_wr_req.data = s_wr_req.data;
        
        // AUTHORIZED: Wait for downstream ready
        // UNAUTHORIZED: Immediately drop (assert ready to consume/reject request)
        s_rd_req.ready = rd_access_allowed ? m_rd_req.ready : 1'b1;
        s_wr_req.ready = wr_access_allowed ? m_wr_req.ready : 1'b1;
    end

    // ----------------------------------------------------------------------------------------
    // Access Violation Signal - Auto-clearing
    // ----------------------------------------------------------------------------------------
    
    // Detect violation occurrence (combinational)
    always_comb begin
        violation_detected = 
            (s_rd_req.valid && !rd_access_allowed) ||
            (s_wr_req.valid && !wr_access_allowed);
    end
    
    logic access_violation_detected;
    logic [31:0] violation_count;

    always_ff @(posedge aclk) begin
        if (~aresetn) begin
            violation_count <= 0;
            access_violation_detected <= 1'b0;
        end
        else begin
            access_violation_detected <= violation_detected;
            if (violation_detected) begin
                violation_count <= violation_count + 1;
            end
        end
    end

endmodule