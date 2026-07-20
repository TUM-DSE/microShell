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

module user_wrapper_mux_rd #(
    parameter N_DESTS                   = 1
) (
    input  logic                                aclk,
    input  logic                                aresetn,

    input logic [1:0]                           port_in,

    AXI4SR.s                                 s_axis,
    AXI4SR.m                                 m_axis [N_DESTS]
);


logic                                   s_axis_tvalid;
logic                                   s_axis_tready;
logic [AXI_DATA_BITS-1:0]                   s_axis_tdata;
logic [AXI_DATA_BITS/8-1:0]                 s_axis_tkeep;
logic                                   s_axis_tlast;
logic [PID_BITS-1:0]                    s_axis_tid;

logic [N_DESTS-1:0]                     m_axis_tvalid;
logic [N_DESTS-1:0]                     m_axis_tready;
logic [N_DESTS-1:0][AXI_DATA_BITS-1:0]      m_axis_tdata;
logic [N_DESTS-1:0][AXI_DATA_BITS/8-1:0]    m_axis_tkeep;
logic [N_DESTS-1:0]                     m_axis_tlast;
logic [N_DESTS-1:0][PID_BITS-1:0]       m_axis_tid;

assign s_axis_tvalid = s_axis.tvalid;
assign s_axis_tdata = s_axis.tdata;
assign s_axis_tkeep = s_axis.tkeep;
assign s_axis_tlast = s_axis.tlast;
assign s_axis_tid = s_axis.tid;
assign s_axis.tready = s_axis_tready;

for(genvar i = 0; i < N_DESTS; i++) begin
    assign m_axis[i].tvalid = m_axis_tvalid[i];
    assign m_axis[i].tdata = m_axis_tdata[i];
    assign m_axis[i].tkeep = m_axis_tkeep[i];
    assign m_axis[i].tlast = m_axis_tlast[i];
    assign m_axis[i].tid   = m_axis_tid[i];
    assign m_axis_tready[i] = m_axis[i].tready;
end


// ----------------------------------------------------------------------------------------------------------------------- 
// Mux
// ----------------------------------------------------------------------------------------------------------------------- 
always_comb begin
    for(int i = 0; i < N_DESTS; i++) begin
        m_axis_tdata[i] = s_axis_tdata;
        m_axis_tkeep[i] = s_axis_tkeep;
        m_axis_tlast[i] = s_axis_tlast;
        m_axis_tid[i]   = s_axis_tid;
        m_axis_tvalid[i] = (port_in == i) ? s_axis_tvalid : 1'b0;

    end

    // if(dest_C < N_DESTS && state_C == ST_MUX) 
    s_axis_tready = m_axis_tready[port_in];
    // else 
    //     s_axis_tready = 1'b0;
end

endmodule