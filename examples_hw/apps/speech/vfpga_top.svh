/**
 * VFPGA Top Module - FFT to SVM Pipeline 
 * SIMPLE DELAY FIX: Add delay to host TLAST capture to account for FFT pipeline
 */
import lynxTypes::*;

// Interface declarations
AXI4SR axis_sink_int ();
AXI4SR axis_src_int ();

// Host stream routing
axisr_reg inst_reg_sink (
    .aclk(aclk),
    .aresetn(aresetn),
    .s_axis(axis_host_recv[0]),
    .m_axis(axis_sink_int)
);

axisr_reg inst_reg_src (
    .aclk(aclk),
    .aresetn(aresetn),
    .s_axis(axis_src_int),
    .m_axis(axis_host_send[0])
);

// FFT internal signals
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

// Set constant values for FFT
assign xlconstant_0_dout = 8'h1;
assign xlconstant_1_dout = 1'b1;

always_comb begin
    axis_host_recv_tkeep = 1;
    axis_host_send_tkeep = 1;
    axis_host_recv_tid = 0;
    axis_host_send_tid = 0;
end

// Input width converter (512->64 bits) for FFT
dwidth_converter_512_64 inst_dwidth_recv_fft (
    .aclk(aclk),
    .aresetn(aresetn),
    .s_axis_tvalid(axis_sink_int.tvalid),
    .s_axis_tready(axis_sink_int.tready),
    .s_axis_tdata(axis_sink_int.tdata),
    .s_axis_tlast(axis_sink_int.tlast),
    .s_axis_tid(axis_sink_int.tid),
    .m_axis_tvalid(axis_host_recv_tvalid),
    .m_axis_tready(axis_host_recv_tready),
    .m_axis_tdata(axis_host_recv_tdata),
    .m_axis_tlast(axis_host_recv_tlast),
    .m_axis_tid(axis_host_recv_tid)
);

// SIMPLE DELAY FIX: Add delay to host TLAST capture
logic host_tlast_immediate;
logic [4:0] tlast_delay_counter;
logic host_tlast_captured;
logic host_stream_complete;
logic stream_reset;

// Reset when starting a new stream
assign stream_reset = !aresetn;

// Capture and delay host TLAST signal
always_ff @(posedge aclk) begin
    if (stream_reset) begin
        host_tlast_immediate <= 0;
        tlast_delay_counter <= 0;
        host_tlast_captured <= 0;
        host_stream_complete <= 0;
    end else begin
        // Capture host TLAST immediately
        if (axis_host_recv_tvalid && axis_host_recv_tready && axis_host_recv_tlast) begin
            host_tlast_immediate <= 1;
            tlast_delay_counter <= 1;  // Start delay counter
        end
        
        // Count delay cycles
        if (tlast_delay_counter > 0 && tlast_delay_counter < 16) begin
            tlast_delay_counter <= tlast_delay_counter + 1;
        end
        
        // Activate delayed capture after sufficient delay
        if (tlast_delay_counter >= 16) begin  // 16 cycle delay - adjust as needed
            host_tlast_captured <= 1;
        end
        
        // Mark stream as complete when SVM outputs final result with TLAST
        if (svm_out_tvalid && svm_out_tready && svm_out_tlast) begin
            host_stream_complete <= 1;
        end
        
        // Reset for next stream when current stream is complete
        if (host_stream_complete) begin
            host_tlast_immediate <= 0;
            tlast_delay_counter <= 0;
            host_tlast_captured <= 0;
            host_stream_complete <= 0;
        end
    end
end

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

// SVM interface signals
logic svm_tvalid;
logic svm_tready;
logic [31:0] svm_tdata;
logic svm_tlast;

// AND GATE APPROACH: Combine DELAYED Host TLAST and FFT TLAST
logic fft_packet_complete;
logic final_packet_tlast;

// FFT indicates packet completion
assign fft_packet_complete = axis_host_send_tlast;

// CRITICAL: TLAST only when BOTH conditions are true
// Use delayed host_tlast_captured to avoid pipeline mismatch
assign final_packet_tlast = host_tlast_captured && fft_packet_complete;

// SVM interface
always_comb begin
    svm_tvalid = axis_host_send_tvalid;
    svm_tdata = axis_host_send_tdata[31:0];  // Extract real part
    svm_tlast = final_packet_tlast;          // AND of delayed host ending + FFT complete
end

// Backpressure handling
assign axis_host_send_tready = svm_tready;

// SVM output signals
logic svm_out_tvalid;
logic svm_out_tready;
logic [31:0] svm_out_tdata;
logic svm_out_tlast;

// SVM instance
svm_speech_30_0 inst_svm (
    .ap_clk(aclk),
    .ap_rst_n(aresetn),
    .input_r_TDATA(svm_tdata),
    .input_r_TVALID(svm_tvalid),
    .input_r_TREADY(svm_tready),
    .input_r_TLAST(svm_tlast),
    .output_r_TDATA(svm_out_tdata),
    .output_r_TVALID(svm_out_tvalid),
    .output_r_TREADY(svm_out_tready),
    .output_r_TLAST(svm_out_tlast)
);

// SVM output width converter signals
logic [3:0] svm_out_tkeep;
logic [15:0] svm_out_tid;

always_comb begin
    svm_out_tkeep = 4'h1;
    svm_out_tid = 16'h0;
end

// Output width converter (32->512) 
dwidth_converter_32_512 inst_dwidth_send (
    .aclk(aclk),
    .aresetn(aresetn),
    .s_axis_tvalid(svm_out_tvalid),
    .s_axis_tready(svm_out_tready),
    .s_axis_tdata(svm_out_tdata),
    .s_axis_tlast(svm_out_tlast),
    .s_axis_tid(svm_out_tid),
    .m_axis_tvalid(axis_src_int.tvalid),
    .m_axis_tready(axis_src_int.tready),
    .m_axis_tdata(axis_src_int.tdata),
    .m_axis_tlast(axis_src_int.tlast),
    .m_axis_tid(axis_src_int.tid)
);

// Tie-off unused interfaces
always_comb axi_ctrl.tie_off_s();
always_comb notify.tie_off_m();
always_comb sq_rd.tie_off_m();
always_comb sq_wr.tie_off_m();
always_comb cq_rd.tie_off_s();
always_comb cq_wr.tie_off_s();