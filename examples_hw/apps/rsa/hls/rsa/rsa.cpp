/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "rsa.hpp"
#include "asymmetric.hpp"
#include <hls_stream.h>

void rsa(
    hls::stream<ap_axiu<256,0,0,0>>& message,
    ap_uint<256> modulus,
    ap_uint<20> exponent,
    hls::stream<ap_axiu<256,0,0,0>>& result
) {
    #pragma HLS INTERFACE axis port=message
    #pragma HLS INTERFACE axis port=result
    #pragma HLS INTERFACE ap_none port=modulus
    #pragma HLS INTERFACE ap_none port=exponent
    #pragma HLS INTERFACE ap_ctrl_none port=return
    
    xf::security::rsa processor;
    processor.updateKey(modulus, exponent);
    
    // Continuous processing loop for ap_ctrl_none
    while (true) {
        #pragma HLS PIPELINE off
        
        // Process messages when available
        if (!message.empty()) {
            processor.process(message, result);
        }
    }
}