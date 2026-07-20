/**
  * Copyright (c) 2021, Systems Group, ETH Zurich
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *
  * 1. Redistributions of source code must retain the above copyright notice,
  * this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  * this list of conditions and the following disclaimer in the documentation
  * and/or other materials provided with the distribution.
  * 3. Neither the name of the copyright holder nor the names of its contributors
  * may be used to endorse or promote products derived from this software
  * without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

`timescale 1ns / 1ps

import lynxTypes::*;

`include "axi_macros.svh"
`include "lynx_macros.svh"

/**
 * @brief   Data switch source
 *
 * Data switch for all vFPGA streams. 
 *
 *  @param N_ID    Number of vFPGA regions
 */
// (* dont_touch = "true" *) module dtu_top #(
module dtu_top #(
    // parameter integer MUX_DATA_BITS = AXI_DATA_BITS,
    parameter integer N_ID = N_REGIONS
) (
    input  logic                                aclk,
    input  logic                                aresetn,

    // DESCRIPTORS
    input logic                                 user_rd_sq_valid,
    output  logic                               user_rd_sq_ready,
    input req_t                                 user_rd_sq_data,
    input logic                                 user_wr_sq_valid,
    output  logic                               user_wr_sq_ready,
    input req_t                                 user_wr_sq_data,

    // HOST DESCRIPTORS
    (* mark_debug = "true" *) input  logic 							      host_sq_valid,
    output logic 							      host_sq_ready,
    (* mark_debug = "true" *) input  dreq_t 							  host_sq_data,

    // BYPASS DESCRIPTORS
    (* mark_debug = "true" *) output logic                                bpss_rd_sq_valid,
    input  logic                                bpss_rd_sq_ready,
    (* mark_debug = "true" *) output req_t                                bpss_rd_sq_data,
    (* mark_debug = "true" *) output logic                                bpss_wr_sq_valid,
    input  logic                                bpss_wr_sq_ready,
    (* mark_debug = "true" *) output req_t                                bpss_wr_sq_data,


    // AXI4S HOST SINK
    input  logic[AXI_DATA_BITS-1:0]             axis_dtu_sink_tdata,
    input  logic[AXI_DATA_BITS/8-1:0]           axis_dtu_sink_tkeep,
    input  logic                                axis_dtu_sink_tlast,
    (* mark_debug = "true" *) output logic                                axis_dtu_sink_tready,
    (* mark_debug = "true" *) input  logic                                axis_dtu_sink_tvalid,
    input logic[PID_BITS-1:0]                   axis_dtu_sink_tid,

	// AXI4SR INTER SOURCE
    output logic[AXI_DATA_BITS-1:0]             axis_dtu_src_tdata,
    output logic[AXI_DATA_BITS/8-1:0]           axis_dtu_src_tkeep,
    output logic                                axis_dtu_src_tlast,
    input  logic                                axis_dtu_src_tready,
    output logic                                axis_dtu_src_tvalid,
    output logic[PID_BITS-1:0]                  axis_dtu_src_tid,

    input  logic[AXI_DATA_BITS-1:0]             axisr_ul_sink_tdata,
    input  logic[AXI_DATA_BITS/8-1:0]           axisr_ul_sink_tkeep,
    input  logic                                axisr_ul_sink_tlast,
    (* mark_debug = "true" *) output logic                                axisr_ul_sink_tready,
    (* mark_debug = "true" *) input  logic                                axisr_ul_sink_tvalid,    
    input logic[PID_BITS-1:0]                   axisr_ul_sink_tid,

	// AXI4SR INTER SOURCE
    output logic[AXI_DATA_BITS-1:0]             axisr_ul_src_tdata,
    output logic[AXI_DATA_BITS/8-1:0]           axisr_ul_src_tkeep,
    output logic                                axisr_ul_src_tlast,
    (* mark_debug = "true" *) input  logic                                axisr_ul_src_tready,
    (* mark_debug = "true" *) output logic                                axisr_ul_src_tvalid,
    output logic[PID_BITS-1:0]                  axisr_ul_src_tid, 

    input logic [7:0]                           host_route_cap,
    input logic [7:0]                           route_in,
    output logic [7:0]                          route_out,

    input logic [1:0]                           port_in,
    output logic [1:0]                          port_out
);

    // Host descriptors
    metaIntf #(.STYPE(dreq_t)) host_sq ();
    metaIntf #(.STYPE(dreq_t)) host_sq_int ();
    
    assign host_sq.valid                        = host_sq_valid;
    assign host_sq_ready                        = host_sq.ready;
    assign host_sq.data                         = host_sq_data;

    // Bypass descriptors
    metaIntf #(.STYPE(req_t)) bpss_rd_req ();
    metaIntf #(.STYPE(req_t)) bpss_wr_req ();
    metaIntf #(.STYPE(req_t)) bpss_rd_req_int ();
    metaIntf #(.STYPE(req_t)) bpss_wr_req_int ();
    
    assign bpss_rd_sq_valid                     = bpss_rd_req.valid;
    assign bpss_rd_req.ready                    = bpss_rd_sq_ready; 
    assign bpss_rd_sq_data                      = bpss_rd_req.data;

    assign bpss_wr_sq_valid                     = bpss_wr_req.valid;
    assign bpss_wr_req.ready                    = bpss_wr_sq_ready; 
    assign bpss_wr_sq_data                      = bpss_wr_req.data; 


    AXI4S axis_dtu_sink ();
    AXI4S axis_dtu_sink_int ();

    assign axis_dtu_sink.tvalid                = axis_dtu_sink_tvalid;
    assign axis_dtu_sink.tdata                 = axis_dtu_sink_tdata;
    assign axis_dtu_sink.tkeep                 = axis_dtu_sink_tkeep;
    assign axis_dtu_sink.tlast                 = axis_dtu_sink_tlast;
    assign axis_dtu_sink_tready                = axis_dtu_sink.tready;

    AXI4S axis_dtu_src ();
    AXI4S axis_dtu_src_int ();

    assign axis_dtu_src_tvalid                 = axis_dtu_src.tvalid;
    assign axis_dtu_src_tdata                  = axis_dtu_src.tdata;
    assign axis_dtu_src_tkeep                  = axis_dtu_src.tkeep;
    assign axis_dtu_src_tlast                  = axis_dtu_src.tlast;
    assign axis_dtu_src.tready                 = axis_dtu_src_tready;

    AXI4SR axis_host_send [N_STRM_AXI] ();
    AXI4SR axis_host_recv [N_STRM_AXI] ();

    `AXISR_ASSIGN(axis_host_send[0], axis_host_send_int[0]);
    `AXISR_ASSIGN(axis_host_recv_int[0], axis_host_recv[0]);

    assign axis_host_send[0].tvalid                = axisr_ul_sink_tvalid;
    assign axis_host_send[0].tdata                 = axisr_ul_sink_tdata;
    assign axis_host_send[0].tkeep                 = axisr_ul_sink_tkeep;
    assign axis_host_send[0].tlast                 = axisr_ul_sink_tlast;
    assign axisr_ul_sink_tready                = axis_host_send[0].tready;
    assign axis_host_send[0].tid                   = axisr_ul_sink_tid;
    
    assign axisr_ul_src_tvalid                 = axis_host_recv[0].tvalid;
    assign axisr_ul_src_tdata                  = axis_host_recv[0].tdata;
    assign axisr_ul_src_tkeep                  = axis_host_recv[0].tkeep;
    assign axisr_ul_src_tlast                  = axis_host_recv[0].tlast;
    assign axis_host_recv[0].tready                 = axisr_ul_src_tready;
    assign axisr_ul_src_tid                    = axis_host_recv[0].tid;

    // (* mark_debug = "true" *) logic host_sq_int_valid;
    // (* mark_debug = "true" *) logic host_sq_int_data_req_1_actv;
    // (* mark_debug = "true" *) logic host_local_sq_rd_valid;
    // (* mark_debug = "true" *) logic local_sq_rd_valid;
    // (* mark_debug = "true" *) logic local_sq_rd_host_valid;
    // (* mark_debug = "true" *) logic local_cred_rd_host_valid;
    // (* mark_debug = "true" *) logic bpss_rd_req_int_valid;

    // assign host_sq_int_valid                     = host_sq_int.valid;
    // assign host_sq_int_data_req_1_actv           = host_sq_int.data.req_1.actv;
    // assign host_local_sq_rd_valid                = host_local_sq_rd.valid;
    // assign local_sq_rd_valid                     = local_sq_rd.valid;
    // assign local_sq_rd_host_valid                = local_sq_rd_host.valid;
    // assign local_cred_rd_host_valid              = local_cred_rd_host.valid;
    // assign bpss_rd_req_int_valid                 = bpss_rd_req_int.valid;


    // 
    // ASSIGN (NO REGS)
    //

    // Host request
    `META_ASSIGN(host_sq, host_sq_int)

    // Bypass
    `META_ASSIGN(bpss_rd_req_int, bpss_rd_req)
    `META_ASSIGN(bpss_wr_req_int, bpss_wr_req)


    // AXIS host
    `AXIS_ASSIGN(axis_dtu_sink, axis_dtu_sink_int)
    `AXIS_ASSIGN(axis_dtu_src_int, axis_dtu_src)

    //
    // SQ
    //

    metaIntf #(.STYPE(req_t)) user_sq_rd_int ();
    metaIntf #(.STYPE(req_t)) user_sq_wr_int ();

    assign user_sq_rd_int.valid                     = user_rd_sq_valid;
    assign user_sq_rd_int.data                      = user_rd_sq_data;
    assign user_rd_sq_ready                         = user_sq_rd_int.ready;

    assign user_sq_wr_int.valid                     = user_wr_sq_valid;
    assign user_sq_wr_int.data                      = user_wr_sq_data;
    assign user_wr_sq_ready                         = user_sq_wr_int.ready;

    metaIntf #(.STYPE(req_t)) host_local_sq_rd ();
    metaIntf #(.STYPE(req_t)) host_local_sq_wr ();
    metaIntf #(.STYPE(req_t)) user_local_sq_rd ();
    metaIntf #(.STYPE(req_t)) user_local_sq_wr ();

    metaIntf #(.STYPE(req_t)) local_sq_rd ();
    metaIntf #(.STYPE(req_t)) local_sq_wr ();


    // Host request mux
    host_req_mux inst_host_mux (
        .host_sq(host_sq_int),
        .host_local_rd(host_local_sq_rd),
        .host_local_wr(host_local_sq_wr),
        .aclk(aclk),
        .aresetn(aresetn)
    );

    // User request mux
    user_req_mux #(
        .ID_REG(0)
    ) inst_user_mux (
        .user_sq_rd(user_sq_rd_int),
        .user_sq_wr(user_sq_wr_int),
        .user_local_rd(user_local_sq_rd),
        .user_local_wr(user_local_sq_wr),
        .aclk(aclk),
        .aresetn(aresetn)
    );

    // Host user arbiters
    req_arb_2_1 inst_arb_local_rd (
        .aclk(aclk),
        .aresetn(aresetn),
        .s_req_0(user_local_sq_rd),
        .s_req_1(host_local_sq_rd),
        .m_req(local_sq_rd)
    );

    req_arb_2_1 inst_arb_local_wr (
        .aclk(aclk),
        .aresetn(aresetn),
        .s_req_0(user_local_sq_wr),
        .s_req_1(host_local_sq_wr),
        .m_req(local_sq_wr)
    );


    //
    // Local credits
    // 
    metaIntf #(.STYPE(req_t)) local_sq_rd_host ();
    metaIntf #(.STYPE(req_t)) local_sq_rd_card ();
    metaIntf #(.STYPE(req_t)) local_sq_wr_host ();
    metaIntf #(.STYPE(req_t)) local_sq_wr_card ();

    metaIntf #(.STYPE(req_t)) local_cred_rd_host ();
    metaIntf #(.STYPE(req_t)) local_cred_rd_card ();
    metaIntf #(.STYPE(req_t)) local_cred_wr_host ();
    metaIntf #(.STYPE(req_t)) local_cred_wr_card ();

    AXI4SR axis_host_send_int [N_STRM_AXI] ();
    AXI4SR axis_host_recv_int [N_STRM_AXI] ();
    AXI4SR axis_card_send_int [N_CARD_AXI] ();
    AXI4SR axis_card_recv_int [N_CARD_AXI] (); 

    `META_ASSIGN(local_sq_rd, local_sq_rd_host)
    `META_ASSIGN(local_sq_wr, local_sq_wr_host)

    local_credits_host_rd #(
        .N_DESTS(N_STRM_AXI)
    ) inst_local_credits_host_rd (
        .aclk(aclk),
        .aresetn(aresetn),
        .s_req(local_sq_rd_host),
        .m_req(local_cred_rd_host),
        .s_axis(axis_dtu_sink_int),
        .m_axis(axis_host_recv_int)
    );

    local_credits_host_wr #(
        .N_DESTS(N_STRM_AXI)
    ) inst_local_credits_host_wr (
        .aclk(aclk),
        .aresetn(aresetn),
        .s_req(local_sq_wr_host),
        .m_req(local_cred_wr_host),
        .s_axis(axis_host_send_int),
        .m_axis(axis_dtu_src_int)
    );

    `META_ASSIGN(local_cred_rd_host, bpss_rd_req_int)
    `META_ASSIGN(local_cred_wr_host, bpss_wr_req_int)

    // Gateways
    gate_send #(
        .N_DESTS(N_STRM_AXI)
    ) inst_gate_send (
        .aclk(aclk),
        .aresetn(aresetn),
        .host_route_cap_in(host_route_cap),
        .ul_port_in(port_in),
        .route_out(route_out)
    );

    gate_recv #(
        .N_DESTS(N_STRM_AXI)
    ) inst_gate_recv (
        .aclk(aclk),
        .aresetn(aresetn),
        .host_route_cap_in(host_route_cap),
        .route_in(route_in),
        .ul_port_out(port_out)
    );

    //
    // Remote credits
    // 


    //
    // CQ
    // 

// create_ip -name ila -vendor xilinx.com -library ip -version 6.2 -module_name ila_dtu
// set_property -dict [list CONFIG.C_NUM_OF_PROBES {12} CONFIG.C_EN_STRG_QUAL {1} CONFIG.Component_Name {ila_dtu} CONFIG.ALL_PROBE_SAME_MU_CNT {2} ] [get_ips ila_dtu]

// ila_dtu inst_ila_dtu (
//     .clk(aclk),
//     .probe0(host_sq_int.valid),
//     .probe1(host_sq_int.data.req_1.actv),
//     .probe2(host_local_sq_rd.valid),
//     .probe3(local_sq_rd.valid),
//     .probe4(local_sq_rd_host.valid),
//     .probe5(local_cred_rd_host.valid),
//     .probe6(bpss_rd_req_int.valid),
//     .probe7(host_sq_int.data.req_2.actv),
//     .probe8(host_local_sq_wr.valid),
//     .probe9(local_sq_wr.valid),
//     .probe10(local_sq_wr_host.valid),
//     .probe11(bpss_wr_req_int.valid)
// );

endmodule