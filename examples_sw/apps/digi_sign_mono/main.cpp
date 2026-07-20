/**
 * Digital Signature Pipeline (Monolithic)
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
#include <vector>
#include <map>
#include <cstring>
#include <sstream>

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
constexpr auto const nRegions = 2;
constexpr auto const defHuge = true;
constexpr auto const defMapped = true;
constexpr auto const defStream = 1;
constexpr auto const nRepsThr = 1;
constexpr auto const nRepsLat = 1;
constexpr auto const defInputSize = 64;  // Default is 64 bytes (512 bits)
constexpr auto const OUTPUT_SIZE = 32;   // 32 bytes (256 bits)
constexpr auto const nBenchRuns = 1;

// Define expected signatures for different data sizes from simulation
const std::map<uint32_t, std::string> expectedSignatures = {
    {32 * 1024, "0df7cd1be029d306e6659b55f528662920ad44045ee08d2f711c7f9eeffcf7fc"},
    {64 * 1024, "0e76911d8183866e508357435109d8348da11959407f8fe42849fe554b00c89a"},
    {128 * 1024, "05f847595f9a76013962e9bf322cf7aa0531af248c898cafd42e7248839b3bd2"},
    {256 * 1024, "0d80761a70dbf3027ef2dfb7b50a7f01615f96367aaf71403bc993baa8f2b99c"},
    {512 * 1024, "09f219ce7f89d489446567fa85e2849a06258a9f9529bf3c4692b11da41578cf"}
};

// Helper function to print hex values
void printHexBuffer(uint32_t* buffer, size_t bytes, const char* label) {
    std::cout << label << ": 0x";
    for(int i = (bytes/4)-1; i >= 0; i--) {
        std::cout << std::hex << std::setw(8) << std::setfill('0') << buffer[i];
    }
    std::cout << std::dec << std::endl;
}

// Helper function to convert buffer to hex string
std::string bufferToHexString(uint32_t* buffer, size_t bytes) {
    std::stringstream ss;
    for(int i = (bytes/4)-1; i >= 0; i--) {
        ss << std::hex << std::setw(8) << std::setfill('0') << buffer[i];
    }
    return ss.str();
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
            << (1000000000.0 * data_size_bytes) / (avg_latency_ns * 1000000.0) 
            << " MB/s" << std::endl;
}

// Helper function to print header
void print_header(const std::string& header) {
    std::cout << "\n-- \033[31m\e[1m" << header << "\033[0m\e[0m" << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;
}

int main(int argc, char *argv[]) {
    // Signal handler setup
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gotInt;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // ---------------------------------------------------------------
    // Command Line Arguments
    // ---------------------------------------------------------------
    boost::program_options::options_description programDescription("Digital Signature Pipeline Options:");
    programDescription.add_options()
        ("bitstream,b", boost::program_options::value<string>(), "Shell bitstream")
        ("device,d", boost::program_options::value<uint32_t>(), "Target device")
        ("regions,g", boost::program_options::value<uint32_t>(), "Number of vFPGAs")
        ("message,m", boost::program_options::value<std::string>(), "Input message to sign")
        ("size,s", boost::program_options::value<uint32_t>(), "Input size in bytes (default: 64)")
        ("reps,r", boost::program_options::value<uint32_t>(), "Number of repetitions")
        ("hugepages,h", boost::program_options::value<bool>(), "Use hugepages (default: true)")
        ("mapped,p", boost::program_options::value<bool>(), "Use mapped memory (default: true)")
        ("stream,t", boost::program_options::value<bool>(), "Use streaming interface (default: true)")
        ("help", "Show help message");
    
    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), 
                                commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    if(commandLineArgs.count("help")) {
        std::cout << programDescription << std::endl;
        return EXIT_SUCCESS;
    }

    // Parse arguments with defaults
    string bstream_path = "";
    uint32_t cs_dev = defDevice;
    uint32_t n_regions = nRegions;
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
    if(commandLineArgs.count("reps") > 0) n_reps = commandLineArgs["reps"].as<uint32_t>();
    if(commandLineArgs.count("hugepages") > 0) huge = commandLineArgs["hugepages"].as<bool>();
    if(commandLineArgs.count("mapped") > 0) mapped = commandLineArgs["mapped"].as<bool>();
    if(commandLineArgs.count("stream") > 0) stream = commandLineArgs["stream"].as<bool>();

    uint32_t input_size = defInputSize;
    if(commandLineArgs.count("size") > 0) {
        input_size = commandLineArgs["size"].as<uint32_t>();
        // Ensure input size is a multiple of 64 (512 bits)
        if(input_size % 64 != 0) {
            uint32_t old_size = input_size;
            input_size = ((input_size + 63) / 64) * 64;
            std::cout << "Adjusting input size from " << old_size << " to " << input_size 
                      << " bytes (multiple of 64 bytes)" << std::endl;
        }
    }

    print_header("DIGITAL SIGNATURE TEST");
    std::cout << "Pipeline: SHA256 → RSA Signing" << std::endl;
    std::cout << "Number of regions: " << n_regions << std::endl;
    std::cout << "Input size: " << input_size << " bytes (" << (input_size * 8) << "-bit)" << std::endl;
    std::cout << "Output size: " << OUTPUT_SIZE << " bytes (256-bit)" << std::endl;
    std::cout << "Number of repetitions: " << n_reps << std::endl;
    std::cout << "Hugepages: " << huge << std::endl;
    std::cout << "Mapped: " << mapped << std::endl;
    std::cout << "Streaming: " << (stream ? "HOST" : "CARD") << std::endl;

    try {
        // ---------------------------------------------------------------
        // Dataflow Setup using ushell's fluent API
        // ---------------------------------------------------------------
        print_header("DATAFLOW SETUP");
        
        // Create digital signature dataflow
        Dataflow digisign_dataflow("digital_signature_pipeline");
        
        // Create processing tasks
        Task& sha256_hasher = digisign_dataflow.add_task("sha256_hasher", "hashing");
        Task& rsa_signer = digisign_dataflow.add_task("rsa_signer", "signing");
        
        // Create buffers
        Buffer& message_buffer = digisign_dataflow.add_buffer(input_size, "message_buffer");
        Buffer& hash_buffer = digisign_dataflow.add_buffer(32, "hash_buffer");  // SHA256 output is 32 bytes
        Buffer& signature_buffer = digisign_dataflow.add_buffer(OUTPUT_SIZE, "signature_buffer");
        
        // Set up the digital signature pipeline using fluent API
        digisign_dataflow.to(message_buffer, sha256_hasher.in)
                        .to(sha256_hasher.out, hash_buffer)
                        .to(hash_buffer, rsa_signer.in)
                        .to(rsa_signer.out, signature_buffer);
        
        std::cout << "Creating digital signature dataflow:" << std::endl;
        std::cout << "  message_buffer → sha256_hasher → hash_buffer → rsa_signer → signature_buffer" << std::endl;
        
        // Check and build the dataflow
        if (!digisign_dataflow.check()) {
            throw std::runtime_error("Failed to validate dataflow");
        }
        
        // ---------------------------------------------------------------
        // Data Generation and Buffer Initialization
        // ---------------------------------------------------------------
        print_header("DATA GENERATION");
        
        // Generate test data for all repetitions
        std::vector<std::vector<uint32_t>> test_data(n_reps);
        std::vector<std::vector<uint32_t>> result_data(n_reps);
        
        for(int i = 0; i < n_reps; i++) {
            test_data[i].resize(input_size / 4);  // uint32_t array
            result_data[i].resize(OUTPUT_SIZE / 4);  // uint32_t array
            
            // Fill input with constant chunk pattern (matching the testbench)
            // This matches CONSTANT_CHUNK = 512'hFEDCBA9876543210FEDCBA9876543210...
            for (int j = 0; j < input_size/4; j += 2) {
                // Fill two 32-bit words at a time to match the 64-bit pattern
                test_data[i][j] = 0x76543210;
                if (j+1 < input_size/4) {
                    test_data[i][j+1] = 0xFEDCBA98;
                }
            }
        }
        std::cout << "\nUsing constant chunk pattern for all inputs." << std::endl;
        
        // ---------------------------------------------------------------
        // Performance Benchmarking
        // ---------------------------------------------------------------
        print_header("PROCESSING");
        
        // Create benchmark object
        cBench bench(n_reps);
        
        auto benchmark_thr = [&]() {
            for(int i = 0; i < n_reps; i++) {
                // Write input data
                write_dataflow_buffer(message_buffer, test_data[i].data(), input_size);
                
                // Clear completion counters
                digisign_dataflow.clear_completed();
                
                // Execute the dataflow
                digisign_dataflow.execute(input_size);
                
                // Read output data
                read_dataflow_buffer(signature_buffer, result_data[i].data(), OUTPUT_SIZE);
            }
        };
        
        bench.runtime(benchmark_thr);
        
        // ---------------------------------------------------------------
        // Results Verification
        // ---------------------------------------------------------------
        print_header("RESULTS");
        
        // Only print the first signature result as a sample
        std::cout << "\nInput Data (first 512 bits):" << std::endl;
        printHexBuffer(test_data[0].data(), 64, "Input ");
        
        std::cout << "\nDigital Signature (256 bits):" << std::endl;
        printHexBuffer(result_data[0].data(), OUTPUT_SIZE, "Output");
        
        // Print detailed latency statistics
        print_header("LATENCY MEASUREMENTS");
        printLatencyStats(bench.getAvg() / n_reps, input_size, n_reps);
        
        // Verify the signature against expected value
        print_header("VERIFICATION");
        std::string actual_sig = bufferToHexString(result_data[0].data(), OUTPUT_SIZE);
        
        // Check if we have an expected signature for this data size
        if (expectedSignatures.find(input_size) != expectedSignatures.end()) {
            std::string expected_sig = expectedSignatures.at(input_size);
            std::cout << "Expected: 0x" << expected_sig << std::endl;
            std::cout << "Actual  : 0x" << actual_sig << std::endl;
            
            if (expected_sig == actual_sig) {
                std::cout << "\033[32mSIGNATURE MATCH: The output matches expected simulation result!\033[0m" << std::endl;
            } else {
                std::cout << "\033[31mSIGNATURE MISMATCH: The output does not match expected simulation!\033[0m" << std::endl;
            }
        } else {
            std::cout << "No expected signature available for comparison at size " << input_size << " bytes." << std::endl;
            std::cout << "Actual signature: 0x" << actual_sig << std::endl;
        }
        
        // ---------------------------------------------------------------
        // Resource Cleanup (Automatic with RAII)
        // ---------------------------------------------------------------
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    print_header("DIGITAL SIGNATURE COMPLETE");
    std::cout << "Digital signature pipeline executed successfully!" << std::endl;
    
    return EXIT_SUCCESS;
}