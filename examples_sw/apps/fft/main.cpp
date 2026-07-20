/**
 * FFT 
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
#include <random>
#include <vector>

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
constexpr auto const defSize = 32;       
constexpr float const sampleRate = 44100.0f;

// Generate sine wave value
float generateSineValue(int index) {
    const float amplitude = 1000.0f;
    const float frequency = 1378.125f;
    const float phase = 0.0f;
    
    float t = (float)index / sampleRate;
    return amplitude * sin(2.0f * M_PI * frequency * t + phase);
}

// Function to print raw FFT results - adjusted for 32-point
void printRawFFT(float* output_ptr, int size) {
    const int nyquistBins = size/2;  // For 32-point: 16 bins
    
    std::cout << "\nRaw FFT Output (32-point FFT):\n";
    std::cout << "Bin\tFrequency (Hz)\tReal\t\tImaginary\tMagnitude\n";
    std::cout << "----------------------------------------------------------------\n";
    
    float freqResolution = sampleRate / 32;  // Always 32-point FFT: 44100/32 = 1378.125 Hz/bin
    
    for(int i = 0; i < 16; i++) {  // Always show 16 bins for 32-point FFT
        float frequency = i * freqResolution;
        float real = output_ptr[2*i];
        float imag = output_ptr[2*i+1];
        float magnitude = std::sqrt(real*real + imag*imag);
        
        std::cout << i << "\t" 
                  << std::fixed << std::setprecision(2) << frequency << "\t\t"
                  << std::setprecision(6) << real << "\t" 
                  << imag << "\t"
                  << magnitude << "\n";
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
    boost::program_options::options_description programDescription("FFT Pipeline Options:");
    programDescription.add_options()
        ("bitstream,b", boost::program_options::value<string>(), "Shell bitstream")
        ("device,d", boost::program_options::value<uint32_t>(), "Target device")
        ("size,s", boost::program_options::value<uint32_t>(), "FFT size (must be multiple of 32)")
        ("reps,r", boost::program_options::value<uint32_t>(), "Number of reps")
        ("freq,f", boost::program_options::value<float>(), "Input signal frequency (Hz)")
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
    uint32_t n_reps = defReps;
    uint32_t size = defSize;

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

    // Ensure size is multiple of 32 (FFT hardware constraint)
    if(size % 32 != 0) {
        std::cout << "Warning: Size must be multiple of 32. Adjusting " << size 
                  << " to " << ((size + 31) / 32) * 32 << std::endl;
        size = ((size + 31) / 32) * 32;
    }

    uint32_t input_size = size;                    // Total complex samples
    uint32_t complex_size = 2 * input_size;       // Total floats (real + imag)
    uint32_t buffer_size = complex_size * sizeof(float);  // Total bytes
    uint32_t num_ffts = size / 32;                 // Number of 32-point FFTs

    print_header("PARAMS");
    std::cout << "Pipeline: FFT Processing" << std::endl;
    std::cout << "Total size: " << size << " points (" << num_ffts << " x 32-point FFTs)" << std::endl;
    std::cout << "Buffer size: " << buffer_size << " bytes" << std::endl;
    std::cout << "Number of reps: " << n_reps << std::endl;
    std::cout << "Sample rate: " << sampleRate << " Hz" << std::endl;
    std::cout << "Frequency resolution: " << (sampleRate/32) << " Hz/bin (per FFT)" << std::endl;

    try {
        // ---------------------------------------------------------------
        // Dataflow Setup using ushell's fluent API
        // ---------------------------------------------------------------
        print_header("DATAFLOW SETUP");
        
        // Create FFT processing dataflow
        Dataflow fft_dataflow("fft_processing_pipeline");
        
        // Create processing task
        Task& fft_processor = fft_dataflow.add_task("fft_processor", "fft");
        
        // Create buffers
        Buffer& input_buffer = fft_dataflow.add_buffer(buffer_size, "input_buffer");
        Buffer& output_buffer = fft_dataflow.add_buffer(buffer_size, "output_buffer");
        
        // Set up the FFT pipeline using fluent API
        fft_dataflow.to(input_buffer, fft_processor.in)
                    .to(fft_processor.out, output_buffer);
        
        std::cout << "Creating FFT dataflow:" << std::endl;
        std::cout << "  input_buffer → fft_processor → output_buffer" << std::endl;
        
        // Check and build the dataflow
        if (!fft_dataflow.check()) {
            throw std::runtime_error("Failed to validate dataflow");
        }
        
        // ---------------------------------------------------------------
        // Data Generation
        // ---------------------------------------------------------------
        print_header("DATA GENERATION");
        
        // Generate test data
        std::vector<float> test_data(complex_size, 0.0f);
        
        std::cout << "\nFirst 32 input values:\n";
        for (uint32_t i = 0; i < min(32U, input_size); ++i) {
            test_data[2*i] = generateSineValue(i);      // Real part
            test_data[2*i + 1] = 0.0f;                  // Imaginary part                 
            std::cout << std::fixed << std::setprecision(6) 
                     << test_data[2*i] << " " << test_data[2*i + 1] << " ";
            if ((i + 1) % 4 == 0) std::cout << "\n";
        }
        
        // Generate remaining data for larger sizes
        for (uint32_t i = 32; i < input_size; ++i) {
            test_data[2*i] = generateSineValue(i);      // Real part
            test_data[2*i + 1] = 0.0f;                  // Imaginary part
        }
        std::cout << "\n";
        
        // ---------------------------------------------------------------
        // Performance Benchmarking
        // ---------------------------------------------------------------
        print_header("FFT PROCESSING");
        
        // Create benchmark object
        cBench bench(n_reps);
        
        // Store output data for verification
        std::vector<std::vector<float>> output_data(n_reps);
        for(int i = 0; i < n_reps; i++) {
            output_data[i].resize(complex_size);
        }
        
        auto benchmark_thr = [&]() {
            for(int i = 0; i < n_reps; i++) {
                // Write input data to buffer
                write_dataflow_buffer(input_buffer, test_data.data(), buffer_size);
                
                // Clear completion counters
                fft_dataflow.clear_completed();
                
                // Execute the dataflow
                fft_dataflow.execute(buffer_size);
                
                // Read output data
                read_dataflow_buffer(output_buffer, output_data[i].data(), buffer_size);
                
                if(stalled.load()) throw std::runtime_error("Stalled, SIGINT caught");
            }
        };
        
        bench.runtime(benchmark_thr);
        
        // Print latency statistics
        print_header("LATENCY MEASUREMENTS");
        printLatencyStats(bench.getAvg() / n_reps, buffer_size, n_reps);
        
        // ---------------------------------------------------------------
        // Results Verification
        // ---------------------------------------------------------------
        print_header("RESULTS");
        int max_ffts_to_show = min(3, (int)num_ffts);  // Show max 3 FFT results
        
        for(int i = 0; i < n_reps; i++) {
            for(int fft_idx = 0; fft_idx < max_ffts_to_show; fft_idx++) {
                std::cout << "\n--- FFT #" << (fft_idx + 1) << " of " << num_ffts << " ---";
                printRawFFT(output_data[i].data() + (fft_idx * 64), 32);  // Each FFT is 64 floats
            }
            if(num_ffts > max_ffts_to_show) {
                std::cout << "\n... (remaining " << (num_ffts - max_ffts_to_show) 
                         << " FFT outputs omitted for brevity)\n";
            }
        }
        
        // Print debug information if needed
        if (fft_dataflow.get_debug_level() > 1) {
            fft_dataflow.print_graph();
            fft_dataflow.print_connections();
            fft_dataflow.print_capability_tree();
        }
        
        // ---------------------------------------------------------------
        // Resource Cleanup (Automatic with RAII)
        // ---------------------------------------------------------------
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    print_header("FFT PROCESSING COMPLETE");
    std::cout << "FFT pipeline executed successfully!" << std::endl;
    
    return EXIT_SUCCESS;
}