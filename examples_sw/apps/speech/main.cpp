/**
 * Speech Recognition Pipeline 
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
#include <cmath>

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
constexpr auto const defSize = 32;  // Default: single 32-point FFT
constexpr auto const nBenchRuns = 1;

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
        ("size,s", boost::program_options::value<uint32_t>(), "Total samples (must be multiple of 32)")
        ("reps,n", boost::program_options::value<uint32_t>(), "Number of repetitions");
    
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
    uint32_t size = defSize;
    
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
    if(commandLineArgs.count("size") > 0) size = commandLineArgs["size"].as<uint32_t>();

    // Ensure size is a multiple of 32 (FFT requirement)
    if (size % 32 != 0) {
        std::cout << "Warning: Size must be multiple of 32. Adjusting " << size 
                  << " to " << ((size + 31) / 32) * 32 << std::endl;
        size = ((size + 31) / 32) * 32;
    }

    // Calculate buffer requirements correctly
    uint32_t num_ffts = size / 32;  // Number of 32-point FFTs
    
    // Input: complex (interleaved real/imaginary) for FFT
    uint32_t complex_samples = 2 * size;  // Each sample becomes [real, imag]
    uint32_t input_buffer_size = complex_samples * sizeof(float);
    
    // Intermediate: FFT results (same size as input, complex output)
    uint32_t fft_output_size = input_buffer_size;
    
    // Output: one classification per 32-point FFT
    uint32_t svm_output_size = num_ffts * sizeof(float);

    print_header("PARAMS");
    std::cout << "Number of regions: " << n_regions << std::endl;
    std::cout << "Hugepages: " << huge << std::endl;
    std::cout << "Mapped pages: " << mapped << std::endl;
    std::cout << "Streaming: " << (stream ? "HOST" : "CARD") << std::endl;
    std::cout << "Number of repetitions (thr): " << n_reps_thr << std::endl;
    std::cout << "Number of repetitions (lat): " << n_reps_lat << std::endl;
    std::cout << "Total samples: " << size << " (" << num_ffts << " x 32-point FFTs)" << std::endl;
    std::cout << "Input buffer size: " << input_buffer_size << " bytes" << std::endl;
    std::cout << "FFT output size: " << fft_output_size << " bytes" << std::endl;
    std::cout << "SVM output size: " << svm_output_size << " bytes" << std::endl;
    std::cout << "Expected classifications: " << num_ffts << std::endl;

    try {
        // ---------------------------------------------------------------
        // Dataflow Setup using ushell's fluent API
        // ---------------------------------------------------------------
        print_header("DATAFLOW SETUP");
        
        // Create an FFT + SVM dataflow
        Dataflow speech_dataflow("speech_dataflow");
        
        // Create processing tasks
        Task& fft_processor = speech_dataflow.add_task("fft_processor", "signal_processing");
        Task& svm_classifier = speech_dataflow.add_task("svm_classifier", "machine_learning");
        
        // Create buffers
        Buffer& signal_input_buffer = speech_dataflow.add_buffer(input_buffer_size, "signal_input_buffer");
        Buffer& fft_output_buffer = speech_dataflow.add_buffer(fft_output_size, "fft_output_buffer");
        Buffer& classification_buffer = speech_dataflow.add_buffer(svm_output_size, "classification_buffer");
        
        // Set up the FFT + SVM pipeline using fluent API
        speech_dataflow.to(signal_input_buffer, fft_processor.in)
                       .to(fft_processor.out, fft_output_buffer)
                       .to(fft_output_buffer, svm_classifier.in)
                       .to(svm_classifier.out, classification_buffer);
        
        std::cout << "Creating speech recognition dataflow:" << std::endl;
        std::cout << "  signal_input_buffer → fft_processor → fft_output_buffer → svm_classifier → classification_buffer" << std::endl;
        
        // Check and build the dataflow
        if (!speech_dataflow.check()) {
            throw std::runtime_error("Failed to validate dataflow");
        }
        
        // ---------------------------------------------------------------
        // Data Generation and Buffer Initialization
        // ---------------------------------------------------------------
        print_header("DATA GENERATION");
        
        // Generate test data that will produce known SVM results after FFT
        // Use the same base pattern that works in your SVM-only test, but apply it per FFT
        float base_pattern[32] = {
            36257662.0f, 70308074.0f, 162763557.0f, 109956489.0f, 86125933.0f,
            35535698.0f, 5473712.0f, 2191429.0f, 1655529.0f, 1427210.0f,
            1819290.0f, 2594503.0f, 2649694.0f, 849597.0f, 765175.0f,
            890647.0f, 1405332.0f, 1132804.0f, 5776375.0f, 18372118.0f,
            6724989.0f, 7313762.0f, 1469345.0f, 1744257.0f, 1631652.0f,
            823229.0f, 1018674.0f, 639273.0f, 601445.0f, 630083.0f,
            678854.0f, 636897.0f
        };

        // Show first FFT input data
        std::cout << "\nFirst 32-point FFT input (complex format):\n";
        for (int i = 0; i < 16; ++i) {  // Show first 16 complex pairs
            std::cout << std::fixed << std::setprecision(1) 
                      << base_pattern[i] << "+0.0i ";
            if ((i + 1) % 4 == 0) std::cout << "\n";
        }
        std::cout << "... (remaining 16 samples omitted)\n\n";

        // Create input data vector
        std::vector<float> input_data(complex_samples);
        
        // Fill complex data for FFT input
        // Each FFT gets a different variant of the base pattern
        for(int fft_idx = 0; fft_idx < num_ffts; fft_idx++) {
            for(int sample = 0; sample < 32; sample++) {
                int complex_base = (fft_idx * 32 + sample) * 2;
                
                // Real part: base pattern + offset per FFT
                input_data[complex_base] = base_pattern[sample] + (fft_idx * 1000.0f);
                
                // Imaginary part: zero (pure real signal)
                input_data[complex_base + 1] = 0.0f;
            }
        }
        
        // Write input data to buffer
        write_dataflow_buffer(signal_input_buffer, input_data.data(), input_buffer_size);
        std::cout << "Initialized input buffer with " << size << " complex samples (" << num_ffts << " FFTs)" << std::endl;
        
        // ---------------------------------------------------------------
        // Performance Benchmarking
        // ---------------------------------------------------------------
        print_header("SPEECH RECOGNITION PROCESSING");
        
        // Create benchmark object
        cBench bench(nBenchRuns);
        
        speech_dataflow.clear_completed();
        
        auto benchmark_thr = [&]() {
            for (int i = 0; i < n_reps_lat; i++) {
                speech_dataflow.execute();
            }
        };

        bench.runtime(benchmark_thr);

        // Print latency statistics
        print_header("LATENCY MEASUREMENTS");
        printLatencyStats(bench.getAvg() / n_reps_lat, input_buffer_size, n_reps_lat);


        /*
        // ---------------------------------------------------------------
        // Results Verification
        // ---------------------------------------------------------------
        print_header("RESULTS");
        
        // Read the classification results
        std::vector<float> classification_results(num_ffts);
        read_dataflow_buffer(classification_buffer, classification_results.data(), svm_output_size);
        
        // Print all classification results
        std::cout << "Classification results:\n";
        for (uint32_t fft_idx = 0; fft_idx < num_ffts; fft_idx++) {
            float result = classification_results[fft_idx];
            std::cout << "  FFT " << (fft_idx + 1) << "/" << num_ffts << ": " << result;
            
            // Map result to class label if it matches known values
            uint32_t result_bits = *(uint32_t*)&result;
            switch(result_bits) {
                case 0x40800000: std::cout << " (Class 4)"; break;
                case 0x3F800000: std::cout << " (Class 1)"; break;
                case 0x40000000: std::cout << " (Class 2)"; break;
                case 0x40400000: std::cout << " (Class 3)"; break;
                case 0x40A00000: std::cout << " (Class 5)"; break;
                default: 
                    if (result == 0.0f) std::cout << " (No result - check pipeline)";
                    else std::cout << " (Unknown class)";
                    break;
            }
            std::cout << std::endl;
        }
        */
        // ---------------------------------------------------------------
        // Resource Cleanup (Automatic with RAII)
        // ---------------------------------------------------------------
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    print_header("SPEECH RECOGNITION PROCESSING COMPLETE");
    std::cout << "Speech recognition pipeline executed successfully!" << std::endl;

    return EXIT_SUCCESS;
}