/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define AP_INT_MAX_W 4097
#include "asymmetric.hpp"
#include <hls_stream.h>

void rsa(
    hls::stream<ap_axiu<256,0,0,0>>& message,
    ap_uint<256> modulus,
    ap_uint<20> exponent,
    hls::stream<ap_axiu<256,0,0,0>>& result
);