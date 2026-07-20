module quant_top (
    // AXI Stream interfaces
    AXI4SR.s axis_sink,  // 512-bit FFT output
    AXI4SR.m axis_src,   // 512-bit quantized output (for RLE)
    // Clock and reset
    input logic aclk,
    input logic aresetn
);

    // Internal signals for width converter output
    logic axis_conv_tvalid;
    logic axis_conv_tready;
    logic [63:0] axis_conv_tdata;
    logic axis_conv_tlast;
    logic [15:0] axis_conv_tid;

    // Width converter (512->64 bits) for quantizer input
    dwidth_converter_512_64 inst_dwidth_converter (
        .aclk(aclk),
        .aresetn(aresetn),
        .s_axis_tvalid(axis_sink.tvalid),
        .s_axis_tready(axis_sink.tready),
        .s_axis_tdata(axis_sink.tdata),      // 512-bit FFT input
        .s_axis_tlast(axis_sink.tlast),
        .s_axis_tid(axis_sink.tid),
        .m_axis_tvalid(axis_conv_tvalid),
        .m_axis_tready(axis_conv_tready),
        .m_axis_tdata(axis_conv_tdata),      // 64-bit output
        .m_axis_tlast(axis_conv_tlast),
        .m_axis_tid(axis_conv_tid)
    );

    // Quantizer instance - converts 64-bit FFT complex data to 512-bit quantized output
    fft_quantizer inst_quantizer (
        .aclk(aclk),
        .aresetn(aresetn),
        .s_axis_tvalid(axis_conv_tvalid),
        .s_axis_tready(axis_conv_tready),
        .s_axis_tdata(axis_conv_tdata),      // 64-bit FFT complex data
        .s_axis_tlast(axis_conv_tlast),
        .m_axis_tvalid(axis_src.tvalid),
        .m_axis_tready(axis_src.tready),
        .m_axis_tdata(axis_src.tdata),       // 512-bit quantized output
        .m_axis_tlast(axis_src.tlast)
    );

endmodule

module fft_quantizer (
    input  logic        aclk,
    input  logic        aresetn,
    
    // Input from FFT (64-bit: 32-bit real + 32-bit imaginary)
    input  logic        s_axis_tvalid,
    output logic        s_axis_tready,
    input  logic [63:0] s_axis_tdata,
    input  logic        s_axis_tlast,
    
    // Output to RLE (512-bit: 64 quantized bytes to match RLE input)
    output logic        m_axis_tvalid,
    input  logic        m_axis_tready,
    output logic [511:0] m_axis_tdata,  
    output logic        m_axis_tlast
);

    typedef enum logic [1:0] {
        IDLE,
        COLLECT,
        OUTPUT
    } state_t;
    
    state_t state, next_state;
    
    // Sample counter (collect 64 complex samples to create 64 quantized bytes)
    logic [5:0] sample_count;  
    logic [511:0] output_buffer;  
    logic output_tlast;
    logic saw_tlast;  // Track if we've seen tlast during collection
    
    // Extract real and imaginary parts and quantize
    logic [31:0] real_part, imag_part;
    assign real_part = s_axis_tdata[31:0];   
    assign imag_part = s_axis_tdata[63:32];  
    
    logic [31:0] real_abs, imag_abs, magnitude_approx;
    assign real_abs = real_part[31] ? (~real_part + 1) : real_part;
    assign imag_abs = imag_part[31] ? (~imag_part + 1) : imag_part;
    assign magnitude_approx = real_abs + imag_abs;
    
    // Much more sensitive thresholds for audio FFT magnitudes
    logic [7:0] quantized_byte;
    always_comb begin
        if (magnitude_approx < 32'd500) begin           // Very low magnitude
            quantized_byte = 8'h41; // 'A' 
        end
        else if (magnitude_approx < 32'd5000) begin     // Low magnitude  
            quantized_byte = 8'h42; // 'B' 
        end
        else if (magnitude_approx < 32'd25000) begin    // Medium magnitude
            quantized_byte = 8'h43; // 'C' 
        end
        else if (magnitude_approx < 32'd100000) begin   // High magnitude
            quantized_byte = 8'h44; // 'D' 
        end
        else begin                                      // Peak magnitude
            quantized_byte = 8'h50; // 'P' 
        end
    end
    
    // State machine
    always_ff @(posedge aclk or negedge aresetn) begin
        if (!aresetn) begin
            state <= IDLE;
            sample_count <= 0;
            output_buffer <= 0;
            output_tlast <= 0;
            saw_tlast <= 0;
        end
        else begin
            state <= next_state;
            
            case (state)
                IDLE: begin
                    sample_count <= 0;
                    output_buffer <= 0;
                    output_tlast <= 0;
                    saw_tlast <= 0;
                end
                
                COLLECT: begin
                    if (s_axis_tvalid && s_axis_tready) begin
                        // Pack quantized byte into correct position
                        output_buffer[sample_count*8 +: 8] <= quantized_byte;
                        sample_count <= sample_count + 1;
                        
                        // Remember if we saw tlast
                        if (s_axis_tlast) begin
                            saw_tlast <= 1'b1;
                        end
                    end
                end
                
                OUTPUT: begin
                    if (m_axis_tready) begin
                        sample_count <= 0;
                        output_buffer <= 0;
                        output_tlast <= saw_tlast;  // Propagate tlast
                        saw_tlast <= 0;
                    end
                end
            endcase
        end
    end
    
    // State transitions
    always_comb begin
        next_state = state;
        
        case (state)
            IDLE: begin
                if (s_axis_tvalid) begin
                    next_state = COLLECT;
                end
            end
            
            COLLECT: begin
                if (s_axis_tvalid && s_axis_tready) begin
                    // Output when:
                    // 1. Just filled the buffer (after this sample, count will be 64)
                    // 2. This is the last sample (tlast) AND we have something
                    if ((sample_count == 6'd63) || 
                        (s_axis_tlast && (sample_count > 6'd0))) begin  
                        next_state = OUTPUT;
                    end
                end
            end
            
            OUTPUT: begin
                if (m_axis_tready) begin
                    next_state = saw_tlast ? IDLE : COLLECT;
                end
            end
        endcase
    end
    
    // Output assignments
    assign s_axis_tready = (state == COLLECT);
    assign m_axis_tvalid = (state == OUTPUT);
    assign m_axis_tdata = output_buffer;
    assign m_axis_tlast = saw_tlast && (state == OUTPUT);

endmodule