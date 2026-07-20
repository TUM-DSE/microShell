`timescale 1ns / 1ps
import lynxTypes::*;
`include "axi_macros.svh"
/**
 * Modular RSA
 * 
 */
module r_top (
    AXI4SR.s                                axis_sink,
    AXI4SR.m                                axis_src,

    input  logic                            aclk,
    input  logic                            aresetn
);

logic        axis_host_recv_tvalid;
logic        axis_host_recv_tready;
logic [255:0] axis_host_recv_tdata;
logic [31:0]  axis_host_recv_tkeep;
logic [15:0] axis_host_recv_tid;
logic        axis_host_recv_tlast;

logic        axis_host_send_tvalid;
logic        axis_host_send_tready;
logic [255:0] axis_host_send_tdata;
logic [31:0]  axis_host_send_tkeep;
logic [15:0] axis_host_send_tid;
logic        axis_host_send_tlast;
        
always_comb begin
    axis_host_recv_tkeep = 1;
    axis_host_send_tkeep = 1;
    axis_host_recv_tid = 0;
    axis_host_send_tid = 0;
end	
    
// Input width converter (512->256 bits)
dwidth_converter_512_256 inst_dwidth_recv (
    .aclk(aclk),
    .aresetn(aresetn),
    .s_axis_tvalid(axis_sink.tvalid),
    .s_axis_tready(axis_sink.tready),
    .s_axis_tdata(axis_sink.tdata),
    .s_axis_tlast(axis_sink.tlast),
    .s_axis_tid(axis_sink.tid),
    .m_axis_tvalid(axis_host_recv_tvalid),
    .m_axis_tready(axis_host_recv_tready),
    .m_axis_tdata(axis_host_recv_tdata),
    .m_axis_tlast(axis_host_recv_tlast),
    .m_axis_tid(axis_host_recv_tid)
);

// Signal declarations for RSA
logic [255:0] rsa_modulus;
logic [19:0]  rsa_exponent;

// RSA instance
rsa inst_rsa (
    .ap_clk(aclk),
    .ap_rst_n(aresetn),
    
    // Message input stream (256-bit)
    .message_TVALID(axis_host_recv_tvalid),
    .message_TREADY(axis_host_recv_tready),
    .message_TDATA(axis_host_recv_tdata),
    .message_TLAST(axis_host_recv_tlast),
    
    // Static inputs
    .modulus(rsa_modulus),
    .exponent(rsa_exponent),
    
    // Result output stream (256-bit)
    .result_TVALID(axis_host_send_tvalid),
    .result_TREADY(axis_host_send_tready),
    .result_TDATA(axis_host_send_tdata),
    .result_TLAST(axis_host_send_tlast)
);

// Default assignments for RSA keys
assign rsa_modulus = 256'hF4F5E3D2C1B0A9876543210FEDCBA9876543210FEDCBA9876543210FEDCBA98;
assign rsa_exponent = 20'h10001;  

// Output width converter (256->512 bits)
dwidth_converter_256_512 inst_dwidth_send (
    .aclk(aclk),
    .aresetn(aresetn),
    .s_axis_tvalid(axis_host_send_tvalid),
    .s_axis_tready(axis_host_send_tready),
    .s_axis_tdata(axis_host_send_tdata),
    .s_axis_tlast(axis_host_send_tlast),
    .s_axis_tid(axis_host_send_tid),
    .m_axis_tvalid(axis_src.tvalid),
    .m_axis_tready(axis_src.tready),
    .m_axis_tdata(axis_src.tdata),
    .m_axis_tlast(axis_src.tlast),
    .m_axis_tid(axis_src.tid)
);

endmodule