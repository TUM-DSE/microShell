/**
 * VFPGA Top Module with FFT + RLE Pipeline
 *
 */
import lynxTypes::*;

// Interface declarations for pipeline stages
AXI4SR axis_sink_int ();
AXI4SR axis_src_int ();
AXI4SR fft_to_quant ();
AXI4SR quant_to_rle ();

// Stream routing with proper interface connections
axisr_reg inst_reg_sink (.aclk(aclk), .aresetn(aresetn), .s_axis(axis_host_recv[0]), .m_axis(axis_sink_int));
axisr_reg inst_reg_src (.aclk(aclk), .aresetn(aresetn), .s_axis(axis_src_int), .m_axis(axis_host_send[0]));

// FFT module instantiation
fft_top inst_fft (
    .axis_sink(axis_sink_int),
    .axis_src(fft_to_quant),   
    .aclk(aclk),
    .aresetn(aresetn)
);

// Quantizer module instantiation 
quant_top inst_quantizer (
    .axis_sink(fft_to_quant),   
    .axis_src(quant_to_rle),  
    .aclk(aclk),
    .aresetn(aresetn)
);

// RLE
rle_top inst_rle (
    .axis_sink      (quant_to_rle),
    .axis_src       (axis_src_int),
    .aclk           (aclk),
    .aresetn        (aresetn)
);

// Tie off unused interfaces
always_comb axi_ctrl.tie_off_s();
always_comb notify.tie_off_m();
always_comb sq_rd.tie_off_m();
always_comb sq_wr.tie_off_m();
always_comb cq_rd.tie_off_s();
always_comb cq_wr.tie_off_s();
