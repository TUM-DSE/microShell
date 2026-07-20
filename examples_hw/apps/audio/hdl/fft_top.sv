/**
 * Modular FFT Module with Host TLAST Propagation
 */
module fft_top (
    // AXI Stream interfaces
    AXI4SR.s axis_sink,
    AXI4SR.m axis_src,
    // Clock and reset
    input logic aclk,
    input logic aresetn
);

// Internal signals
logic axis_host_recv_tvalid;
logic axis_host_recv_tready;
logic [63:0] axis_host_recv_tdata;
logic [3:0] axis_host_recv_tkeep;
logic [15:0] axis_host_recv_tid;
logic axis_host_recv_tlast;
logic axis_host_send_tvalid;
logic axis_host_send_tready;
logic [63:0] axis_host_send_tdata;
logic [3:0] axis_host_send_tkeep;
logic [15:0] axis_host_send_tid;
logic axis_host_send_tlast;

// FFT configuration constants
logic [7:0] xlconstant_0_dout;
logic xlconstant_1_dout;

// TLAST tracking signals
logic host_tlast_captured;
logic final_output_tlast;

// Set constant values
assign xlconstant_0_dout = 8'h1;
assign xlconstant_1_dout = 1'b1;

always_comb begin
    axis_host_recv_tkeep = 1;
    axis_host_send_tkeep = 1;
    axis_host_recv_tid = 0;
    axis_host_send_tid = 0;
end

// Track host TLAST through the pipeline
always_ff @(posedge aclk) begin
    if (!aresetn) begin
        host_tlast_captured <= 0;
    end else begin
        // Capture host TLAST when it arrives
        if (axis_host_recv_tvalid && axis_host_recv_tready && axis_host_recv_tlast) begin
            host_tlast_captured <= 1;
        end
        
        // Reset when we output the final TLAST
        if (axis_host_send_tvalid && axis_host_send_tready && final_output_tlast) begin
            host_tlast_captured <= 0;
        end
    end
end

// AND gate: Only assert TLAST when both host indicated end AND FFT outputs its TLAST
assign final_output_tlast = host_tlast_captured && axis_host_send_tlast;

// Input width converter (512->64 bits)
dwidth_converter_512_64 inst_dwidth_recv (
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

// FFT instance
xfft_0 inst_xfft (
    .aclk(aclk),
    .aresetn(aresetn),
    .s_axis_data_tdata(axis_host_recv_tdata),
    .s_axis_data_tvalid(axis_host_recv_tvalid),
    .s_axis_data_tready(axis_host_recv_tready),
    .s_axis_data_tlast(axis_host_recv_tlast),
    .m_axis_data_tdata(axis_host_send_tdata),
    .m_axis_data_tvalid(axis_host_send_tvalid),
    .m_axis_data_tready(axis_host_send_tready),
    .m_axis_data_tlast(axis_host_send_tlast),
    .s_axis_config_tdata(xlconstant_0_dout),
    .s_axis_config_tvalid(xlconstant_1_dout),
    .s_axis_config_tready()
);

// Output width converter (64->512 bits) with modified TLAST
dwidth_converter_64_512 inst_dwidth_send (
    .aclk(aclk),
    .aresetn(aresetn),
    .s_axis_tvalid(axis_host_send_tvalid),
    .s_axis_tready(axis_host_send_tready),
    .s_axis_tdata(axis_host_send_tdata),
    .s_axis_tlast(final_output_tlast),    // Use AND-gated TLAST instead of raw FFT TLAST
    .s_axis_tid(axis_host_send_tid),
    .m_axis_tvalid(axis_src.tvalid),
    .m_axis_tready(axis_src.tready),
    .m_axis_tdata(axis_src.tdata),
    .m_axis_tlast(axis_src.tlast),
    .m_axis_tid(axis_src.tid)
);

endmodule