/**
 * VFPGA Top Module
 * 
 */
import lynxTypes::*;
     
// Intermediate interfaces for connecting modules
AXI4SR axis_sink_int ();
AXI4SR axis_src_int ();
AXI4SR rsa_to_counter ();

// Stream routing with proper interface connections
axisr_reg inst_reg_sink (.aclk(aclk), .aresetn(aresetn), .s_axis(axis_host_recv[0]), .m_axis(axis_sink_int));
axisr_reg inst_reg_src (.aclk(aclk), .aresetn(aresetn), .s_axis(axis_src_int), .m_axis(axis_host_send[0]));

// Cycle counter outputs
logic [31:0] pipeline_cycle_count;
logic pipeline_count_valid;

// RSA 
rsa_top inst_rsa (
    .axis_sink      (axis_sink_int),
    .axis_src       (rsa_to_counter),
    .aclk           (aclk),
    .aresetn        (aresetn)
);

// Cycle Counter
counter_top inst_counter (
    .axis_sink      (rsa_to_counter),
    .axis_src       (axis_src_int),
    .aclk           (aclk),
    .aresetn        (aresetn),
    .cycle_count    (pipeline_cycle_count),
    .count_valid    (pipeline_count_valid)
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
    
// Tie off unused interfaces
always_comb axi_ctrl.tie_off_s();
always_comb notify.tie_off_m();
always_comb sq_rd.tie_off_m();
always_comb sq_wr.tie_off_m();
always_comb cq_rd.tie_off_s();
always_comb cq_wr.tie_off_s();
assign user_data = 0;