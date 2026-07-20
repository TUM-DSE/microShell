# Create Data Width Converter IPs
create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name dwidth_converter_512_32
set_property -dict [list \
    CONFIG.S_TDATA_NUM_BYTES {64} \
    CONFIG.M_TDATA_NUM_BYTES {4} \
    CONFIG.TID_WIDTH {4} \
    CONFIG.HAS_TLAST {1} \
] [get_ips dwidth_converter_512_32]

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name dwidth_converter_32_512
set_property -dict [list \
    CONFIG.S_TDATA_NUM_BYTES {4} \
    CONFIG.M_TDATA_NUM_BYTES {64} \
    CONFIG.TID_WIDTH {4} \
    CONFIG.HAS_TLAST {1} \
] [get_ips dwidth_converter_32_512]

create_ip -name ila -vendor xilinx.com -library ip -version 6.2 -module_name ila_0
set_property -dict [list \
    CONFIG.C_NUM_OF_PROBES {16} \
    CONFIG.C_EN_STRG_QUAL {1} \
    CONFIG.C_PROBE0_WIDTH {1} \
    CONFIG.C_PROBE1_WIDTH {6} \
    CONFIG.C_PROBE2_WIDTH {4} \
    CONFIG.C_PROBE3_WIDTH {3} \
    CONFIG.C_PROBE4_WIDTH {1} \
    CONFIG.C_PROBE5_WIDTH {1} \
    CONFIG.C_PROBE6_WIDTH {32} \
    CONFIG.C_PROBE7_WIDTH {1} \
    CONFIG.C_PROBE8_WIDTH {1} \
    CONFIG.C_PROBE9_WIDTH {1} \
    CONFIG.C_PROBE10_WIDTH {32} \
    CONFIG.C_PROBE11_WIDTH {1} \
    CONFIG.C_PROBE12_WIDTH {1} \
    CONFIG.C_PROBE13_WIDTH {1} \
    CONFIG.C_PROBE14_WIDTH {1} \
    CONFIG.C_PROBE15_WIDTH {1} \
    CONFIG.C_PROBE0_MU_CNT {2} \
    CONFIG.C_PROBE1_MU_CNT {2} \
    CONFIG.C_PROBE2_MU_CNT {2} \
    CONFIG.C_PROBE3_MU_CNT {2} \
    CONFIG.C_PROBE4_MU_CNT {2} \
    CONFIG.C_PROBE5_MU_CNT {2} \
    CONFIG.C_PROBE6_MU_CNT {2} \
    CONFIG.C_PROBE7_MU_CNT {2} \
    CONFIG.C_PROBE8_MU_CNT {2} \
    CONFIG.C_PROBE9_MU_CNT {2} \
    CONFIG.C_PROBE10_MU_CNT {2} \
    CONFIG.C_PROBE11_MU_CNT {2} \
    CONFIG.C_PROBE12_MU_CNT {2} \
    CONFIG.C_PROBE13_MU_CNT {2} \
    CONFIG.C_PROBE14_MU_CNT {2} \
    CONFIG.C_PROBE15_MU_CNT {2} \
    CONFIG.C_DATA_DEPTH {1024} \
] [get_ips ila_0]

# Generate all IP targets
generate_target all [get_ips]
