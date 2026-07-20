#ifndef XF_SECURITY_ASYMMETRIC_CRYPTOGRAPHY_HPP_
#define XF_SECURITY_ASYMMETRIC_CRYPTOGRAPHY_HPP_
#include <ap_int.h>
#include <hls_stream.h>
#include "modular.hpp"
#include <ap_axi_sdata.h>

namespace xf {
namespace security {

class rsa {
private:
    // Make these member variables to persist across calls
    ap_uint<256> accumulator;
    bool processing_message;
    ap_uint<16> packet_count;  // Increased to 16 bits for larger messages
    
public:
    ap_uint<20> exponent;
    ap_uint<256> modulus;
    ap_uint<256> rMod;
    
    // Constructor to initialize state
    rsa() : accumulator(0), processing_message(false), packet_count(0) {}
    
    void updateKey(ap_uint<256> inputModulus, ap_uint<20> inputExponent) {
        // Store key parameters
        modulus = inputModulus;
        exponent = inputExponent;
        
        // Calculate Montgomery parameters
        // R = 2^256 mod N
        ap_uint<257> R = 0;
        R[256] = 1;
        ap_uint<256> R_mod = R % modulus;
        
        // Calculate R^2 mod N for Montgomery arithmetic
        rMod = xf::security::internal::productMod<256>(R_mod, R_mod, modulus);
    }
    
    void process(
        hls::stream<ap_axiu<256,0,0,0>>& message_in,
        hls::stream<ap_axiu<256,0,0,0>>& result_out
    ) {
        // Process ONE packet per call (suitable for repeated calls)
        if (!message_in.empty()) {
            // Read input packet
            ap_axiu<256,0,0,0> message_input = message_in.read();
            ap_uint<256> msg = message_input.data;
            
            // Start of new message
            if (!processing_message) {
                accumulator = msg;
                processing_message = true;
                packet_count = 1;
            } else {
                // Continue accumulating
                packet_count++;
                
                // Fix: Include packet count to break symmetry
                // This prevents zero output at 256 chunks
                ap_uint<256> rotated = (accumulator << 7) | (accumulator >> (256-7));
                accumulator = rotated ^ msg ^ ap_uint<256>(packet_count);
            }
            
            // Check if this is the last packet
            if (message_input.last) {
                // Process the accumulated data
                // Convert to Montgomery form
                ap_uint<256> mont_message = xf::security::internal::monProduct<256>(
                    accumulator, rMod, modulus);
                
                // Perform modular exponentiation
                ap_uint<256> result = xf::security::internal::modularExp<256, 20>(
                    mont_message, exponent, modulus, rMod);
                
                // Write output
                ap_axiu<256,0,0,0> result_output;
                result_output.data = result;
                result_output.keep = ~0;
                result_output.last = 1;
                result_output.strb = ~0;
                result_out.write(result_output);
                
                // Reset state for next message
                processing_message = false;
                accumulator = 0;
                packet_count = 0;
            }
        }
    }
};

} // namespace security
} // namespace xf
#endif