/**
 * VFPGA Top Module
 */
import lynxTypes::*;

// Interface declarations
AXI4SR axis_sink_int ();
AXI4SR axis_src_int ();

// Host stream routing
axisr_reg inst_reg_sink (.aclk(aclk), .aresetn(aresetn), .s_axis(axis_host_recv[0]), .m_axis(axis_sink_int));
axisr_reg inst_reg_src (.aclk(aclk), .aresetn(aresetn), .s_axis(axis_src_int), .m_axis(axis_host_send[0]));

// RLE
rle_top inst_rle (
    .axis_sink      (axis_sink_int),
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
