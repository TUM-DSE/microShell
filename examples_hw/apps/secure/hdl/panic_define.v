`timescale 1ns / 1ps
`define PANIC_DESC_WIDTH (128)          // INCREASE from 96 to 128 bits
`define PANIC_DESC_LEN_SIZE (32)        // Your change - good!
`define PANIC_DESC_LEN_OF (0)
`define PANIC_DESC_CELL_ID_SIZE (16)
`define PANIC_DESC_CELL_ID_OF (32)      // Shifted from 16 to 32
`define PANIC_DESC_CHAIN_SIZE (16)
`define PANIC_DESC_CHAIN_OF (48)        // Shifted from 32 to 48  
`define PANIC_DESC_CHAIN_ITEM_SIZE (4)
`define PANIC_DESC_PRIO_SIZE (8)
`define PANIC_DESC_PRIO_OF (64)         // Shifted from 48 to 64
`define PANIC_DESC_TIME_SIZE (12)
`define PANIC_DESC_TIME_OF (72)         // Shifted from 56 to 72
`define PANIC_DESC_DROP_OF (84)         // Shifted from 68 to 84
`define PANIC_DESC_FLOW_SIZE (8)
`define PANIC_DESC_FLOW_OF (85)         // Shifted from 69 to 85
`define PANIC_DESC_TS_SIZE (18)
`define PANIC_DESC_TS_OF (93)           // Shifted from 77 to 93
`define PANIC_DESC_PORT_OF (111)        // Shifted from 95 to 111


`define PANIC_CREDIT_WIDTH (32)     

`define PANIC_CREDIT_SRC_SIZE (4)
`define PANIC_CREDIT_SRC_OF (0)
`define PANIC_CREDIT_TYPE_SIZE (4)  // 1 add credit, 2 cancle credit
`define PANIC_CREDIT_TYPE_OF (4)

`define PANIC_CELL_SIZE (512*3*8/8)
`define PANIC_SERVICE_TIME (5)