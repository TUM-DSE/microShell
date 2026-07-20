/**
 * VFPGA Top Module with AES CTR
 * 
 */
import lynxTypes::*;

// Interface declarations for pipeline stages
AXI4SR axis_host_sink_int ();
AXI4SR axis_host_src_int ();

// Stream routing with proper interface connections
axisr_reg inst_reg_sink (.aclk(aclk), .aresetn(aresetn), .s_axis(axis_host_recv[0]), .m_axis(axis_host_sink_int));
axisr_reg inst_reg_src (.aclk(aclk), .aresetn(aresetn), .s_axis(axis_host_src_int), .m_axis(axis_host_send[0]));

// AES 
aes_top inst_aes (
    .axis_sink      (axis_host_sink_int),
    .axis_src       (axis_host_src_int),
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