create_ip -name ila -vendor xilinx.com -library ip -version 6.2 -module_name ila_perf_host

set_property -dict { 
    CONFIG.C_NUM_OF_PROBES {6} 
    CONFIG.C_EN_STRG_QUAL {1} 
    CONFIG.C_PROBE5_MU_CNT {2} 
    CONFIG.C_PROBE4_MU_CNT {2} 
    CONFIG.C_PROBE3_MU_CNT {2} 
    CONFIG.C_PROBE2_MU_CNT {2} 
    CONFIG.C_PROBE1_MU_CNT {2} 
    CONFIG.C_PROBE0_MU_CNT {2} 
    CONFIG.ALL_PROBE_SAME_MU_CNT {2}
} [get_ips ila_perf_host]

create_ip -name ila -vendor xilinx.com -library ip -version 6.2 -module_name ila_perf_counter

set_property -dict { 
    CONFIG.C_NUM_OF_PROBES {4} 
    CONFIG.C_EN_STRG_QUAL {1} 
    CONFIG.C_PROBE3_WIDTH {32}
    CONFIG.ALL_PROBE_SAME_MU_CNT {2}
} [get_ips ila_perf_counter]

