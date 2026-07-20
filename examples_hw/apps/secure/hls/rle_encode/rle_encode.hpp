/*
 *  Run-Length Encoding
 */
#ifndef RLE_HPP
#define RLE_HPP

#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <ap_int.h>

// Function declaration for RLE 
void rle_encode(
    hls::stream<ap_axiu<512,0,0,0>>& input_stream,
    hls::stream<ap_axiu<512,0,0,0>>& output_stream
);

#endif // RLE_HPP