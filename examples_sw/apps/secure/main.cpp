/**
 * Secure Storage Pipeline
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
constexpr auto const defMinSize = 2 * 1024 * 1024;  // 2MB
constexpr auto const defMaxSize = 2 * 1024 * 1024;  // 2MB
constexpr auto const nBenchRuns = 1;

// AES test pattern from working individual test
constexpr uint8_t test_plaintext[16] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
    'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'
};

// Generate pattern optimized for secure storage pipeline
void generatePipelineOptimizedPattern(uint8_t* buffer, size_t size) {
    memset(buffer, 0, size);
    
    std::cout << "Generating pipeline-optimized pattern for secure storage:" << std::endl;
    
    // Each 64-byte input chunk should compress to 16 bytes of 'abcdefghijklmnop'
    for (size_t chunk = 0; chunk < size / 64; chunk++) {
        size_t chunk_offset = chunk * 64;
        
        // Fill each 64-byte chunk with pattern that compresses to 'abcdefghijklmnop'
        // Strategy: Repeat each character 4 times for 4:1 compression
        for (int i = 0; i < 16; i++) {
            char target_char = test_plaintext[i];  // 'a' through 'p'
            
            // Repeat each character 4 times within the chunk
            for (int repeat = 0; repeat < 4; repeat++) {
                buffer[chunk_offset + i * 4 + repeat] = target_char;
            }
        }
    }
    
    std::cout << "Expected RLE compression: aaaabbbbccccdddd...pppp → abcdefghijklmnop (4:1 ratio)" << std::endl;
    std::cout << "Pipeline flow: Input → RLE → AES-compatible format → AES encryption" << std::endl;
}

// Generate TRUE 4:1 compression pattern
void generateStreamingRLEPattern(uint8_t* buffer, size_t size) {
    memset(buffer, 0, size);

    // Pattern: Each character repeated 4 times for true 4:1 compression
    for (size_t pos = 0; pos < size; pos++) {
        // Each character appears 4 times in sequence, cycling through A-P (16 chars)
        char base_char = 'A' + ((pos / 4) % 16);  // A-P, each repeated 4 times
        buffer[pos] = base_char;
    }

    std::cout << "Expected compression: AAAABBBBCCCC...PPPP → ABCDEFGHIJKLMNOP (TRUE 4:1 ratio)" << std::endl;
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
    // ---------------------------------------------------------------
    // Signal Handler Setup
    // ---------------------------------------------------------------
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gotInt;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // ---------------------------------------------------------------
    // Command Line Arguments
    // ---------------------------------------------------------------
    boost::program_options::options_description programDescription("Options:");
    programDescription.add_options()
        ("bitstream,b", boost::program_options::value<string>(), "Shell bitstream")
        ("device,d", boost::program_options::value<uint32_t>(), "Target device")
        ("regions,g", boost::program_options::value<uint32_t>(), "Number of vFPGAs")
        ("hugepages,h", boost::program_options::value<bool>(), "Hugepages")
        ("mapped,m", boost::program_options::value<bool>(), "Mapped / page fault")
        ("stream,t", boost::program_options::value<bool>(), "Streaming interface")
        ("repst,r", boost::program_options::value<uint32_t>(), "Number of repetitions (throughput)")
        ("repsl,l", boost::program_options::value<uint32_t>(), "Number of repetitions (latency)")
        ("min_size,n", boost::program_options::value<uint32_t>(), "Starting transfer size")
        ("max_size,x", boost::program_options::value<uint32_t>(), "Ending transfer size")
        ("pattern,p", boost::program_options::value<string>(), "Pattern type: 'optimized' or 'streaming'");
    
    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    // Parse arguments with defaults
    string bstream_path = "";
    uint32_t cs_dev = defDevice; 
    uint32_t n_regions = nRegions;
    bool huge = defHuge;
    bool mapped = defMapped;
    bool stream = defStream;
    uint32_t n_reps_thr = nRepsThr;
    uint32_t n_reps_lat = nRepsLat;
    uint32_t curr_size = defMinSize;
    uint32_t max_size = defMaxSize;
    string pattern_type = "streaming";  // Default to streaming pattern

    // Process command line arguments
    if(commandLineArgs.count("bitstream") > 0) { 
        bstream_path = commandLineArgs["bitstream"].as<string>();
        
        std::cout << std::endl << "Shell loading (path: " << bstream_path << ") ..." << std::endl;
        cRnfg crnfg(cs_dev);
        crnfg.shellReconfigure(bstream_path);
    }
    if(commandLineArgs.count("device") > 0) cs_dev = commandLineArgs["device"].as<uint32_t>();
    if(commandLineArgs.count("regions") > 0) n_regions = commandLineArgs["regions"].as<uint32_t>();
    if(commandLineArgs.count("hugepages") > 0) huge = commandLineArgs["hugepages"].as<bool>();
    if(commandLineArgs.count("mapped") > 0) mapped = commandLineArgs["mapped"].as<bool>();
    if(commandLineArgs.count("stream") > 0) stream = commandLineArgs["stream"].as<bool>();
    if(commandLineArgs.count("repst") > 0) n_reps_thr = commandLineArgs["repst"].as<uint32_t>();
    if(commandLineArgs.count("repsl") > 0) n_reps_lat = commandLineArgs["repsl"].as<uint32_t>();
    if(commandLineArgs.count("min_size") > 0) curr_size = commandLineArgs["min_size"].as<uint32_t>();
    if(commandLineArgs.count("max_size") > 0) max_size = commandLineArgs["max_size"].as<uint32_t>();
    if(commandLineArgs.count("pattern") > 0) pattern_type = commandLineArgs["pattern"].as<string>();

    print_header("PARAMS");
    std::cout << "Number of regions: " << n_regions << std::endl;
    std::cout << "Hugepages: " << huge << std::endl;
    std::cout << "Mapped pages: " << mapped << std::endl;
    std::cout << "Streaming: " << (stream ? "HOST" : "CARD") << std::endl;
    std::cout << "Number of repetitions (thr): " << n_reps_thr << std::endl;
    std::cout << "Number of repetitions (lat): " << n_reps_lat << std::endl;
    std::cout << "Starting transfer size: " << curr_size << " bytes" << std::endl;
    std::cout << "Ending transfer size: " << max_size << " bytes" << std::endl;
    std::cout << "Pattern type: " << pattern_type << std::endl;

    try {
        // ---------------------------------------------------------------
        // Dataflow Setup using ushell's fluent API
        // ---------------------------------------------------------------
        print_header("DATAFLOW SETUP");
        
        // Create a compression + encryption dataflow
        Dataflow secure_dataflow("secure_dataflow");
        
        // Create processing tasks
        Task& rle_compressor = secure_dataflow.add_task("rle_compressor", "compression");
        Task& aes_encryptor = secure_dataflow.add_task("aes_encryptor", "encryption");
        
        // Calculate buffer sizes
        uint32_t input_buffer_size = max_size;
        uint32_t compressed_buffer_size = max_size / 4;  // 4:1 compression
        uint32_t encrypted_buffer_size = compressed_buffer_size;  // AES doesn't change size
        
        // Create buffers
        Buffer& raw_data_buffer = secure_dataflow.add_buffer(input_buffer_size, "raw_data_buffer");
        Buffer& compressed_data_buffer = secure_dataflow.add_buffer(compressed_buffer_size, "compressed_data_buffer");
        Buffer& encrypted_data_buffer = secure_dataflow.add_buffer(encrypted_buffer_size, "encrypted_data_buffer");
        
        // Set up the pipeline using fluent API
        secure_dataflow.to(raw_data_buffer, rle_compressor.in)
                       .to(rle_compressor.out, compressed_data_buffer)
                       .to(compressed_data_buffer, aes_encryptor.in)
                       .to(aes_encryptor.out, encrypted_data_buffer);
        
        std::cout << "Creating secure storage dataflow:" << std::endl;
        std::cout << "  raw_data_buffer → rle_compressor → compressed_data_buffer → aes_encryptor → encrypted_data_buffer" << std::endl;
        
        // Check and build the dataflow
        if (!secure_dataflow.check()) {
            throw std::runtime_error("Failed to validate dataflow");
        }
        
        // ---------------------------------------------------------------
        // Data Generation and Buffer Initialization
        // ---------------------------------------------------------------
        print_header("DATA GENERATION");
        
        // Generate test data based on pattern type
        std::vector<uint8_t> test_data(max_size);
        
        if (pattern_type == "optimized") {
            generatePipelineOptimizedPattern(test_data.data(), max_size);
        } else {
            generateStreamingRLEPattern(test_data.data(), max_size);
        }
        
        // Write test data to input buffer
        write_dataflow_buffer(raw_data_buffer, test_data.data(), max_size);
        std::cout << "Initialized input buffer with " << max_size << " bytes of test data" << std::endl;
        
        // ---------------------------------------------------------------
        // Performance Benchmarking
        // ---------------------------------------------------------------
        print_header("PERFORMANCE");
        
        // Create benchmark object
        cBench bench(nBenchRuns);
        
        uint32_t current_size = curr_size;
        
        while (current_size <= max_size) {
            secure_dataflow.clear_completed();
            
            auto benchmark_lat = [&]() {
                for (int i = 0; i < n_reps_lat; i++) {
                    secure_dataflow.execute(current_size);
                }
            };
            
            bench.runtime(benchmark_lat);
            
            std::cout << "\nSize: " << current_size << " bytes";
            printLatencyStats(bench.getAvg() / n_reps_lat, current_size, n_reps_lat);
            
            current_size *= 2;
        }
        
        // ---------------------------------------------------------------
        // Results Verification
        // ---------------------------------------------------------------
        print_header("RESULTS");
        
        // Read the encrypted output
        std::vector<uint8_t> encrypted_result(encrypted_buffer_size);
        read_dataflow_buffer(encrypted_data_buffer, encrypted_result.data(), encrypted_buffer_size);
        
        std::cout << "Input size: " << max_size << " bytes" << std::endl;
        std::cout << "Compressed size: " << compressed_buffer_size << " bytes" << std::endl;
        std::cout << "Encrypted size: " << encrypted_buffer_size << " bytes" << std::endl;
        std::cout << "Compression ratio: " << (float)max_size / (float)compressed_buffer_size << ":1" << std::endl;
        
        // ---------------------------------------------------------------
        // Resource Cleanup (Automatic with RAII)
        // ---------------------------------------------------------------
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    print_header("SECURE STORAGE COMPLETE");
    std::cout << "Secure storage pipeline executed successfully!" << std::endl;
    
    return EXIT_SUCCESS;
}