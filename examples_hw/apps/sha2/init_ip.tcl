# Create Data Width Converter IPs
create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name dwidth_converter_256_512
set_property -dict [list \
    CONFIG.S_TDATA_NUM_BYTES {32} \
    CONFIG.M_TDATA_NUM_BYTES {64} \
    CONFIG.TID_WIDTH {4} \
    CONFIG.HAS_TLAST {1} \
] [get_ips dwidth_converter_256_512]

# Generate all IP targets
generate_target all [get_ips]
