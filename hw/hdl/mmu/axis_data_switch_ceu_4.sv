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
module axis_data_switch_ceu_4 #(
    // parameter integer MUX_DATA_BITS = AXI_DATA_BITS,
    parameter integer N_ID = N_REGIONS
) (
    input  logic                             aclk,
    input  logic                             aresetn,

    // IO control
    input logic [N_REGIONS-1:0][13:0]         route_in,
    output logic [N_REGIONS-1:0][13:0]        route_out,


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
logic [7:0] axis_switch_0_s_decode_err;
logic [N_ID-1:0][13:0] axis_switch_0_m_tdest;
logic [N_ID-1:0] region_src_id, region_dest_id;


// ----------------------------------------------------------------------------------------------------------------------- 
// -- Mux 
// ----------------------------------------------------------------------------------------------------------------------- 
// -- interface loop issues => temp signals

logic [N_ID-1:0]                        data_host_sink_tvalid;
logic [N_ID-1:0]                        data_host_sink_tready;
logic [N_ID-1:0][AXI_DATA_BITS-1:0]     data_host_sink_tdata;
logic [N_ID-1:0][AXI_DATA_BITS/8-1:0]   data_host_sink_tkeep;
logic [N_ID-1:0]                        data_host_sink_tlast;
logic [N_ID-1:0][PID_BITS-1:0]                        data_host_sink_tid;

logic [N_ID-1:0]                        data_host_src_tvalid;
logic [N_ID-1:0]                        data_host_src_tready;
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


logic [N_ID-1:0]                        data_dtu_sink_tvalid;
logic [N_ID-1:0]                        data_dtu_sink_tready;
logic [N_ID-1:0][AXI_DATA_BITS-1:0]     data_dtu_sink_tdata;
logic [N_ID-1:0][AXI_DATA_BITS/8-1:0]   data_dtu_sink_tkeep;
logic [N_ID-1:0]                        data_dtu_sink_tlast;
logic [N_ID-1:0][PID_BITS-1:0]                        data_dtu_sink_tid;

logic [N_ID-1:0]                        data_dtu_src_tvalid;
logic [N_ID-1:0]                        data_dtu_src_tready;
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


// always_comb begin
//     for(int i = 0; i < N_ID; i++) begin
//         region_src_id =  data_dtu_sink_tvalid[i] ? i : 1'b0;
//     end

//     // here might have a problem
//     if (data_dtu_sink_tvalid[region_src_id]) begin
//         if (route_in[region_src_id][5:3] > N_ID-1) begin
//             region_dest_id = route_in[region_src_id][5:3] - N_ID;
//             route_out[region_dest_id] = route_in[region_src_id];
//         end
//     end
// end


axis_switch_ceu_8_0 inst_axis_switch_ceu_0 (
    .aclk(aclk),
    .aresetn(aresetn),
    .m_axis_tdata({data_dtu_src_tdata, data_host_src_tdata}),
    .m_axis_tdest({route_out, axis_switch_0_m_tdest}),
    .m_axis_tready({data_dtu_src_tready, data_host_src_tready}),
    .m_axis_tvalid({data_dtu_src_tvalid, data_host_src_tvalid}),
    .m_axis_tlast({data_dtu_src_tlast, data_host_src_tlast}),
    .m_axis_tid({data_dtu_src_tid, data_host_src_tid}),
    .s_axis_tdata({data_dtu_sink_tdata, data_host_sink_tdata}),
    .s_axis_tdest({route_in[3], route_in[2], route_in[1], route_in[0], 14'b01111111111100, 14'b01101111111100, 14'b01011111111100, 14'b01001111111100}),
    .s_axis_tready({data_dtu_sink_tready, data_host_sink_tready}),
    .s_axis_tvalid({data_dtu_sink_tvalid, data_host_sink_tvalid}),
    .s_axis_tlast({data_dtu_sink_tlast, data_host_sink_tlast}),
    .s_axis_tid({data_dtu_sink_tid, data_host_sink_tid}),
    .s_decode_err(axis_switch_0_s_decode_err)
);

// ila_interconnect_config inst_ila_interconnect_config (
//     .clk(aclk),
//     .probe0(route_in[0]),
//     .probe1(route_in[1])
// );
// set_property -dict [list CONFIG.TDEST_WIDTH {8} CONFIG.M01_AXIS_BASETDEST {0x00000010} CONFIG.M02_AXIS_BASETDEST {0x00000020} CONFIG.M03_AXIS_BASETDEST {0x00000030} CONFIG.M00_AXIS_HIGHTDEST {0x0000000f} CONFIG.M01_AXIS_HIGHTDEST {0x0000001f} CONFIG.M02_AXIS_HIGHTDEST {0x0000002f} CONFIG.M03_AXIS_HIGHTDEST {0x0000003f}] [get_ips axis_switch_4_0]

// set_property -dict [list CONFIG.M00_AXIS_BASETDEST {0x00} CONFIG.M01_AXIS_BASETDEST {0x20} CONFIG.M02_AXIS_BASETDEST {0x40} CONFIG.M03_AXIS_BASETDEST {0x60} CONFIG.M00_AXIS_HIGHTDEST {0x0000001F} CONFIG.M01_AXIS_HIGHTDEST {0x0000003F} CONFIG.M02_AXIS_HIGHTDEST {0x5F} CONFIG.M03_AXIS_HIGHTDEST {0x7f}] [get_ips axis_switch_4_0]

//create_ip -name axis_switch -vendor xilinx.com -library ip -version 1.1 -module_name axis_switch_6_0
//set_property -dict [list CONFIG.NUM_MI {6} CONFIG.NUM_SI {6} CONFIG.TDATA_NUM_BYTES {64} CONFIG.TDEST_WIDTH {8} CONFIG.TID_WIDTH {6} CONFIG.DECODER_REG {1}] [get_ips axis_switch_6_0]
//set_property -dict [list CONFIG.M00_AXIS_BASETDEST {0x00} CONFIG.M01_AXIS_BASETDEST {0x20} CONFIG.M02_AXIS_BASETDEST {0x40} CONFIG.M03_AXIS_BASETDEST {0x60} CONFIG.M04_AXIS_BASETDEST {0x80} CONFIG.M05_AXIS_BASETDEST {0xA0} CONFIG.M00_AXIS_HIGHTDEST {0x0000001F} CONFIG.M01_AXIS_HIGHTDEST {0x0000003F} CONFIG.M02_AXIS_HIGHTDEST {0x5F} CONFIG.M03_AXIS_HIGHTDEST {0x7F} CONFIG.M04_AXIS_HIGHTDEST {0x9F} CONFIG.M05_AXIS_HIGHTDEST {0xBF}] [get_ips axis_switch_4_0]


endmodule