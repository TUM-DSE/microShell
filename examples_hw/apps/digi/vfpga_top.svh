/**
 * VFPGA Top Module - Digital Signature Pipeline
 * 
 */
import lynxTypes::*;

// Interface declarations
AXI4SR axis_sink_int ();
AXI4SR axis_src_int ();

// Host stream routing
axisr_reg inst_reg_sink (.aclk(aclk), .aresetn(aresetn), .s_axis(axis_host_recv[0]), .m_axis(axis_sink_int));
axisr_reg inst_reg_src (.aclk(aclk), .aresetn(aresetn), .s_axis(axis_src_int), .m_axis(axis_host_send[0]));

// Intermediate interfaces for connecting modules
AXI4SR sha2_to_rsa ();
AXI4SR rsa_to_counter ();

// Cycle counter signals
logic [31:0] pipeline_cycle_count;
logic        pipeline_count_valid;

// SHA2 Module
sha2_top inst_sha2 (
    .axis_sink          (axis_sink_int),
    .axis_src           (sha2_to_rsa),
    .aclk               (aclk),
    .aresetn            (aresetn)
);

// RSA Module
r_top inst_rsa (
    .axis_sink          (sha2_to_rsa),
    .axis_src           (rsa_to_counter),
    .aclk               (aclk),
    .aresetn            (aresetn)
);

// Cycle Counter Module
counter_top inst_counter (
    .axis_sink          (rsa_to_counter),
    .axis_src           (axis_src_int),
    .aclk               (aclk),
    .aresetn            (aresetn),
    .cycle_count        (pipeline_cycle_count),
    .count_valid        (pipeline_count_valid)
);

// Register to store cycle count result
logic [31:0] latency_result_reg;

always_ff @(posedge aclk or negedge aresetn) begin
    if (!aresetn) begin
        latency_result_reg <= '0;
    end else if (pipeline_count_valid) begin
        latency_result_reg <= pipeline_cycle_count;
    end
end

// Tie-off unused
always_comb axi_ctrl.tie_off_s();
always_comb notify.tie_off_m();
always_comb sq_rd.tie_off_m();
always_comb sq_wr.tie_off_m();
always_comb cq_rd.tie_off_s();
always_comb cq_wr.tie_off_s();