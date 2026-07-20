`timescale 1ns / 1ps
import lynxTypes::*;
`include "axi_macros.svh"
/**
 * Modular Cycle Counter 
 */
module counter_top (
    // AXI Stream interfaces - pass-through design
    AXI4SR.s             axis_sink,
    AXI4SR.m             axis_src,
    
    // Clock and reset
    input  logic         aclk,
    input  logic         aresetn,
    
    // Optional: Output interface for cycle count result
    output logic [31:0]  cycle_count,
    output logic         count_valid
);
    // Internal signals for event detection and state tracking
    logic first_data_seen;
    logic pipeline_start_event;
    logic pipeline_end_event;
    
    // Pass-through connection - input to output
    assign axis_src.tdata = axis_sink.tdata;
    assign axis_src.tvalid = axis_sink.tvalid;
    assign axis_src.tlast = axis_sink.tlast;
    assign axis_src.tkeep = axis_sink.tkeep;
    assign axis_src.tid = axis_sink.tid;
    assign axis_sink.tready = axis_src.tready;
    
    // Detect start of data processing (first valid input)
    always_ff @(posedge aclk or negedge aresetn) begin
        if (!aresetn) begin
            first_data_seen <= 1'b0;
        end else if (axis_sink.tvalid && axis_sink.tready && !first_data_seen) begin
            first_data_seen <= 1'b1;
        end else if (axis_sink.tvalid && axis_sink.tready && axis_sink.tlast) begin
            // Reset when a packet with tlast is processed to prepare for next run
            first_data_seen <= 1'b0;
        end
    end

    // Define start and end events for cycle counter
    assign pipeline_start_event = axis_sink.tvalid && axis_sink.tready && !first_data_seen;
    assign pipeline_end_event = axis_sink.tvalid && axis_sink.tready && axis_sink.tlast;
    
    // Instantiate the cycle counter module
    cycle_counter #(
        .COUNTER_WIDTH(32)
    ) inst_counter (
        .aclk           (aclk),
        .aresetn        (aresetn),
        .start_event    (pipeline_start_event),
        .end_event      (pipeline_end_event),
        .cycle_count    (cycle_count),
        .count_valid    (count_valid)
    );

endmodule

/**
 * Cycle Counter 
 * 
 */
module cycle_counter #(
    parameter COUNTER_WIDTH = 32  // Width of the counter register
)(
    input  logic                    aclk,
    input  logic                    aresetn,
    input  logic                    start_event,  // Event that starts the counter
    input  logic                    end_event,    // Event that stops the counter
    output logic [COUNTER_WIDTH-1:0] cycle_count, // Cycle count result
    output logic                    count_valid   // Flag indicating valid count
);
    // States for the counter FSM
    typedef enum logic [1:0] {
        IDLE,      // Waiting for start event
        COUNTING,  // Counting cycles
        DONE       // Counting complete
    } counter_state_t;
    
    counter_state_t state, next_state;
    logic [COUNTER_WIDTH-1:0] counter;
    logic counter_reset, counter_enable;
    
    // FSM state register
    always_ff @(posedge aclk or negedge aresetn) begin
        if (!aresetn) begin
            state <= IDLE;
        end else begin
            state <= next_state;
        end
    end
    
    // FSM next state logic
    always_comb begin
        next_state = state;
        counter_reset = 1'b0;
        counter_enable = 1'b0;
        
        case (state)
            IDLE: begin
                if (start_event) begin
                    next_state = COUNTING;
                    counter_reset = 1'b1;
                end
            end
            
            COUNTING: begin
                counter_enable = 1'b1;
                if (end_event) begin
                    next_state = DONE;
                    counter_enable = 1'b0;
                end
            end
            
            DONE: begin
                // Stay in DONE until reset or new start event
                if (start_event) begin
                    next_state = COUNTING;
                    counter_reset = 1'b1;
                end
            end
            
            default: begin
                next_state = IDLE;
            end
        endcase
    end
    
    // Counter logic
    always_ff @(posedge aclk or negedge aresetn) begin
        if (!aresetn) begin
            counter <= '0;
        end else if (counter_reset) begin
            counter <= '0;
        end else if (counter_enable) begin
            counter <= counter + 1'b1;
        end
    end
    
    // Output assignments
    assign cycle_count = counter;
    assign count_valid = (state == DONE);
    
endmodule