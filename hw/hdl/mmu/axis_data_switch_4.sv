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
module axis_data_switch_4 #(
    // parameter integer MUX_DATA_BITS = AXI_DATA_BITS,
    parameter integer N_ID = N_REGIONS
) (
    input  logic                            aclk,
    input  logic                            aresetn,

    // IO control
    input logic [N_REGIONS-1:0][7:0]        io_ctrl,

    // // data input from shell to switch
    // AXI4S.s                                 data_shell_in [N_ID],
    // // data output from switch to shell
    // AXI4S.m                                 data_shell_out [N_ID],

    // data input from user logic to switch
    AXI4SR.s                                 data_ul_out [N_ID],
    // data output from switch to user logic
    AXI4SR.m                                 data_ul_in [N_ID]

);

    // `AXISR_ASSIGN(data_ul_out[0], data_ul_in[1])
    // `AXISR_ASSIGN(data_ul_out[1], data_ul_in[0])

// -- Constants
localparam integer BEAT_LOG_BITS = $clog2(AXI_DATA_BITS/8);
localparam integer BLEN_BITS = LEN_BITS - BEAT_LOG_BITS;
localparam integer N_ID_BITS = clog2s(N_ID);

// -- FSM
typedef enum logic[0:0]  {ST_IDLE, ST_MUX} state_t;
logic [0:0] state_C, state_N;

// -- Internal regs
logic [N_ID_BITS-1:0] vfid_C, vfid_N;
logic [BLEN_BITS-1:0] cnt_C, cnt_N;
logic last_C, last_N;

// -- Internal signals
logic tr_done;

// For axi stream switch decode error
logic [3:0] axis_switch_0_s_decode_err;
logic [N_ID-1:0][1:0] axis_switch_0_m_tdest;


// ----------------------------------------------------------------------------------------------------------------------- 
// -- Mux 
// ----------------------------------------------------------------------------------------------------------------------- 
// -- interface loop issues => temp signals

(* mark_debug = "true" *) logic [N_ID-1:0]                        data_ul_out_tvalid;
logic [N_ID-1:0]                        data_ul_out_tready;
logic [N_ID-1:0][AXI_DATA_BITS-1:0]     data_ul_out_tdata;
logic [N_ID-1:0][AXI_DATA_BITS/8-1:0]   data_ul_out_tkeep;
logic [N_ID-1:0]                        data_ul_out_tlast;
logic [N_ID-1:0][PID_BITS-1:0]                        data_ul_out_tid;

(* mark_debug = "true" *) logic [N_ID-1:0]                        data_ul_in_tvalid;
logic [N_ID-1:0]                        data_ul_in_tready;
logic [N_ID-1:0][AXI_DATA_BITS-1:0]     data_ul_in_tdata;
logic [N_ID-1:0][AXI_DATA_BITS/8-1:0]   data_ul_in_tkeep;
logic [N_ID-1:0]                        data_ul_in_tlast;
logic [N_ID-1:0][PID_BITS-1:0]                        data_ul_in_tid;


for(genvar i = 0; i < N_ID; i++) begin
    assign data_ul_out_tvalid[i] = data_ul_out[i].tvalid;
    assign data_ul_out_tdata[i] = data_ul_out[i].tdata;
    assign data_ul_out_tkeep[i] = data_ul_out[i].tkeep;
    assign data_ul_out_tlast[i] = data_ul_out[i].tlast;
    assign data_ul_out_tid[i] = data_ul_out[i].tid;
    assign data_ul_out[i].tready = data_ul_out_tready[i];
end

for(genvar i = 0; i < N_ID; i++) begin
    assign data_ul_in[i].tvalid = data_ul_in_tvalid[i];
    assign data_ul_in[i].tdata = data_ul_in_tdata[i];
    assign data_ul_in[i].tkeep = data_ul_in_tkeep[i];
    assign data_ul_in[i].tlast = data_ul_in_tlast[i];
    assign data_ul_in[i].tid = data_ul_in_tid[i];
    assign data_ul_in_tready[i] = data_ul_in[i].tready;
end


axis_switch_4_0 inst_axis_switch_0 (
    .aclk(aclk),
    .aresetn(aresetn),
    .m_axis_tdata({data_ul_in_tdata[3], data_ul_in_tdata[2], data_ul_in_tdata[1], data_ul_in_tdata[0]}),
    .m_axis_tdest({axis_switch_0_m_tdest[3], axis_switch_0_m_tdest[2], axis_switch_0_m_tdest[1], axis_switch_0_m_tdest[0]}),
    .m_axis_tready({data_ul_in_tready[3], data_ul_in_tready[2], data_ul_in_tready[1], data_ul_in_tready[0]}),
    .m_axis_tvalid({data_ul_in_tvalid[3], data_ul_in_tvalid[2], data_ul_in_tvalid[1], data_ul_in_tvalid[0]}),
    .m_axis_tid({data_ul_in_tid[3], data_ul_in_tid[2], data_ul_in_tid[1], data_ul_in_tid[0]}),
    .s_axis_tdata({data_ul_out_tdata[3], data_ul_out_tdata[2], data_ul_out_tdata[1], data_ul_out_tdata[0]}),
    .s_axis_tdest({io_ctrl[3][1:0], io_ctrl[2][1:0], io_ctrl[1][1:0], io_ctrl[0][1:0]}),
    .s_axis_tready({data_ul_out_tready[3], data_ul_out_tready[2], data_ul_out_tready[1], data_ul_out_tready[0]}),
    .s_axis_tvalid({data_ul_out_tvalid[3], data_ul_out_tvalid[2], data_ul_out_tvalid[1], data_ul_out_tvalid[0]}),
    .s_axis_tid({data_ul_out_tid[3], data_ul_out_tid[2], data_ul_out_tid[1], data_ul_out_tid[0]}),
    .s_decode_err(axis_switch_0_s_decode_err)
);


ila_switch_4 inst_ila_switch (
    .clk(aclk),
    .probe0(data_ul_in[0].tvalid),
    .probe1(data_ul_out[0].tvalid),
    .probe2(data_ul_in[1].tvalid),
    .probe3(data_ul_out[1].tvalid),
    .probe4(data_ul_in[0].tready),
    .probe5(data_ul_out[0].tready),
    .probe6(data_ul_in[1].tready),
    .probe7(data_ul_out[1].tready),
    .probe8(data_ul_in[2].tvalid),
    .probe9(data_ul_out[2].tvalid),
    .probe10(data_ul_in[3].tvalid),
    .probe11(data_ul_out[3].tvalid),
    .probe12(data_ul_in[2].tready),
    .probe13(data_ul_out[2].tready),
    .probe14(data_ul_in[3].tready),
    .probe15(data_ul_out[3].tready),
    .probe16(io_ctrl[0]),
    .probe17(io_ctrl[1]),
    .probe18(io_ctrl[2]),
    .probe19(io_ctrl[3])
);

// ila_interconnect_config inst_ila_interconnect_config (
//     .clk(aclk),
//     .probe0(io_ctrl[0]),
//     .probe1(io_ctrl[1])
// );


endmodule