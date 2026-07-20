/**
 * Secure Storage Pipeline (Monolithic)
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
constexpr auto const defSize = 64 * 1024;  // 64KB default
constexpr auto const maxSize = 1024 * 1024;  // Max: 1MB
constexpr auto const nBenchRuns = 1;

/* Pattern types */
enum PatternType {
    PATTERN_REGULAR = 0,    // Groups of 4 identical bytes (2:1 compression)
    PATTERN_RANDOM = 1,     // Random data (worst case)
    PATTERN_LONG_RUNS = 2   // Long runs (best case)
};

// Generate test patterns for RLE
void generatePattern(uint8_t* buffer, size_t size, PatternType pattern) {
    switch(pattern) {
        case PATTERN_REGULAR: // Groups of 4 identical bytes for 2:1 compression
            for (size_t i = 0; i < size; i++) {
                buffer[i] = 'A' + ((i / 4) % 16);
            }
            break;
            
        case PATTERN_RANDOM: // Random bytes (no compression)
            srand(time(NULL));
            for (size_t i = 0; i < size; i++) {
                buffer[i] = rand() % 256;
            }
            break;
            
        case PATTERN_LONG_RUNS: // Long runs for maximum compression
            srand(time(NULL));
            for (size_t pos = 0; pos < size; ) {
                uint8_t value = 'A' + (rand() % 26);
                int run_len = 16 + (rand() % 48);
                if (pos + run_len > size) run_len = size - pos;
                memset(buffer + pos, value, run_len);
                pos += run_len;
            }
            break;
    }
}

// Analyze pipeline output
void analyzePipelineOutput(uint8_t* buffer, size_t buffer_size, size_t input_size) {
    std::cout << "\nPipeline Output Analysis:" << std::endl;
    
    // Check first chunk (0-63)
    std::cout << "  First chunk (RLE output):" << std::endl;
    bool first_chunk_is_rle = false;
    int rle_pairs = 0;
    
    for (int i = 0; i < 64 && i < buffer_size; i += 2) {
        uint8_t count = buffer[i];
        uint8_t value = buffer[i + 1];
        
        if (count > 0 && count <= 64) {
            if (rle_pairs < 8) {  // Show first few pairs
                std::cout << "    RLE pair: (" << (int)count << ", ";
                if (value >= 32 && value <= 126) {
                    std::cout << "'" << (char)value << "'";
                } else {
                    std::cout << "0x" << std::hex << (int)value << std::dec;
                }
                std::cout << ")" << std::endl;
            }
            rle_pairs++;
            first_chunk_is_rle = true;
        }
    }
    
    if (first_chunk_is_rle) {
        std::cout << "    Total RLE pairs in first chunk: " << rle_pairs << std::endl;
    }
    
    // Check subsequent chunks for encryption
    std::cout << "\n  Subsequent chunks (encrypted data):" << std::endl;
    std::cout << "    Bytes 64-127: ";
    
    int encrypted_bytes = 0;
    bool looks_random = true;
    
    for (int i = 64; i < std::min((int)buffer_size, 128); i++) {
        if (i % 16 == 0 && i > 64) std::cout << std::endl << "                   ";
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[i] << " ";
        
        if (buffer[i] != 0) encrypted_bytes++;
        
        // Check for obvious patterns (like repeated 16-byte blocks)
        if (i >= 80 && i % 16 == 15) {
            if (memcmp(&buffer[i-15], &buffer[i-31], 16) == 0) {
                looks_random = false;
            }
        }
    }
    std::cout << std::dec << std::endl;
    
    // Count total non-zero bytes
    size_t total_non_zero = 0;
    for (size_t i = 0; i < buffer_size; i++) {
        if (buffer[i] != 0) total_non_zero++;
    }
    
    // Summary
    std::cout << "\n  Pipeline behavior:" << std::endl;
    std::cout << "    First chunk: " << (first_chunk_is_rle ? "RLE compression" : "Unknown") << std::endl;
    std::cout << "    Rest: " << (encrypted_bytes > 0 ? "Encrypted" : "Empty") << std::endl;
    std::cout << "    Encryption quality: " << (looks_random ? "Good (random-looking)" : "Poor (patterns detected)") << std::endl;
    std::cout << "    Total output bytes: " << total_non_zero << std::endl;
    std::cout << "    Compression ratio: " << std::fixed << std::setprecision(2) 
              << (double)input_size / total_non_zero << ":1" << std::endl;
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

int main(int argc, char *argv[]) {
    // Signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gotInt;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // Parse arguments
    boost::program_options::options_description programDescription("Secure Storage Pipeline Test:");
    programDescription.add_options()
        ("help,h", "Show help")
        ("bitstream,b", boost::program_options::value<string>(), "Shell bitstream")
        ("device,d", boost::program_options::value<uint32_t>(), "Target device")
        ("regions,g", boost::program_options::value<uint32_t>(), "Number of vFPGAs")
        ("size,s", boost::program_options::value<uint32_t>(), "Data size in bytes (default: 64KB)")
        ("reps,r", boost::program_options::value<uint32_t>(), "Number of repetitions (default: 1)")
        ("pattern,p", boost::program_options::value<int>(), 
         "Pattern: 0=regular (2:1), 1=random, 2=long runs (default: 0)")
        ("hugepages,h", boost::program_options::value<bool>(), "Use hugepages (default: true)")
        ("mapped,m", boost::program_options::value<bool>(), "Use mapped memory (default: true)")
        ("stream,t", boost::program_options::value<bool>(), "Use streaming interface (default: true)")
        ("verbose,v", "Show detailed output");
    
    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, programDescription), 
        commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    if(commandLineArgs.count("help")) {
        std::cout << programDescription << std::endl;
        std::cout << "\nPipeline characteristics:" << std::endl;
        std::cout << "  - First 64-byte chunk: RLE compression only" << std::endl;
        std::cout << "  - Remaining chunks: RLE compression + AES encryption" << std::endl;
        std::cout << "  - No explicit descriptor handling needed" << std::endl;
        return EXIT_SUCCESS;
    }

    // Parse arguments with defaults
    string bstream_path = "";
    uint32_t cs_dev = defDevice;
    uint32_t n_regions = nRegions;
    uint32_t data_size = commandLineArgs.count("size") ? 
        commandLineArgs["size"].as<uint32_t>() : defSize;
    uint32_t n_reps = commandLineArgs.count("reps") ? 
        commandLineArgs["reps"].as<uint32_t>() : nRepsLat;
    PatternType pattern = static_cast<PatternType>(
        commandLineArgs.count("pattern") ? commandLineArgs["pattern"].as<int>() : 0);
    bool huge = defHuge;
    bool mapped = defMapped;
    bool stream = defStream;
    bool verbose = commandLineArgs.count("verbose") > 0;
    
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
    
    // Validate and align size
    if (data_size == 0 || data_size > maxSize) {
        std::cerr << "Error: Invalid size (max: " << maxSize << " bytes)" << std::endl;
        return EXIT_FAILURE;
    }
    
    // Ensure data size is multiple of 64 bytes
    data_size = ((data_size + 63) / 64) * 64;
    uint32_t chunks = data_size / 64;
    
    // Calculate expected sizes for pipeline
    uint32_t pipeline_output_size = data_size;  // Same size output for encrypted data

    // Print configuration
    print_header("SECURE STORAGE PIPELINE TEST");
    std::cout << "Pipeline: RLE Compression → RSA Encryption" << std::endl;
    std::cout << "Number of regions: " << n_regions << std::endl;
    std::cout << "Input size: " << data_size << " bytes (" << chunks << " chunks)" << std::endl;
    std::cout << "Pattern: " << (pattern == PATTERN_REGULAR ? "Regular (2:1 compression)" : 
                                pattern == PATTERN_RANDOM ? "Random (no compression)" : 
                                "Long runs (best compression)") << std::endl;
    std::cout << "Output buffer size: " << pipeline_output_size << " bytes" << std::endl;
    std::cout << "Repetitions: " << n_reps << std::endl;
    std::cout << "Hugepages: " << huge << std::endl;
    std::cout << "Mapped: " << mapped << std::endl;
    std::cout << "Streaming: " << (stream ? "HOST" : "CARD") << std::endl;
    
    try {
        // ---------------------------------------------------------------
        // Dataflow Setup using ushell's fluent API
        // ---------------------------------------------------------------
        print_header("DATAFLOW SETUP");
        
        // Create secure storage dataflow
        Dataflow secure_dataflow("secure_pipeline");
        
        // Create processing tasks
        // Note: In the combined pipeline, both operations happen in the same vFPGA
        Task& secure_processor = secure_dataflow.add_task("secure_processor", "secure");
        
        // Create buffers
        Buffer& input_buffer = secure_dataflow.add_buffer(data_size, "input_buffer");
        Buffer& output_buffer = secure_dataflow.add_buffer(pipeline_output_size, "output_buffer");
        
        // Set up the pipeline using fluent API
        secure_dataflow.to(input_buffer, secure_processor.in)
                              .to(secure_processor.out, output_buffer);
        
        std::cout << "Creating secure storage dataflow:" << std::endl;
        std::cout << "  input_buffer → secure_processor → output_buffer" << std::endl;
        
        // Check and build the dataflow
        if (!secure_dataflow.check()) {
            throw std::runtime_error("Failed to validate dataflow");
        }
        
        // ---------------------------------------------------------------
        // Data Generation and Buffer Initialization
        // ---------------------------------------------------------------
        print_header("DATA GENERATION");
        
        // Generate test data for all repetitions
        std::vector<std::vector<uint8_t>> test_data(n_reps);
        std::vector<std::vector<uint8_t>> result_data(n_reps);
        
        for(int i = 0; i < n_reps; i++) {
            test_data[i].resize(data_size);
            result_data[i].resize(pipeline_output_size);
            
            // Generate pattern
            generatePattern(test_data[i].data(), data_size, pattern);
        }
        
        // Show input sample if verbose
        if (verbose) {
            print_header("INPUT DATA SAMPLE");
            std::cout << "First 128 bytes:" << std::endl;
            for(int i = 0; i < std::min(data_size, 128u); i++) {
                if (i % 64 == 0) std::cout << std::endl << "Chunk " << (i/64) << ": ";
                uint8_t val = test_data[0][i];
                std::cout << (char)((val >= 32 && val <= 126) ? val : '.');
            }
            std::cout << std::endl;
        }
        
        // ---------------------------------------------------------------
        // Performance Benchmarking
        // ---------------------------------------------------------------
        print_header("EXECUTING PIPELINE");
        
        // Create benchmark object
        cBench bench(n_reps);
        
        std::cout << "Starting transfers..." << std::endl;
        
        auto benchmark_thr = [&]() {
            for(int i = 0; i < n_reps; i++) {
                // Write input data
                write_dataflow_buffer(input_buffer, test_data[i].data(), data_size);
                
                // Clear completion counters
                secure_dataflow.clear_completed();
                
                // Execute the dataflow
                secure_dataflow.execute(data_size);
                
                // Read output data
                read_dataflow_buffer(output_buffer, result_data[i].data(), pipeline_output_size);
            }
        };
        
        bench.runtime(benchmark_thr);
        std::cout << "All transfers completed!" << std::endl;
        
        // ---------------------------------------------------------------
        // Results Analysis
        // ---------------------------------------------------------------
        print_header("RESULTS");
        
        for(int i = 0; i < n_reps; i++) {
            if (n_reps > 1) std::cout << "\n--- Repetition " << (i+1) << " ---" << std::endl;
            analyzePipelineOutput(result_data[i].data(), pipeline_output_size, data_size);
        }
        
        // ---------------------------------------------------------------
        // Performance Metrics
        // ---------------------------------------------------------------
        print_header("LATENCY MEASUREMENTS");
        printLatencyStats(bench.getAvg() / n_reps, data_size, n_reps);
        
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