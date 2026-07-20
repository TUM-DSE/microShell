/**
 * RSA 
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
constexpr auto const defSize = 32;     // 256-bit input size in bytes
constexpr auto const RSA_OUTPUT_SIZE = 32;  // 256-bit RSA output (in bytes)

// Structure to hold 256-bit value as 8 32-bit words
struct BigInt256 {
    uint32_t words[8];
    
    // Constructor for hex string
    BigInt256(const std::string& hexStr) {
        std::string paddedHex = hexStr;
        // Remove 0x if present
        if(paddedHex.substr(0,2) == "0x") {
            paddedHex = paddedHex.substr(2);
        }
        // Pad to 64 hex chars (256 bits)
        paddedHex.insert(0, 64 - paddedHex.length(), '0');
        
        // Parse into words
        for(int i = 0; i < 8; i++) {
            words[7-i] = std::stoul(paddedHex.substr(i*8, 8), nullptr, 16);
        }
    }
    
    // Constructor for default value
    BigInt256() {
        memset(words, 0, sizeof(words));
    }
};

// Helper function to print hex values
void printHexBuffer(uint32_t* buffer, size_t words, const char* label) {
    std::cout << label << ": 0x";
    for(int i = words-1; i >= 0; i--) {
        std::cout << std::hex << std::setw(8) << std::setfill('0') << buffer[i];
    }
    std::cout << std::dec << std::endl;
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
    // Signal handler setup
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gotInt;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // ---------------------------------------------------------------
    // Command Line Arguments
    // ---------------------------------------------------------------
    boost::program_options::options_description programDescription("RSA Test Options:");
    programDescription.add_options()
        ("bitstream,b", boost::program_options::value<string>(), "Shell bitstream")
        ("device,d", boost::program_options::value<uint32_t>(), "Target device")
        ("input,i", boost::program_options::value<std::string>(), "Input value (256-bit hex)")
        ("reps,r", boost::program_options::value<uint32_t>(), "Number of repetitions")
        ("help", "Show help");
    
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
    uint32_t n_reps = defReps;
    
    // Process command line arguments
    if(commandLineArgs.count("bitstream") > 0) { 
        bstream_path = commandLineArgs["bitstream"].as<string>();
        
        std::cout << std::endl << "Shell loading (path: " << bstream_path << ") ..." << std::endl;
        cRnfg crnfg(cs_dev);
        crnfg.shellReconfigure(bstream_path);
    }
    if(commandLineArgs.count("device") > 0) cs_dev = commandLineArgs["device"].as<uint32_t>();
    if(commandLineArgs.count("reps") > 0) n_reps = commandLineArgs["reps"].as<uint32_t>();

    // Test values
    BigInt256 inputValue;
    if(commandLineArgs.count("input") > 0) {
        inputValue = BigInt256(commandLineArgs["input"].as<std::string>());
    } else {
        // Default test pattern - using a meaningful 256-bit test value
        inputValue = BigInt256("0xE5E5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5A5");
    }

    print_header("RSA TEST");
    std::cout << "Input size: " << defSize << " bytes (256-bit)" << std::endl;
    std::cout << "Output size: " << RSA_OUTPUT_SIZE << " bytes (256-bit)" << std::endl;
    std::cout << "Number of reps: " << n_reps << std::endl;

    try {
        // ---------------------------------------------------------------
        // Dataflow Setup using ushell's fluent API
        // ---------------------------------------------------------------
        print_header("DATAFLOW SETUP");
        
        // Create RSA dataflow
        Dataflow rsa_dataflow("rsa_encryption_pipeline");
        
        // Create processing task
        Task& rsa_processor = rsa_dataflow.add_task("rsa_processor", "rsa");
        
        // Create buffers
        Buffer& input_buffer = rsa_dataflow.add_buffer(defSize, "input_buffer");
        Buffer& output_buffer = rsa_dataflow.add_buffer(RSA_OUTPUT_SIZE, "output_buffer");
        
        // Set up the RSA pipeline using fluent API
        rsa_dataflow.to(input_buffer, rsa_processor.in)
                    .to(rsa_processor.out, output_buffer);
        
        std::cout << "Creating RSA dataflow:" << std::endl;
        std::cout << "  input_buffer → rsa_processor → output_buffer" << std::endl;
        
        // Check and build the dataflow
        if (!rsa_dataflow.check()) {
            throw std::runtime_error("Failed to validate dataflow");
        }
        
        // ---------------------------------------------------------------
        // Performance Benchmarking
        // ---------------------------------------------------------------
        print_header("RSA PROCESSING");
        
        // Print input value
        printHexBuffer(inputValue.words, defSize/4, "Input ");
        
        // Create benchmark object
        cBench bench(n_reps);
        
        // Store output data for verification
        std::vector<std::vector<uint32_t>> output_data(n_reps);
        for(int i = 0; i < n_reps; i++) {
            output_data[i].resize(RSA_OUTPUT_SIZE / sizeof(uint32_t));
        }
        
        auto benchmark_thr = [&]() {
            for(int i = 0; i < n_reps; i++) {
                // Write input data
                write_dataflow_buffer(input_buffer, inputValue.words, defSize);
                
                // Clear completion counters
                rsa_dataflow.clear_completed();
                
                // Execute the dataflow
                rsa_dataflow.execute(defSize);
                
                // Read output data
                read_dataflow_buffer(output_buffer, output_data[i].data(), RSA_OUTPUT_SIZE);
                
                if(stalled.load()) throw std::runtime_error("Stalled, SIGINT caught");
            }
        };
        
        bench.runtime(benchmark_thr);
        
        // ---------------------------------------------------------------
        // Results
        // ---------------------------------------------------------------
        print_header("RESULTS");
        
        // Print output for each repetition
        for(int i = 0; i < n_reps; i++) {
            if(n_reps > 1) {
                std::cout << "\nRep " << i << ":" << std::endl;
            }
            printHexBuffer(output_data[i].data(), RSA_OUTPUT_SIZE/4, "Output");
        }
        
        // Print performance metrics using printLatencyStats
        print_header("LATENCY MEASUREMENTS");
        printLatencyStats(bench.getAvg() / n_reps, defSize, n_reps);
        
        // Print debug information if needed
        if (rsa_dataflow.get_debug_level() > 1) {
            print_header("DEBUG INFO");
            rsa_dataflow.print_graph();
            rsa_dataflow.print_connections();
            rsa_dataflow.print_capability_tree();
        }
        
        // ---------------------------------------------------------------
        // Resource Cleanup (Automatic with RAII)
        // ---------------------------------------------------------------
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    print_header("RSA TEST COMPLETE");
    std::cout << "RSA encryption pipeline executed successfully!" << std::endl;
    
    return EXIT_SUCCESS;
}