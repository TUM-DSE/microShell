# Create Data Width Converter IPs
create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name dwidth_converter_512_64
set_property -dict [list \
    CONFIG.S_TDATA_NUM_BYTES {64} \
    CONFIG.M_TDATA_NUM_BYTES {8} \
    CONFIG.TID_WIDTH {4} \
    CONFIG.HAS_TLAST {1} \
] [get_ips dwidth_converter_512_64]

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name dwidth_converter_64_512
set_property -dict [list \
    CONFIG.S_TDATA_NUM_BYTES {8} \
    CONFIG.M_TDATA_NUM_BYTES {64} \
    CONFIG.TID_WIDTH {4} \
    CONFIG.HAS_TLAST {1} \
] [get_ips dwidth_converter_64_512]

# Create FFT IP
create_ip -name xfft -vendor xilinx.com -library ip -version 9.1 -module_name xfft_0
set_property -dict [list \
    CONFIG.transform_length {16384} \
    CONFIG.data_format {floating_point} \
    CONFIG.implementation_options {automatically_select} \
    CONFIG.input_width {32} \
    CONFIG.output_ordering {natural_order} \
    CONFIG.aresetn {true} \
] [get_ips xfft_0]

# Generate all IP targets
generate_target all [get_ips]
