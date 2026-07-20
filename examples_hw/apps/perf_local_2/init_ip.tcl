create_ip -name ila -vendor xilinx.com -library ip -version 6.2 -module_name ila_perf_host

set_property -dict { 
    CONFIG.C_NUM_OF_PROBES {6} 
    CONFIG.C_EN_STRG_QUAL {1} 
    CONFIG.ALL_PROBE_SAME_MU_CNT {2}
} [get_ips ila_perf_host]

create_ip -name ila -vendor xilinx.com -library ip -version 6.2 -module_name ila_perf_host_2

set_property -dict { 
    CONFIG.C_NUM_OF_PROBES {12} 
    CONFIG.C_EN_STRG_QUAL {1} 
    CONFIG.ALL_PROBE_SAME_MU_CNT {2}
} [get_ips ila_perf_host_2]