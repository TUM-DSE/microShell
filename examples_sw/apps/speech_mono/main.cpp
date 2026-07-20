/**
 * Speech Recognition Pipeline (Monolithic)
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
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gotInt;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // ---------------------------------------------------------------
    // Command Line Arguments
    // ---------------------------------------------------------------
    boost::program_options::options_description programDescription("Speech Recognition Pipeline Options:");
    programDescription.add_options()
        ("bitstream,b", boost::program_options::value<string>(), "Shell bitstream")
        ("device,d", boost::program_options::value<uint32_t>(), "Target device")
        ("regions,g", boost::program_options::value<uint32_t>(), "Number of vFPGAs")
        ("size,s", boost::program_options::value<uint32_t>(), "Total samples (must be multiple of 32)")
        ("reps,r", boost::program_options::value<uint32_t>(), "Number of repetitions")
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
    uint32_t size = defSize;
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
    if(commandLineArgs.count("size") > 0) size = commandLineArgs["size"].as<uint32_t>();
    if(commandLineArgs.count("reps") > 0) n_reps = commandLineArgs["reps"].as<uint32_t>();
    if(commandLineArgs.count("hugepages") > 0) huge = commandLineArgs["hugepages"].as<bool>();
    if(commandLineArgs.count("mapped") > 0) mapped = commandLineArgs["mapped"].as<bool>();
    if(commandLineArgs.count("stream") > 0) stream = commandLineArgs["stream"].as<bool>();

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
    std::cout << "Pipeline: FFT → SVM Classification" << std::endl;
    std::cout << "Number of regions: " << n_regions << std::endl;
    std::cout << "Total samples: " << size << " (" << num_ffts << " x 32-point FFTs)" << std::endl;
    std::cout << "Input buffer size: " << input_buffer_size << " bytes" << std::endl;
    std::cout << "FFT output size: " << fft_output_size << " bytes" << std::endl;
    std::cout << "SVM output size: " << svm_output_size << " bytes" << std::endl;
    std::cout << "Expected classifications: " << num_ffts << std::endl;
    std::cout << "Number of reps: " << n_reps << std::endl;
    std::cout << "Hugepages: " << huge << std::endl;
    std::cout << "Mapped: " << mapped << std::endl;
    std::cout << "Streaming: " << (stream ? "HOST" : "CARD") << std::endl;

    try {
        // ---------------------------------------------------------------
        // Dataflow Setup using ushell's fluent API
        // ---------------------------------------------------------------
        print_header("DATAFLOW SETUP");
        
        // Create speech recognition dataflow
        Dataflow speech_dataflow("speech_recognition_pipeline");
        
        // Create processing tasks
        Task& fft_processor = speech_dataflow.add_task("fft_processor", "fft");
        Task& svm_classifier = speech_dataflow.add_task("svm_classifier", "classification");
        
        // Create buffers
        Buffer& audio_input_buffer = speech_dataflow.add_buffer(input_buffer_size, "audio_input_buffer");
        Buffer& fft_output_buffer = speech_dataflow.add_buffer(fft_output_size, "fft_output_buffer");
        Buffer& classification_buffer = speech_dataflow.add_buffer(svm_output_size, "classification_buffer");
        
        // Set up the speech recognition pipeline using fluent API
        speech_dataflow.to(audio_input_buffer, fft_processor.in)
                      .to(fft_processor.out, fft_output_buffer)
                      .to(fft_output_buffer, svm_classifier.in)
                      .to(svm_classifier.out, classification_buffer);
        
        std::cout << "Creating speech recognition dataflow:" << std::endl;
        std::cout << "  audio_input_buffer → fft_processor → fft_output_buffer → svm_classifier → classification_buffer" << std::endl;
        
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

        // Generate test data for all repetitions
        std::vector<std::vector<float>> test_data(n_reps);
        std::vector<std::vector<float>> result_data(n_reps);
        
        for(int i = 0; i < n_reps; i++) {
            test_data[i].resize(complex_samples);
            result_data[i].resize(num_ffts);
            
            // Fill complex data for FFT input
            // Each FFT gets a different variant of the base pattern
            for(int fft_idx = 0; fft_idx < num_ffts; fft_idx++) {
                for(int sample = 0; sample < 32; sample++) {
                    int complex_base = (fft_idx * 32 + sample) * 2;
                    
                    // Real part: base pattern + offset per FFT
                    test_data[i][complex_base] = base_pattern[sample] + (fft_idx * 1000.0f);
                    
                    // Imaginary part: zero (pure real signal)
                    test_data[i][complex_base + 1] = 0.0f;
                }
            }
        }
        
        // ---------------------------------------------------------------
        // Performance Benchmarking
        // ---------------------------------------------------------------
        print_header("SPEECH RECOGNITION PROCESSING");
        
        // Create benchmark object
        cBench bench(n_reps);
        
        auto benchmark_thr = [&]() {
            for(int i = 0; i < n_reps; i++) {
                // Write input data
                write_dataflow_buffer(audio_input_buffer, test_data[i].data(), input_buffer_size);
                
                // Clear completion counters
                speech_dataflow.clear_completed();
                
                // Execute the dataflow
                speech_dataflow.execute(input_buffer_size);
                
                // Read output data
                read_dataflow_buffer(classification_buffer, result_data[i].data(), svm_output_size);
            }
        };
        
        bench.runtime(benchmark_thr);

        // ---------------------------------------------------------------
        // Results Verification
        // ---------------------------------------------------------------
        print_header("LATENCY MEASUREMENTS");
        printLatencyStats(bench.getAvg() / n_reps, input_buffer_size, n_reps);

        /*
        print_header("RESULTS");
        // Print all classification results
        for(int i = 0; i < n_reps; i++) {
            std::cout << "Rep " << i << " classification results:\n";
            for (uint32_t fft_idx = 0; fft_idx < num_ffts; fft_idx++) {
                float result = result_data[i][fft_idx];
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
    
    print_header("SPEECH RECOGNITION COMPLETE");
    std::cout << "Speech recognition pipeline executed successfully!" << std::endl;
    
    return EXIT_SUCCESS;
}