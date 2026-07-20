/**
 * VFPGA Top Module - SVM Only
 */
import lynxTypes::*;

    // Interface signals 
    logic        dwidth_in_tvalid;
    logic        dwidth_in_tready;
    logic [31:0] dwidth_in_tdata;
    logic        dwidth_in_tlast;
    logic [3:0]  dwidth_in_tkeep;
    logic [15:0] dwidth_in_tid;
    
    logic        dwidth_out_tvalid;
    logic        dwidth_out_tready;
    logic [31:0] dwidth_out_tdata;
    logic        dwidth_out_tlast;
    logic [3:0]  dwidth_out_tkeep;
    logic [15:0] dwidth_out_tid;
     
    
    // Stream interface connections
    AXI4SR axis_sink_int ();
    AXI4SR axis_src_int ();
    
    // Stream routing with proper interface connections
    axisr_reg inst_reg_sink (.aclk(aclk), .aresetn(aresetn), .s_axis(axis_host_recv[0]), .m_axis(axis_sink_int));
    axisr_reg inst_reg_src (.aclk(aclk), .aresetn(aresetn), .s_axis(axis_src_int), .m_axis(axis_host_send[0]));
        
    // Static assignments
    always_comb begin
        dwidth_in_tkeep = 1;
        dwidth_out_tkeep = 1;
        dwidth_in_tid = 0;
        dwidth_out_tid = 0;
    end
    
    // Input width converter (512->32)
    dwidth_converter_512_32 inst_dwidth_recv (
        .aclk(aclk),
        .aresetn(aresetn),
        .s_axis_tvalid(axis_sink_int.tvalid),
        .s_axis_tready(axis_sink_int.tready),
        .s_axis_tdata (axis_sink_int.tdata),
        .s_axis_tlast (axis_sink_int.tlast),
        .s_axis_tid   (axis_sink_int.tid),  
        .m_axis_tvalid(dwidth_in_tvalid),
        .m_axis_tready(dwidth_in_tready),
        .m_axis_tdata (dwidth_in_tdata),
        .m_axis_tlast (dwidth_in_tlast),
        .m_axis_tid   (dwidth_in_tid) 
    );
        
    // SVM IP
    svm_speech_30_0 inst_svm (
	 .ap_clk          (aclk),
	 .ap_rst_n        (aresetn),
	 .input_r_TDATA   (dwidth_in_tdata),
	 .input_r_TVALID  (dwidth_in_tvalid),
	 .input_r_TREADY  (dwidth_in_tready),
	 .input_r_TLAST   (dwidth_in_tlast),
	 .output_r_TDATA  (dwidth_out_tdata),
	 .output_r_TVALID (dwidth_out_tvalid),
	 .output_r_TREADY (dwidth_out_tready),
	 .output_r_TLAST  (dwidth_out_tlast)
    );

    // Output width converter (32->512)
    dwidth_converter_32_512 inst_dwidth_send (
        .aclk(aclk),
        .aresetn(aresetn),
        .s_axis_tvalid(dwidth_out_tvalid),
        .s_axis_tready(dwidth_out_tready),
        .s_axis_tdata (dwidth_out_tdata),
        .s_axis_tlast (dwidth_out_tlast),
        .s_axis_tid   (dwidth_out_tid), 
        .m_axis_tvalid(axis_src_int.tvalid),
        .m_axis_tready(axis_src_int.tready),
        .m_axis_tdata (axis_src_int.tdata),
        .m_axis_tlast (axis_src_int.tlast),
        .m_axis_tid   (axis_src_int.tid) 
    );
    
    // Tie off unused interfaces
    always_comb axi_ctrl.tie_off_s();
    always_comb notify.tie_off_m();
    always_comb sq_rd.tie_off_m();
    always_comb sq_wr.tie_off_m();
    always_comb cq_rd.tie_off_s();
    always_comb cq_wr.tie_off_s();
