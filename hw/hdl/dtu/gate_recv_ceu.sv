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

module gate_recv_ceu #(
    parameter N_DESTS                   = 1
) (
    input  logic                                aclk,
    input  logic                                aresetn,

    input logic [13:0]                           host_route_cap_in,

    input logic [13:0]                           route_in,
    output logic [1:0]                          ul_port_out
);


logic [N_DESTS-1:0][3:0] route_capa_reg;
logic [3:0] send_ul_id; 
logic [3:0] send_ul_id_host; 
logic [1:0] port_id; 
logic [1:0] port_id_host; 
logic test;

// getting routing capability from host
always_ff @(posedge aclk) begin
    if (~aresetn) begin
        route_capa_reg <= '{default:0};
        send_ul_id <= 0;
        send_ul_id_host <= 0;
        port_id <= 0;
        port_id_host <= 0;
        ul_port_out <= 0;
        test <= 0;
    end else begin
        send_ul_id_host <= host_route_cap_in[9:6];

        // This needs to be updated to include receiving port id
        // currently it's assuming the receiving port is always 00
        port_id_host <= host_route_cap_in[1:0];
        route_capa_reg[port_id_host] <= send_ul_id_host;

        send_ul_id <= route_in[9:6];
        port_id <= route_in[1:0];
        test <= 0;
        // Check if route_in matches the entry in route_capa_reg indexed by route_in[1:0]
        if (send_ul_id == route_capa_reg[port_id]) begin
            $display("send_ul_id: %d, route_capa_reg: %d", send_ul_id, route_capa_reg[port_id]);
            ul_port_out <= route_in[1:0];
            test <= 1;
        end
    end
end

// logic [N_DESTS-1:0][7:0] route_capa_reg;
// logic [1:0] ul_id; 

// // getting routing capability from host
// always_ff @(posedge aclk) begin
//     if (~aresetn) begin
//         route_capa_reg <= '{default:0};
//         ul_id <= 0;
//     end else begin
//         ul_id <= host_route_cap_in[1:0];
//         route_capa_reg[ul_id] <= host_route_cap_in;
//         ul_port_out <= route_in[1:0];
//     end
// end

// // getting routing capability from host
// always_ff @(posedge aclk) begin
//   ul_id <= host_route_cap_in[1:0];
//   route_capa_reg[ul_id] <= host_route_cap_in;
// end

// assign ul_port_out = route_in[1:0];


endmodule