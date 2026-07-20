/**
 * SHA2
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

// Include high-level ushell API
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
constexpr auto const defReps = 1;
constexpr auto const defSize = 64 * 1024;  // 64KB default
constexpr auto const SHA256_DIGEST_LENGTH = 32;  // SHA256 produces 256-bit (32-byte) hash

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

// Generate test data with constant pattern
void generateTestData(uint32_t* buffer, size_t size_bytes) {
    // Fill with constant chunk pattern matching the hardware expectation
    // This matches CONSTANT_CHUNK = 512'hFEDCBA9876543210FEDCBA9876543210...
    size_t num_words = size_bytes / 4;
    for (size_t j = 0; j < num_words; j += 2) {
        // Fill two 32-bit words at a time to match the 64-bit pattern
        buffer[j] = 0x76543210;
        if (j+1 < num_words) {
            buffer[j+1] = 0xFEDCBA98;
        }
    }
}

// Print SHA256 hash result
void printHashResult(unsigned char* hash, int index) {
    std::cout << "Hash " << index << ": ";
    for(int j = 0; j < SHA256_DIGEST_LENGTH; j++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                 << static_cast<int>(hash[j]);
    }
    std::cout << std::dec << std::endl;
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
    boost::program_options::options_description programDescription("SHA256 Test Options:");
    programDescription.add_options()
        ("bitstream,b", boost::program_options::value<string>(), "Shell bitstream")
        ("device,d", boost::program_options::value<uint32_t>(), "Target device")
        ("size,s", boost::program_options::value<uint32_t>(), "Data size")
        ("reps,r", boost::program_options::value<uint32_t>(), "Number of reps")
        ("help", "Show help");
    
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
    uint32_t size = defSize;
    uint32_t n_reps = defReps;

    // Process command line arguments
    if(commandLineArgs.count("bitstream") > 0) { 
        bstream_path = commandLineArgs["bitstream"].as<string>();
        
        std::cout << std::endl << "Shell loading (path: " << bstream_path << ") ..." << std::endl;
        cRnfg crnfg(cs_dev);
        crnfg.shellReconfigure(bstream_path);
    }
    if(commandLineArgs.count("device") > 0) cs_dev = commandLineArgs["device"].as<uint32_t>();
    if(commandLineArgs.count("size") > 0) size = commandLineArgs["size"].as<uint32_t>();
    if(commandLineArgs.count("reps") > 0) n_reps = commandLineArgs["reps"].as<uint32_t>();

    print_header("PARAMS");
    std::cout << "Pipeline: SHA256 Hashing" << std::endl;
    std::cout << "Data size: " << size << " bytes" << std::endl;
    std::cout << "Number of reps: " << n_reps << std::endl;
    std::cout << "Hash size: " << SHA256_DIGEST_LENGTH << " bytes per hash" << std::endl;

    try {
        // ---------------------------------------------------------------
        // Dataflow Setup using ushell's fluent API
        // ---------------------------------------------------------------
        print_header("DATAFLOW SETUP");
        
        // Create SHA256 dataflow
        Dataflow sha_dataflow("sha256_hashing_pipeline");
        
        // Create processing task
        Task& sha_processor = sha_dataflow.add_task("sha256_processor", "hashing");
        
        // Create buffers
        Buffer& input_buffer = sha_dataflow.add_buffer(size, "input_buffer");
        Buffer& hash_buffer = sha_dataflow.add_buffer(SHA256_DIGEST_LENGTH, "hash_buffer");
        
        // Set up the SHA256 pipeline using fluent API
        sha_dataflow.to(input_buffer, sha_processor.in)
                    .to(sha_processor.out, hash_buffer);
        
        std::cout << "Creating SHA256 dataflow:" << std::endl;
        std::cout << "  input_buffer → sha256_processor → hash_buffer" << std::endl;
        
        // Check and build the dataflow
        if (!sha_dataflow.check()) {
            throw std::runtime_error("Failed to validate dataflow");
        }
        
        // ---------------------------------------------------------------
        // Data Generation
        // ---------------------------------------------------------------
        print_header("DATA GENERATION");
        
        // Generate test data
        std::vector<uint32_t> test_data(size / sizeof(uint32_t));
        generateTestData(test_data.data(), size);
        
        std::cout << "Generated " << size << " bytes of test data with constant pattern" << std::endl;
        std::cout << "Pattern: 0xFEDCBA9876543210 repeated" << std::endl;
        
        // ---------------------------------------------------------------
        // Performance Benchmarking
        // ---------------------------------------------------------------
        print_header("SHA256 HASHING");
        
        // Create benchmark object
        cBench bench(n_reps);
        
        // Store hash results
        std::vector<std::vector<unsigned char>> hash_results(n_reps);
        for(int i = 0; i < n_reps; i++) {
            hash_results[i].resize(SHA256_DIGEST_LENGTH);
        }
        
        auto benchmark_thr = [&]() {
            for(int i = 0; i < n_reps; i++) {
                // Write input data
                write_dataflow_buffer(input_buffer, test_data.data(), size);
                
                // Clear completion counters
                sha_dataflow.clear_completed();
                
                // Execute the dataflow
                sha_dataflow.execute(size);
                
                // Read hash result
                read_dataflow_buffer(hash_buffer, hash_results[i].data(), SHA256_DIGEST_LENGTH);
                
                if(stalled.load()) throw std::runtime_error("Stalled, SIGINT caught");
            }
        };
        
        bench.runtime(benchmark_thr);
        
        // ---------------------------------------------------------------
        // Results and Performance
        // ---------------------------------------------------------------
        print_header("LATENCY MEASUREMENTS");
        printLatencyStats(bench.getAvg() / n_reps, size, n_reps);
        
        print_header("RESULTS");
        
        // Print hash results
        std::cout << "SHA256 Hash Results:" << std::endl;
        for(int i = 0; i < n_reps; i++) {
            printHashResult(hash_results[i].data(), i);
        }
        
        // Verify consistency (all hashes should be the same for same input)
        if(n_reps > 1) {
            bool all_match = true;
            for(int i = 1; i < n_reps; i++) {
                if(memcmp(hash_results[0].data(), hash_results[i].data(), SHA256_DIGEST_LENGTH) != 0) {
                    all_match = false;
                    break;
                }
            }
            std::cout << "\nHash consistency: " << (all_match ? "PASSED" : "FAILED") << std::endl;
        }
        
        // Print debug information if needed
        if (sha_dataflow.get_debug_level() > 1) {
            print_header("DEBUG INFO");
            sha_dataflow.print_graph();
            sha_dataflow.print_connections();
            sha_dataflow.print_capability_tree();
        }
        
        // ---------------------------------------------------------------
        // Resource Cleanup (Automatic with RAII)
        // ---------------------------------------------------------------
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    print_header("SHA256 TEST COMPLETE");
    std::cout << "SHA256 hashing pipeline executed successfully!" << std::endl;
    
    return EXIT_SUCCESS;
}