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
// include "axi_macros.svh"

/**
 * @brief   Data switch source
 *
 * Data switch for all vFPGA streams. 
 *
 *  @param N_ID    Number of vFPGA regions
 */
module axis_data_switch_dtu_2 #(
    // parameter integer MUX_DATA_BITS = AXI_DATA_BITS,
    parameter integer N_ID = N_REGIONS
) (
    input  logic                             aclk,
    input  logic                             aresetn,

    // IO control
    (* mark_debug = "true" *) input logic [N_REGIONS-1:0][7:0]         route_in,
    output logic [N_REGIONS-1:0][7:0]        route_out,


    // // data input from shell to switch
    AXI4S.s                                 data_host_sink [N_ID],
    // data output from switch to shell
    AXI4S.m                                 data_host_src [N_ID],

    // data input from user logic to switch
    AXI4SR.s                                 data_dtu_sink [N_ID],
    // data output from switch to user logic
    AXI4SR.m                                 data_dtu_src [N_ID]

);

// For axi stream switch decode error
logic [3:0] axis_switch_0_s_decode_err;
logic [N_ID-1:0][7:0] axis_switch_0_m_tdest;

// ----------------------------------------------------------------------------------------------------------------------- 
// -- Mux 
// ----------------------------------------------------------------------------------------------------------------------- 
// -- interface loop issues => temp signals

(* mark_debug = "true" *) logic [N_ID-1:0]                        data_host_sink_tvalid;
(* mark_debug = "true" *) logic [N_ID-1:0]                        data_host_sink_tready;
logic [N_ID-1:0][AXI_DATA_BITS-1:0]     data_host_sink_tdata;
logic [N_ID-1:0][AXI_DATA_BITS/8-1:0]   data_host_sink_tkeep;
logic [N_ID-1:0]                        data_host_sink_tlast;
logic [N_ID-1:0][PID_BITS-1:0]                        data_host_sink_tid;

(* mark_debug = "true" *) logic [N_ID-1:0]                        data_host_src_tvalid;
(* mark_debug = "true" *) logic [N_ID-1:0]                        data_host_src_tready;
logic [N_ID-1:0][AXI_DATA_BITS-1:0]     data_host_src_tdata;
logic [N_ID-1:0][AXI_DATA_BITS/8-1:0]   data_host_src_tkeep;
logic [N_ID-1:0]                        data_host_src_tlast;
logic [N_ID-1:0][PID_BITS-1:0]                        data_host_src_tid;


for(genvar i = 0; i < N_ID; i++) begin
    assign data_host_sink_tvalid[i] = data_host_sink[i].tvalid;
    assign data_host_sink_tdata[i] = data_host_sink[i].tdata;
    assign data_host_sink_tkeep[i] = data_host_sink[i].tkeep;
    assign data_host_sink_tlast[i] = data_host_sink[i].tlast;
    assign data_host_sink_tid[i] = 0;
    assign data_host_sink[i].tready = data_host_sink_tready[i];
end

for(genvar i = 0; i < N_ID; i++) begin
    assign data_host_src[i].tvalid = data_host_src_tvalid[i];
    assign data_host_src[i].tdata = data_host_src_tdata[i];
    assign data_host_src[i].tkeep = data_host_src_tkeep[i];
    assign data_host_src[i].tlast = data_host_src_tlast[i];
    // assign data_host_src[i].tid = data_host_src_tid[i];
    assign data_host_src_tready[i] = data_host_src[i].tready;
end


(* mark_debug = "true" *) logic [N_ID-1:0]                        data_dtu_sink_tvalid;
(* mark_debug = "true" *) logic [N_ID-1:0]                        data_dtu_sink_tready;
logic [N_ID-1:0][AXI_DATA_BITS-1:0]     data_dtu_sink_tdata;
logic [N_ID-1:0][AXI_DATA_BITS/8-1:0]   data_dtu_sink_tkeep;
logic [N_ID-1:0]                        data_dtu_sink_tlast;
logic [N_ID-1:0][PID_BITS-1:0]                        data_dtu_sink_tid;

(* mark_debug = "true" *) logic [N_ID-1:0]                        data_dtu_src_tvalid;
(* mark_debug = "true" *) logic [N_ID-1:0]                        data_dtu_src_tready;
logic [N_ID-1:0][AXI_DATA_BITS-1:0]     data_dtu_src_tdata;
logic [N_ID-1:0][AXI_DATA_BITS/8-1:0]   data_dtu_src_tkeep;
logic [N_ID-1:0]                        data_dtu_src_tlast;
logic [N_ID-1:0][PID_BITS-1:0]                        data_dtu_src_tid;

// for(genvar i = 0; i < N_ID; i++) begin
//     assign route_out[i] = route_in[i];
// end



for(genvar i = 0; i < N_ID; i++) begin
    assign data_dtu_sink_tvalid[i] = data_dtu_sink[i].tvalid;
    assign data_dtu_sink_tdata[i] = data_dtu_sink[i].tdata;
    assign data_dtu_sink_tkeep[i] = data_dtu_sink[i].tkeep;
    assign data_dtu_sink_tlast[i] = data_dtu_sink[i].tlast;
    assign data_dtu_sink_tid[i] = data_dtu_sink[i].tid;
    assign data_dtu_sink[i].tready = data_dtu_sink_tready[i];
end

for(genvar i = 0; i < N_ID; i++) begin
    assign data_dtu_src[i].tvalid = data_dtu_src_tvalid[i];
    assign data_dtu_src[i].tdata = data_dtu_src_tdata[i];
    assign data_dtu_src[i].tkeep = data_dtu_src_tkeep[i];
    assign data_dtu_src[i].tlast = data_dtu_src_tlast[i];
    assign data_dtu_src[i].tid = data_dtu_src_tid[i];
    assign data_dtu_src_tready[i] = data_dtu_src[i].tready;
end




axis_switch_4_0 inst_axis_switch_0 (
    .aclk(aclk),
    .aresetn(aresetn),
    .m_axis_tdata({data_dtu_src_tdata, data_host_src_tdata}),
    .m_axis_tdest({route_out, axis_switch_0_m_tdest}),
    .m_axis_tready({data_dtu_src_tready, data_host_src_tready}),
    .m_axis_tvalid({data_dtu_src_tvalid, data_host_src_tvalid}),
    .m_axis_tlast({data_dtu_src_tlast, data_host_src_tlast}),
    .m_axis_tid({data_dtu_src_tid, data_host_src_tid}),
    .s_axis_tdata({data_dtu_sink_tdata, data_host_sink_tdata}),
    .s_axis_tdest({route_in[1], route_in[0], 8'b01111100, 8'b01011100}),
    .s_axis_tready({data_dtu_sink_tready, data_host_sink_tready}),
    .s_axis_tvalid({data_dtu_sink_tvalid, data_host_sink_tvalid}),
    .s_axis_tlast({data_dtu_sink_tlast, data_host_sink_tlast}),
    .s_axis_tid({data_dtu_sink_tid, data_host_sink_tid}),
    .s_decode_err(axis_switch_0_s_decode_err)
);

ila_switch_4 inst_ila_switch (
    .clk(aclk),
    .probe0(data_host_sink[0].tvalid),
    .probe1(data_host_src[0].tvalid),
    .probe2(data_host_sink[1].tvalid),
    .probe3(data_host_src[1].tvalid),
    .probe4(data_host_sink[0].tready),
    .probe5(data_host_src[0].tready),
    .probe6(data_host_sink[1].tready),
    .probe7(data_host_src[1].tready),
    .probe8(data_host_sink[0].tlast),
    .probe9(data_host_src[0].tlast),
    .probe10(data_host_sink[1].tlast),
    .probe11(data_host_src[1].tlast),
    .probe12(data_dtu_sink[0].tvalid),
    .probe13(data_dtu_src[0].tvalid),
    .probe14(data_dtu_sink[1].tvalid),
    .probe15(data_dtu_src[1].tvalid),
    .probe16(data_dtu_sink[0].tready),
    .probe17(data_dtu_src[0].tready),
    .probe18(data_dtu_sink[1].tready),
    .probe19(data_dtu_src[1].tready),
    .probe20(data_dtu_sink[0].tlast),
    .probe21(data_dtu_src[0].tlast),
    .probe22(data_dtu_sink[1].tlast),
    .probe23(data_dtu_src[1].tlast),
    .probe24(route_in[0]),
    .probe25(route_in[1]),
    .probe26(route_in[0]),
    .probe27(route_in[1])
);

// ila_interconnect_config inst_ila_interconnect_config (
//     .clk(aclk),
//     .probe0(route_in[0]),
//     .probe1(route_in[1])
// );


endmodule