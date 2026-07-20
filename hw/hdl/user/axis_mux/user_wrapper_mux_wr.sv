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

module user_wrapper_mux_wr #(
    parameter N_DESTS                   = 1
) (
    input  logic                                aclk,
    input  logic                                aresetn,

    output logic [1:0]                          port_out,

    AXI4SR.s                                 s_axis [N_DESTS],
    AXI4SR.m                                 m_axis
);


// ----------------------------------------------------------------------------------------------------------------------- 
// IO
// ----------------------------------------------------------------------------------------------------------------------- 
logic [N_DESTS-1:0]                     s_axis_tvalid;
logic [N_DESTS-1:0]                     s_axis_tready;
logic [N_DESTS-1:0][AXI_DATA_BITS-1:0]      s_axis_tdata;
logic [N_DESTS-1:0][AXI_DATA_BITS/8-1:0]    s_axis_tkeep;
logic [N_DESTS-1:0]                     s_axis_tlast;
logic [N_DESTS-1:0][PID_BITS-1:0]       s_axis_tid;

logic                                   m_axis_tvalid;
logic                                   m_axis_tready;
logic [AXI_DATA_BITS-1:0]                   m_axis_tdata;
logic [AXI_DATA_BITS/8-1:0]                 m_axis_tkeep;
logic                                   m_axis_tlast;
logic [PID_BITS-1:0]                    m_axis_tid;


for(genvar i = 0; i < N_DESTS; i++) begin
    assign s_axis_tvalid[i] = s_axis[i].tvalid;
    assign s_axis_tdata[i] = s_axis[i].tdata;
    assign s_axis_tkeep[i] = s_axis[i].tkeep;
    assign s_axis_tlast[i] = s_axis[i].tlast;
    assign s_axis_tid[i] = s_axis[i].tid;
    assign s_axis[i].tready = s_axis_tready[i];
end

assign m_axis.tvalid = m_axis_tvalid;
assign m_axis.tdata = m_axis_tdata;
assign m_axis.tkeep = m_axis_tkeep;
assign m_axis.tlast = m_axis_tlast;
assign m_axis.tid = m_axis_tid;
assign m_axis_tready = m_axis.tready;

// ----------------------------------------------------------------------------------------------------------------------- 
// Mux
// ----------------------------------------------------------------------------------------------------------------------- 
always_comb begin
    for(int i = 0; i < N_DESTS; i++) begin
        port_out = 0;
        if (s_axis_tvalid[i]) begin
            s_axis_tready[i] = m_axis_tready;
            port_out = i;
        end
        else begin
            s_axis_tready[i] = 0;
        end 
    end

    if(port_out < N_DESTS) begin
        m_axis_tdata = s_axis_tdata[port_out];
        m_axis_tkeep = s_axis_tkeep[port_out];
        m_axis_tlast = s_axis_tlast[port_out];
        m_axis_tvalid = s_axis_tvalid[port_out];
        m_axis_tid = s_axis_tid[port_out];
    end
    else begin
        m_axis_tdata = 0;
        m_axis_tkeep = 0;
        m_axis_tlast = 1'b0;
        m_axis_tvalid = 1'b0;
        m_axis_tid = 0;
    end

end

endmodule