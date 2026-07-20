/**
 * AES CTR Encryption
 */

#include <iostream>
#include <string>
#include <malloc.h>
#include <time.h> 
#include <sys/time.h>  
#include <chrono>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <iomanip>
#include <signal.h>
#include <boost/program_options.hpp>
#include <any>
#include <cstring>
#include <thread>

// Include our high-level ushell API
#include "dfg.hpp"
#include "ushell.hpp"

using namespace std;
using namespace std::chrono;
using namespace fpga;
using namespace ushell;

/* Signal handler */
std::atomic<bool> stalled(false); 
void gotInt(int) {
    stalled.store(true);
}

/* Default parameters */
constexpr auto const defDevice = 0;
constexpr auto const nRegions = 1;  // Single stage: AES encryption
constexpr auto const defHuge = true;
constexpr auto const defMapped = true;
constexpr auto const defStream = 1;
constexpr auto const nRepsThr = 1;
constexpr auto const nRepsLat = 1;
constexpr auto const defSize = 1024;   // Start with smaller size: 1KB
constexpr auto const nBenchRuns = 1;

// Simple test pattern
constexpr uint8_t test_plaintext[16] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};

// Pack into 64-bit words
constexpr uint64_t plain_word0 = 
    ((uint64_t)test_plaintext[0] << 56) |
    ((uint64_t)test_plaintext[1] << 48) |
    ((uint64_t)test_plaintext[2] << 40) |
    ((uint64_t)test_plaintext[3] << 32) |
    ((uint64_t)test_plaintext[4] << 24) |
    ((uint64_t)test_plaintext[5] << 16) |
    ((uint64_t)test_plaintext[6] << 8) |
    ((uint64_t)test_plaintext[7]);

constexpr uint64_t plain_word1 = 
    ((uint64_t)test_plaintext[8] << 56) |
    ((uint64_t)test_plaintext[9] << 48) |
    ((uint64_t)test_plaintext[10] << 40) |
    ((uint64_t)test_plaintext[11] << 32) |
    ((uint64_t)test_plaintext[12] << 24) |
    ((uint64_t)test_plaintext[13] << 16) |
    ((uint64_t)test_plaintext[14] << 8) |
    ((uint64_t)test_plaintext[15]);

// Create descriptor based on panic_define.v format (128-bit descriptor)
void createPanicDescriptor(uint8_t* desc_ptr, uint32_t data_size) {
    // Initialize descriptor to zero (128 bits = 16 bytes)
    memset(desc_ptr, 0, 16);
    
    // Bits 0-31: Length
    desc_ptr[0] = data_size & 0xFF;
    desc_ptr[1] = (data_size >> 8) & 0xFF;
    desc_ptr[2] = (data_size >> 16) & 0xFF;
    desc_ptr[3] = (data_size >> 24) & 0xFF;
    
    // Bits 32-47: Cell ID
    desc_ptr[4] = 0x00;
    desc_ptr[5] = 0x00;
    
    // Bits 48-63: Chain (Set to 0x0001 for DMA engine)
    desc_ptr[6] = 0x01;
    desc_ptr[7] = 0x00;
    
    // Remaining fields set to 0
    // Priority, Time, Drop, Flow, Timestamp, Port
    memset(&desc_ptr[8], 0, 8);
}

// Helper function to print hex data
void printHexData(const char* label, uint64_t* data, int words) {
    std::cout << label << std::endl;
    for(int i = 0; i < words; i++) {
        std::cout << "  [" << i << "]: 0x" << std::hex << std::setfill('0') 
                  << std::setw(16) << data[i] << std::dec << std::endl;
    }
}

// Helper function to print latency statistics
void printLatencyStats(double avg_latency_ns, uint32_t data_size_bytes, uint32_t n_reps) {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nLatency Measurements:" << std::endl;
    std::cout << "Processing started at: 0 ns" << std::endl;
    std::cout << "Processing completed at: " << avg_latency_ns << " ns" << std::endl;
    std::cout << "Total latency: " << avg_latency_ns << " ns (" << (avg_latency_ns / 1000) << " us)" << std::endl;
    std::cout << "Average latency per KB: " << (avg_latency_ns * 1024 / data_size_bytes) << " ns" << std::endl;
    std::cout << "Throughput: " << std::setw(8) 
            << (1000.0 * data_size_bytes) / avg_latency_ns 
            << " MB/s" << std::endl;
}

// Helper function to print header
void print_header(const std::string& header) {
    std::cout << "\n-- \033[31m\e[1m" << header << "\033[0m\e[0m" << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;
}

int main(int argc, char *argv[])  
{
    // Signal handler setup
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gotInt;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // ---------------------------------------------------------------
    // Command Line Arguments
    // ---------------------------------------------------------------
    boost::program_options::options_description programDescription("AES Encryption Pipeline Options:");
    programDescription.add_options()
        ("bitstream,b", boost::program_options::value<string>(), "Shell bitstream")
        ("device,d", boost::program_options::value<uint32_t>(), "Target device")
        ("regions,g", boost::program_options::value<uint32_t>(), "Number of vFPGAs")
        ("size,s", boost::program_options::value<uint32_t>(), "Data size")
        ("reps,r", boost::program_options::value<uint32_t>(), "Number of reps")
        ("hugepages,h", boost::program_options::value<bool>(), "Use hugepages (default: true)")
        ("mapped,m", boost::program_options::value<bool>(), "Use mapped memory (default: true)")
        ("stream,t", boost::program_options::value<bool>(), "Use streaming interface (default: true)")
        ("help", "Show help message");
    
    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    if(commandLineArgs.count("help")) {
        std::cout << programDescription << std::endl;
        return EXIT_SUCCESS;
    }

    // Parse arguments with defaults
    string bstream_path = "";
    uint32_t cs_dev = defDevice;
    uint32_t n_regions = nRegions;
    uint32_t data_size = defSize;
    uint32_t n_reps = nRepsLat;
    bool huge = defHuge;
    bool mapped = defMapped;
    bool stream = defStream;

    // Process command line arguments
    if(commandLineArgs.count("bitstream") > 0) { 
        bstream_path = commandLineArgs["bitstream"].as<string>();
        
        std::cout << std::endl << "Shell loading (path: " << bstream_path << ") ..." << std::endl;
        cRnfg crnfg(cs_dev);
        crnfg.shellReconfigure(bstream_path);
    }
    if(commandLineArgs.count("device") > 0) cs_dev = commandLineArgs["device"].as<uint32_t>();
    if(commandLineArgs.count("regions") > 0) n_regions = commandLineArgs["regions"].as<uint32_t>();
    if(commandLineArgs.count("size") > 0) data_size = commandLineArgs["size"].as<uint32_t>();
    if(commandLineArgs.count("reps") > 0) n_reps = commandLineArgs["reps"].as<uint32_t>();
    if(commandLineArgs.count("hugepages") > 0) huge = commandLineArgs["hugepages"].as<bool>();
    if(commandLineArgs.count("mapped") > 0) mapped = commandLineArgs["mapped"].as<bool>();
    if(commandLineArgs.count("stream") > 0) stream = commandLineArgs["stream"].as<bool>();

    // Ensure data size is multiple of 128 bits (16 bytes) for AES blocks
    data_size = ((data_size + 15) / 16) * 16;
    
    // Limit data size to 16-bit descriptor field
    if(data_size > 65535) {
        std::cout << "WARNING: Data size " << data_size << " exceeds 16-bit length field limit (65535)." << std::endl;
        std::cout << "Limiting to 65520 bytes (65535 rounded down to 16-byte boundary)." << std::endl;
        data_size = 65520;
    }
    
    // Descriptor is 128 bits (16 bytes), padded to 512-bit boundary
    constexpr uint32_t DESC_SIZE_BYTES = 64;  // 512-bit word for descriptor
    constexpr uint32_t ACTUAL_DESC_SIZE = 16; // Actual descriptor is 128 bits
    uint32_t total_packet_size = DESC_SIZE_BYTES + data_size;

    print_header("PARAMS");
    std::cout << "Pipeline: AES Encryption with PANIC Descriptor" << std::endl;
    std::cout << "Number of regions: " << n_regions << std::endl;
    std::cout << "Descriptor size: " << DESC_SIZE_BYTES << " bytes (includes padding)" << std::endl;
    std::cout << "Data size: " << data_size << " bytes (" << data_size/16 << " x 128-bit AES blocks)" << std::endl;
    std::cout << "Total packet size: " << total_packet_size << " bytes" << std::endl;
    std::cout << "Number of reps: " << n_reps << std::endl;
    std::cout << "Hugepages: " << huge << std::endl;
    std::cout << "Mapped: " << mapped << std::endl;
    std::cout << "Streaming: " << (stream ? "HOST" : "CARD") << std::endl;

    try {
        // ---------------------------------------------------------------
        // Dataflow Setup using ushell's fluent API
        // ---------------------------------------------------------------
        print_header("DATAFLOW SETUP");
        
        // Create AES encryption dataflow
        Dataflow aes_dataflow("aes_encryption_pipeline");
        
        // Create processing task
        Task& aes_encryptor = aes_dataflow.add_task("aes_encryptor", "encryption");
        
        // Create buffers
        Buffer& plaintext_buffer = aes_dataflow.add_buffer(total_packet_size, "plaintext_buffer");
        Buffer& ciphertext_buffer = aes_dataflow.add_buffer(total_packet_size, "ciphertext_buffer");
        
        // Set up the AES encryption pipeline using fluent API
        aes_dataflow.to(plaintext_buffer, aes_encryptor.in)
                   .to(aes_encryptor.out, ciphertext_buffer);
        
        std::cout << "Creating AES encryption dataflow:" << std::endl;
        std::cout << "  plaintext_buffer → aes_encryptor → ciphertext_buffer" << std::endl;
        
        // Check and build the dataflow
        if (!aes_dataflow.check()) {
            throw std::runtime_error("Failed to validate dataflow");
        }
        
        // ---------------------------------------------------------------
        // Data Generation and Buffer Initialization
        // ---------------------------------------------------------------
        print_header("DATA GENERATION");
        
        // Generate test data for all repetitions
        std::vector<std::vector<uint64_t>> test_data(n_reps);
        std::vector<std::vector<uint64_t>> result_data(n_reps);
        
        for(int i = 0; i < n_reps; i++) {
            test_data[i].resize(total_packet_size / sizeof(uint64_t));
            result_data[i].resize(total_packet_size / sizeof(uint64_t));
            
            // Create descriptor at the beginning
            memset(test_data[i].data(), 0, DESC_SIZE_BYTES / sizeof(uint64_t));
            createPanicDescriptor((uint8_t*)test_data[i].data(), data_size);
            
            // Fill data portion with test pattern
            uint64_t* data_ptr = test_data[i].data() + (DESC_SIZE_BYTES / sizeof(uint64_t));
            
            // Fill with alternating pattern of test data
            for(int j = 0; j < data_size / 16; j++) {
                data_ptr[j*2] = plain_word0;     // Low 64 bits
                data_ptr[j*2 + 1] = plain_word1; // High 64 bits
            }
        }
        
        // Print input packet for debugging
        print_header("INPUT PACKET DEBUG");
        std::cout << "128-bit Descriptor (16 bytes):" << std::endl;
        uint8_t* desc_bytes = (uint8_t*)test_data[0].data();
        for(int i = 0; i < 16; i++) {
            std::cout << "  Byte[" << i << "]: 0x" << std::hex << std::setfill('0') 
                      << std::setw(2) << (int)desc_bytes[i] << std::dec << std::endl;
        }
        
        printHexData("Full Descriptor Word:", test_data[0].data(), 8);
        printHexData("Input Data (first 8 words):", test_data[0].data() + 8, 8);
        
        // ---------------------------------------------------------------
        // Performance Benchmarking
        // ---------------------------------------------------------------
        print_header("AES ENGINE TEST");
        
        // Create benchmark object
        cBench bench(n_reps);
        
        std::cout << "Starting transfers..." << std::endl;
        
        auto benchmark_thr = [&]() {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            for(int i = 0; i < n_reps; i++) {
                std::cout << "Processing transfer " << i << std::endl;
                
                // Write input data
                write_dataflow_buffer(plaintext_buffer, test_data[i].data(), total_packet_size);
                
                // Clear completion counters
                aes_dataflow.clear_completed();
                
                // Execute the dataflow
                aes_dataflow.execute(total_packet_size);
                
                // Read output data
                read_dataflow_buffer(ciphertext_buffer, result_data[i].data(), total_packet_size);
                
                std::cout << "Transfer " << i << " completed" << std::endl;
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
            std::cout << "All transfers completed in " << elapsed << " seconds!" << std::endl;
        };
        
        bench.runtime(benchmark_thr);
        
        // ---------------------------------------------------------------
        // Results Verification
        // ---------------------------------------------------------------
        print_header("OUTPUT PACKET DEBUG");
        std::cout << "128-bit Output Descriptor (16 bytes):" << std::endl;
        uint8_t* out_desc_bytes = (uint8_t*)result_data[0].data();
        for(int i = 0; i < 16; i++) {
            std::cout << "  Byte[" << i << "]: 0x" << std::hex << std::setfill('0') 
                      << std::setw(2) << (int)out_desc_bytes[i] << std::dec << std::endl;
        }
        
        printHexData("Full Output Descriptor Word:", result_data[0].data(), 8);
        printHexData("Output Data (first 8 words):", result_data[0].data() + 8, 8);
        
        // Verify results
        print_header("VERIFICATION");
        bool success = true;
        
        for(int i = 0; i < n_reps && success; i++) {
            uint64_t* output_data = result_data[i].data() + (DESC_SIZE_BYTES / 8);
            uint64_t* input_data = test_data[i].data() + (DESC_SIZE_BYTES / 8);
            
            // Check for non-zero output
            bool has_output = false;
            for(int j = 0; j < data_size/8; j++) {
                if(output_data[j] != 0) {
                    has_output = true;
                    break;
                }
            }
            
            if(!has_output) {
                std::cout << "ERROR: All zero output detected" << std::endl;
                success = false;
            }
            
            // Check if data was processed (different from input)
            bool is_processed = false;
            for(int j = 0; j < data_size/8; j++) {
                if(output_data[j] != input_data[j]) {
                    is_processed = true;
                    break;
                }
            }
            
            if(!is_processed) {
                std::cout << "WARNING: Output identical to input" << std::endl;
            }
        }
        
        std::cout << "Test result: " << (success ? "PASSED" : "FAILED") << std::endl;
        
        // Performance metrics using printLatencyStats
        print_header("PERFORMANCE");
        printLatencyStats(bench.getAvg() / n_reps, data_size, n_reps);
        
        // ---------------------------------------------------------------
        // Resource Cleanup (Automatic with RAII)
        // ---------------------------------------------------------------
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    print_header("AES ENCRYPTION COMPLETE");
    std::cout << "AES encryption pipeline executed successfully!" << std::endl;
    
    return EXIT_SUCCESS;
}