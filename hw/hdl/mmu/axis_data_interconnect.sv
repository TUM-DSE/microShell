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
 * @brief   Data interconnect source
 *
 * Data interconnect for all vFPGA streams. 
 *
 *  @param N_ID    Number of vFPGA regions
 */
module axis_data_interconnect #(
    // parameter integer MUX_DATA_BITS = AXI_DATA_BITS,
    parameter integer N_ID = N_REGIONS
) (
    input  logic                            aclk,
    input  logic                            aresetn,

    // IO control
    input logic [N_REGIONS-1:0][7:0]        io_ctrl,

    // data input from shell to interconnect
    AXI4S.s                                 data_shell_in [N_ID],
    // data output from interconnect to shell
    AXI4S.m                                 data_shell_out [N_ID],

    // data input from user logic to interconnect
    AXI4S.s                                 data_ul_out [N_ID],
    // data output from interconnect to user logic
    AXI4S.m                                 data_ul_in [N_ID]

);

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

// ----------------------------------------------------------------------------------------------------------------------- 
// -- Mux 
// ----------------------------------------------------------------------------------------------------------------------- 
// -- interface loop issues => temp signals
(* mark_debug = "true" *) logic [N_ID-1:0]                        data_shell_in_tvalid;
logic [N_ID-1:0]                        data_shell_in_tready;
logic [N_ID-1:0][AXI_DATA_BITS-1:0]     data_shell_in_tdata;
logic [N_ID-1:0][AXI_DATA_BITS/8-1:0]   data_shell_in_tkeep;
logic [N_ID-1:0]                        data_shell_in_tlast;

(* mark_debug = "true" *) logic [N_ID-1:0]                        data_shell_out_tvalid;
logic [N_ID-1:0]                        data_shell_out_tready;
logic [N_ID-1:0][AXI_DATA_BITS-1:0]     data_shell_out_tdata;
logic [N_ID-1:0][AXI_DATA_BITS/8-1:0]   data_shell_out_tkeep;
logic [N_ID-1:0]                        data_shell_out_tlast;


(* mark_debug = "true" *) logic [N_ID-1:0]                        data_ul_out_tvalid;
logic [N_ID-1:0]                        data_ul_out_tready;
logic [N_ID-1:0][AXI_DATA_BITS-1:0]     data_ul_out_tdata;
logic [N_ID-1:0][AXI_DATA_BITS/8-1:0]   data_ul_out_tkeep;
logic [N_ID-1:0]                        data_ul_out_tlast;

(* mark_debug = "true" *) logic [N_ID-1:0]                        data_ul_in_tvalid;
logic [N_ID-1:0]                        data_ul_in_tready;
logic [N_ID-1:0][AXI_DATA_BITS-1:0]     data_ul_in_tdata;
logic [N_ID-1:0][AXI_DATA_BITS/8-1:0]   data_ul_in_tkeep;
logic [N_ID-1:0]                        data_ul_in_tlast;


for(genvar i = 0; i < N_ID; i++) begin
    assign data_shell_in_tvalid[i] = data_shell_in[i].tvalid;
    assign data_shell_in_tdata[i] = data_shell_in[i].tdata;
    assign data_shell_in_tkeep[i] = data_shell_in[i].tkeep;
    assign data_shell_in_tlast[i] = data_shell_in[i].tlast;
    assign data_shell_in[i].tready = data_shell_in_tready[i];
end


for(genvar i = 0; i < N_ID; i++) begin
    assign data_shell_out[i].tvalid = data_shell_out_tvalid[i];
    assign data_shell_out[i].tdata = data_shell_out_tdata[i];
    assign data_shell_out[i].tkeep = data_shell_out_tkeep[i];
    assign data_shell_out[i].tlast = data_shell_out_tlast[i];
    assign data_shell_out_tready[i] = data_shell_out[i].tready;
end


for(genvar i = 0; i < N_ID; i++) begin
    assign data_ul_out_tvalid[i] = data_ul_out[i].tvalid;
    assign data_ul_out_tdata[i] = data_ul_out[i].tdata;
    assign data_ul_out_tkeep[i] = data_ul_out[i].tkeep;
    assign data_ul_out_tlast[i] = data_ul_out[i].tlast;
    assign data_ul_out[i].tready = data_ul_out_tready[i];
end

for(genvar i = 0; i < N_ID; i++) begin
    assign data_ul_in[i].tvalid = data_ul_in_tvalid[i];
    assign data_ul_in[i].tdata = data_ul_in_tdata[i];
    assign data_ul_in[i].tkeep = data_ul_in_tkeep[i];
    assign data_ul_in[i].tlast = data_ul_in_tlast[i];
    assign data_ul_in_tready[i] = data_ul_in[i].tready;
end


// -- Mux
always_comb begin

    // for(int i = 2; i < N_ID; i++) begin
    // // for(int i = 0; i < N_ID; i++) begin
    //     data_ul_in_tvalid[i] = data_shell_in_tvalid[i];
    //     data_ul_in_tdata[i] = data_shell_in_tdata[i];
    //     data_ul_in_tkeep[i] = data_shell_in_tkeep[i];
    //     data_ul_in_tlast[i] = data_shell_in_tlast[i];
    //     data_shell_in_tready[i] = data_ul_in_tready[i];
    // end

    // for(int i = 2; i < N_ID; i++) begin
    // // for(int i = 0; i < N_ID; i++) begin
    //     data_shell_out_tvalid[i] = data_ul_out_tvalid[i];
    //     data_shell_out_tdata[i] = data_ul_out_tdata[i];
    //     data_shell_out_tkeep[i] = data_ul_out_tkeep[i];
    //     data_shell_out_tlast[i] = data_ul_out_tlast[i];
    //     data_ul_out_tready[i] = data_shell_out_tready[i];
    // end

    // data_ul_in_tvalid[1] = data_ul_out_tvalid[0];
    // data_ul_in_tdata[1] = data_ul_out_tdata[0];
    // data_ul_in_tkeep[1] = data_ul_out_tkeep[0];
    // data_ul_in_tlast[1] = data_ul_out_tlast[0];
    // data_ul_out_tready[0] = data_ul_in_tready[1];

    // data_shell_out_tvalid[0] = data_ul_out_tvalid[1];
    // data_shell_out_tdata[0] = data_ul_out_tdata[1];
    // data_shell_out_tkeep[0] = data_ul_out_tkeep[1];
    // data_shell_out_tlast[0] = data_ul_out_tlast[1];
    // data_ul_out_tready[1] = data_shell_out_tready[0];

    case(io_ctrl[0]) 
        8'h01:begin
            // AXIS_ASSIGN(data_shell_in[0], data_ul_in[0]);
            // AXIS_ASSIGN(data_shell_in[1], data_ul_in[1]);
            // AXIS_ASSIGN(data_ul_out[0], data_shell_out[0]);
            // AXIS_ASSIGN(data_ul_out[1], data_shell_out[1]);

            data_ul_in_tvalid[0] = data_shell_in_tvalid[0];
            data_ul_in_tdata[0] = data_shell_in_tdata[0];
            data_ul_in_tkeep[0] = data_shell_in_tkeep[0];
            data_ul_in_tlast[0] = data_shell_in_tlast[0];
            data_shell_in_tready[0] = data_ul_in_tready[0];

            data_ul_in_tvalid[1] = data_shell_in_tvalid[1];
            data_ul_in_tdata[1] = data_shell_in_tdata[1];
            data_ul_in_tkeep[1] = data_shell_in_tkeep[1];
            data_ul_in_tlast[1] = data_shell_in_tlast[1];
            data_shell_in_tready[1] = data_ul_in_tready[1];

            data_shell_out_tvalid[0] = data_ul_out_tvalid[0];
            data_shell_out_tdata[0] = data_ul_out_tdata[0];
            data_shell_out_tkeep[0] = data_ul_out_tkeep[0];
            data_shell_out_tlast[0] = data_ul_out_tlast[0];
            data_ul_out_tready[0] = data_shell_out_tready[0];

            data_shell_out_tvalid[1] = data_ul_out_tvalid[1];
            data_shell_out_tdata[1] = data_ul_out_tdata[1];
            data_shell_out_tkeep[1] = data_ul_out_tkeep[1];
            data_shell_out_tlast[1] = data_ul_out_tlast[1];
            data_ul_out_tready[1] = data_shell_out_tready[1];
        end
        
    //     // 8'h02:begin
           
    //     //     data_ul_in_tvalid[1] = data_ul_out_tvalid[0];
    //     //     data_ul_in_tdata[1] = data_ul_out_tdata[0];
    //     //     data_ul_in_tkeep[1] = data_ul_out_tkeep[0];
    //     //     data_ul_in_tlast[1] = data_ul_out_tlast[0];
    //     //     data_ul_out_tready[0] = data_ul_in_tready[1];
    //     //     // data_ul_out_tready[0] = 1'b1;

    //     //     // data_shell_out_tvalid[0] = data_ul_out_tvalid[0];
    //     //     // data_shell_out_tdata[0] = data_ul_out_tdata[0];
    //     //     // data_shell_out_tkeep[0] = data_ul_out_tkeep[0];
    //     //     // data_shell_out_tlast[0] = data_ul_out_tlast[0];
    //     //     // data_ul_out_tready[0] = data_shell_out_tready[0];
            
    //     //     data_shell_out_tvalid[1] = data_ul_out_tvalid[1];
    //     //     data_shell_out_tdata[1] = data_ul_out_tdata[1];
    //     //     data_shell_out_tkeep[1] = data_ul_out_tkeep[1];
    //     //     data_shell_out_tlast[1] = data_ul_out_tlast[1];
    //     //     data_ul_out_tready[1] = data_shell_out_tready[1];
    //     // end
        default: 
            ;
    endcase

    // data_ul_in_tvalid[0] = data_shell_in_tvalid[0];
    // data_ul_in_tdata[0] = data_shell_in_tdata[0];
    // data_ul_in_tkeep[0] = data_shell_in_tkeep[0];
    // data_ul_in_tlast[0] = data_shell_in_tlast[0];
    // data_shell_in_tready[0] = data_ul_in_tready[0];

    // data_ul_in_tvalid[1] = data_ul_out_tvalid[0];
    // data_ul_in_tdata[1] = data_ul_out_tdata[0];
    // data_ul_in_tkeep[1] = data_ul_out_tkeep[0];
    // data_ul_in_tlast[1] = data_ul_out_tlast[0];
    // data_ul_out_tready[0] = data_ul_in_tready[1];
    // // data_ul_out_tready[0] = 1'b1;

    // data_shell_out_tvalid[1] = data_shell_in_tvalid[1];
    // data_shell_out_tdata[1] = data_shell_in_tdata[1];
    // data_shell_out_tkeep[1] = data_shell_in_tkeep[1];
    // data_shell_out_tlast[1] = data_shell_in_tlast[1];
    // data_shell_in_tready[1] = data_shell_out_tready[1];
    
    // data_shell_out_tvalid[0] = data_ul_out_tvalid[1];
    // data_shell_out_tdata[0] = data_ul_out_tdata[1];
    // data_shell_out_tkeep[0] = data_ul_out_tkeep[1];
    // data_shell_out_tlast[0] = data_ul_out_tlast[1];
    // data_ul_out_tready[1] = data_shell_out_tready[0];
end

ila_interconnect inst_ila_interconnect (
    .clk(aclk),
    .probe0(data_shell_in[0].tvalid),
    .probe1(data_shell_out[0].tvalid),
    .probe2(data_ul_in[0].tvalid),
    .probe3(data_ul_out[0].tvalid),
    .probe4(data_shell_in[0].tvalid),
    .probe5(data_shell_out[1].tvalid),
    .probe6(data_ul_in[1].tvalid),
    .probe7(data_ul_out[1].tvalid),
    .probe8(data_shell_in[0].tready),
    .probe9(data_shell_out[0].tready),
    .probe10(data_ul_in[0].tready),
    .probe11(data_ul_out[0].tready),
    .probe12(data_shell_in[0].tready),
    .probe13(data_shell_out[1].tready),
    .probe14(data_ul_in[1].tready),
    .probe15(data_ul_out[1].tready),
    .probe16(io_ctrl[0]),
    .probe17(io_ctrl[1])
);

// ila_interconnect_config inst_ila_interconnect_config (
//     .clk(aclk),
//     .probe0(io_ctrl[0]),
//     .probe1(io_ctrl[1])
// );


endmodule