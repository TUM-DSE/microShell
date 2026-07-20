/**
 * Digital Signature Pipeline 
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
#include <map>
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

/* Def params */
constexpr auto const defDevice = 0;
constexpr auto const nRegions = 2;
constexpr auto const targetVfid = 0;  
constexpr auto const defReps = 1;
constexpr auto const defMinSize = 8 * 1024;
constexpr auto const defMaxSize = 128 * 1024;
constexpr auto const SHA256_DIGEST_LENGTH = 32;  // SHA256 produces 256-bit (32-byte) hash
constexpr auto const nBenchRuns = 1;
constexpr auto const RSA_OUTPUT_SIZE = 32;  // 256-bit RSA output (in bytes)

// Define expected signatures for different data sizes
const std::map<uint32_t, std::string> expectedSignatures = {
    {32 * 1024, "0df7cd1be029d306e6659b55f528662920ad44045ee08d2f711c7f9eeffcf7fc"},
    {64 * 1024, "0e76911d8183866e508357435109d8348da11959407f8fe42849fe554b00c89a"},
    {128 * 1024, "05f847595f9a76013962e9bf322cf7aa0531af248c898cafd42e7248839b3bd2"},
    {256 * 1024, "0d80761a70dbf3027ef2dfb7b50a7f01615f96367aaf71403bc993baa8f2b99c"},
    {512 * 1024, "09f219ce7f89d489446567fa85e2849a06258a9f9529bf3c4692b11da41578cf"}
};

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
              << (1000000000 * data_size_bytes * n_reps) / (avg_latency_ns * n_reps) 
              << " MB/s" << std::endl;
}

// Function to print header
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

    // Read arguments
    boost::program_options::options_description programDescription("Options:");
    programDescription.add_options()
        ("size,s", boost::program_options::value<uint32_t>(), "Data size")
        ("reps,r", boost::program_options::value<uint32_t>(), "Number of reps");
    
    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    uint32_t n_reps = defReps;
    uint32_t curr_size = defMinSize;
    uint32_t max_size = defMaxSize;

    if(commandLineArgs.count("size") > 0) curr_size = commandLineArgs["size"].as<uint32_t>();
    if(commandLineArgs.count("reps") > 0) n_reps = commandLineArgs["reps"].as<uint32_t>();

    print_header("PARAMS");
    std::cout << "vFPGA ID: " << targetVfid << std::endl;
    std::cout << "Starting transfer size: " << curr_size << std::endl;
    std::cout << "Ending transfer size: " << max_size << std::endl;
    std::cout << "Number of reps: " << n_reps << std::endl;

    try {
        // ---------------------------------------------------------------
        // Dataflow Setup using ushell's fluent API
        // ---------------------------------------------------------------
        print_header("DATAFLOW SETUP");
        
        // Create a dataflow with modern fluent API
        Dataflow digi_sign_dataflow("digi_sign_dataflow");
        
        // Create tasks using fluent interface
        Task& sha256_task = digi_sign_dataflow.add_task("sha256_task", "processing");
        Task& rsa_task = digi_sign_dataflow.add_task("rsa_task", "processing");
        
        // Create buffers with appropriate sizes
        Buffer& input_buffer = digi_sign_dataflow.add_buffer(max_size, "input_buffer");
        Buffer& hash_buffer = digi_sign_dataflow.add_buffer(SHA256_DIGEST_LENGTH, "hash_buffer");
        Buffer& signature_buffer = digi_sign_dataflow.add_buffer(RSA_OUTPUT_SIZE, "signature_buffer");
        
        // Set up the digital signature pipeline using fluent API
        // Flow: input_buffer → sha256_task → hash_buffer → rsa_task → signature_buffer
        digi_sign_dataflow.to(input_buffer, sha256_task.in)
                        .to(sha256_task.out, hash_buffer)
                        .to(hash_buffer, rsa_task.in)
                        .to(rsa_task.out, signature_buffer);
        
        std::cout << "Creating dataflow: input_buffer → sha256_task → hash_buffer → rsa_task → signature_buffer" << std::endl;
        
        // Check and build the dataflow
        if (!digi_sign_dataflow.check()) {
            throw std::runtime_error("Failed to validate dataflow");
        }
        std::cout << "Digital signature dataflow created and validated successfully" << std::endl;
        
        // Optional: Print the graph structure for verification
        digi_sign_dataflow.print_graph();
        
        // ---------------------------------------------------------------
        // Buffer Initialization
        // ---------------------------------------------------------------
        print_header("BUFFER INITIALIZATION");
        
        // Initialize input buffer with test pattern
        void* input_data = malloc(max_size);
        for (int j = 0; j < max_size/4; j += 2) {
            ((uint32_t*)input_data)[j] = 0x76543210;
            if (j+1 < max_size/4) {
                ((uint32_t*)input_data)[j+1] = 0xFEDCBA98;
            }
        }
        
        // Write test data to input buffer using ushell API
        write_dataflow_buffer(input_buffer, input_data, max_size);
        std::cout << "Initialized input buffer with test pattern" << std::endl;
        free(input_data);
        
        // ---------------------------------------------------------------
        // Benchmark Loop
        // ---------------------------------------------------------------
        print_header("DIGITAL SIGNATURE PIPELINE");
        
        // Create a benchmark object
        cBench bench(nBenchRuns);
        
        uint32_t test_size = curr_size;
        while (test_size <= max_size) {
            digi_sign_dataflow.clear_completed();
            
            // Benchmark using ushell's execute method
            auto benchmark_thr = [&]() {
                for (int i = 0; i < n_reps; i++) {
                    digi_sign_dataflow.execute(test_size);
                }
            };
            
            bench.runtime(benchmark_thr);
            
            // Print latency statistics
            print_header("LATENCY MEASUREMENTS");
            printLatencyStats(bench.getAvg() / n_reps, test_size, n_reps);
            std::cout << "Size: " << test_size << ", Latency: " << bench.getAvg() / n_reps << " ns" << std::endl;
            
            test_size *= 2;
        }
        
        // ---------------------------------------------------------------
        // Verification
        // ---------------------------------------------------------------
        print_header("VERIFICATION");
        
        // Read the signature for verification using ushell API
        uint32_t signature_data[RSA_OUTPUT_SIZE/4];
        read_dataflow_buffer(signature_buffer, signature_data, RSA_OUTPUT_SIZE);
        std::string actual_sig = bufferToHexString(signature_data, RSA_OUTPUT_SIZE);
        
        // Compare with expected signature if available
        auto expected_it = expectedSignatures.find(max_size);
        std::cout << "Actual signature: 0x" << actual_sig << std::endl;
        
        if (expected_it != expectedSignatures.end()) {
            std::cout << "Expected signature: 0x" << expected_it->second << std::endl;
            bool matches = (expected_it->second == actual_sig);
            std::cout << (matches ? 
                "\033[32mSIGNATURE MATCH: Output matches expected result!\033[0m" : 
                "\033[31mSIGNATURE MISMATCH: Output does not match expected result!\033[0m") 
                << std::endl;
        } else {
            std::cout << "No expected signature available for comparison at size " << max_size << " bytes." << std::endl;
        }
        
        // ---------------------------------------------------------------
        // Additional Debugging Information
        // ---------------------------------------------------------------
        if (digi_sign_dataflow.get_debug_level() > 0) {
            print_header("DEBUG INFORMATION");
            std::cout << "Dataflow connections:" << std::endl;
            digi_sign_dataflow.print_connections();
            
            // Optional: Print capability tree for advanced debugging
            if (digi_sign_dataflow.get_debug_level() > 1) {
                digi_sign_dataflow.print_capability_tree();
            }
        }
        
        // ---------------------------------------------------------------
        // Resource Cleanup (Automatic with RAII)
        // ---------------------------------------------------------------
        print_header("CLEANUP");
        std::cout << "Resources will be automatically cleaned up by RAII" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}