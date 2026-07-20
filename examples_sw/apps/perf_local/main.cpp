/**
  * Copyright (c) 2021, Systems Group, ETH Zurich
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *
  * 1. Redistributions of source code must retain the above copyright notice,
  * this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  * this list of conditions and the following disclaimer in the documentation
  * and/or other materials provided with the distribution.
  * 3. Neither the name of the copyright holder nor the names of its contributors
  * may be used to endorse or promote products derived from this software
  * without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#ifdef EN_AVX
#include <x86intrin.h>
#endif
#include <signal.h> 
#include <boost/program_options.hpp>
#include <any>


#include "cBench.hpp"
#include "cThread.hpp"

// #define EN_THR_TESTS
#define EN_LAT_TESTS

using namespace std;
using namespace std::chrono;
using namespace fpga;

/* Signal handler */
std::atomic<bool> stalled(false); 
void gotInt(int) {
    stalled.store(true);
}

/* Def params */
constexpr auto const defDevice = 0;

constexpr auto const nRegions = 2;
constexpr auto const defHuge = true;
constexpr auto const defMappped = true;
constexpr auto const defStream = 1;
constexpr auto const nRepsThr = 1;
constexpr auto const nRepsLat = 1;
constexpr auto const defMinSize = 1 * 1024;
constexpr auto const defMaxSize = 1 * 1024 * 1024;
constexpr auto const nBenchRuns = 1;

/**
 * @brief Validation tests
 * 
 */
int main(int argc, char *argv[])  
{
    // ---------------------------------------------------------------
    // Args 
    // ---------------------------------------------------------------

    // Sig handler
    struct sigaction sa;
    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = gotInt;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT,&sa,NULL);

    // Read arguments
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
        ("max_size,x", boost::program_options::value<uint32_t>(), "Ending transfer size");
    
    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    string bstream_path = "";
    uint32_t cs_dev = defDevice; 
    uint32_t n_regions = nRegions;
    bool huge = defHuge;
    bool mapped = defMappped;
    bool stream = defStream;
    uint32_t n_reps_thr = nRepsThr;
    uint32_t n_reps_lat = nRepsLat;
    uint32_t curr_size = defMinSize;
    uint32_t max_size = defMaxSize;

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

    PR_HEADER("PARAMS");
    std::cout << "Number of regions: " << n_regions << std::endl;
    std::cout << "Hugepages: " << huge << std::endl;
    std::cout << "Mapped pages: " << mapped << std::endl;
    std::cout << "Streaming: " << (stream ? "HOST" : "CARD") << std::endl;
    std::cout << "Number of repetitions (thr): " << n_reps_thr << std::endl;
    std::cout << "Number of repetitions (lat): " << n_reps_lat << std::endl;
    std::cout << "Starting transfer size: " << curr_size << std::endl;
    std::cout << "Ending transfer size: " << max_size << std::endl << std::endl;

    // ---------------------------------------------------------------
    // Init 
    // ---------------------------------------------------------------

    // Handles
    std::vector<std::unique_ptr<cThread<std::any>>> cthread; // Coyote threads
    void* hMem[n_regions];
    
    // Obtain resources
    for (int i = 0; i < n_regions; i++) {
        cthread.emplace_back(new cThread<std::any>(i, getpid(), cs_dev));
        hMem[i] = mapped ? (cthread[i]->getMem({huge ? CoyoteAlloc::HPF : CoyoteAlloc::REG, max_size})) 
                         : (huge ? (mmap(NULL, max_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0))
                                 : (malloc(max_size)));
    }
    // cthread.emplace_back(new cThread<std::any>(1, getpid(), cs_dev));
    // cthread.emplace_back(new cThread<std::any>(2, getpid(), cs_dev));
    // cthread.emplace_back(new cThread<std::any>(3, getpid(), cs_dev));

    sgEntry sg[n_regions];

    for(int i = 0; i < n_regions; i++) {
        // SG entries
        memset(&sg[i], 0, sizeof(localSg));
        sg[i].local.src_addr = hMem[i]; sg[i].local.src_len = curr_size; sg[i].local.src_stream = stream;
        sg[i].local.dst_addr = hMem[i]; sg[i].local.dst_len = curr_size; sg[i].local.dst_stream = stream;
    }

    // direct connection for each vFPGA
    // sg[0].local.offset_r = 0;
    // sg[0].local.offset_w = 0;
    // sg[1].local.offset_r = 0;
    // sg[1].local.offset_w = 0;
    // cthread[0]->ioSwitch(IODevs::Inter_TO_HOST_0);
    // cthread[0]->ioSwDbg();
    // cthread[1]->ioSwitch(IODevs::Inter_TO_HOST_1);
    // cthread[1]->ioSwDbg();

    // from vFPGA 0 to vFPGA 1
    // sg[0].local.offset_r = 0;
    // sg[0].local.offset_w = 6;
    // sg[1].local.offset_r = 6;
    // sg[1].local.offset_w = 0;
    // cthread[0]->ioSwitch(IODevs::Inter_TO_DTU_1);
    // cthread[0]->ioSwDbg();
    // cthread[1]->ioSwitch(IODevs::Inter_TO_HOST_1);
    // cthread[1]->ioSwDbg();

    // sg[0].local.offset_r = 0;
    // sg[0].local.offset_w = 0;
    // sg[1].local.offset_r = 0;
    // sg[1].local.offset_w = 0;
    // cthread[0]->ioSwitch(IODevs::Inter_HOST_TO_DTU_0 | IODevs::Inter_DTU_TO_HOST_0);
    // cthread[0]->ioSwDbg();
    // cthread[1]->ioSwitch(IODevs::Inter_HOST_TO_DTU_1 | IODevs::Inter_DTU_TO_HOST_1);
    // cthread[1]->ioSwDbg();

    // sg[0].local.offset_r = 0;
    // sg[0].local.offset_w = 6;
    // sg[1].local.offset_r = 6;
    // sg[1].local.offset_w = 0;
    // cthread[0]->ioSwitch(IODevs::Inter_HOST_TO_DTU_0 | IODevs::Inter_DTU_TO_DTU_1);
    // cthread[0]->ioSwDbg();
    // cthread[1]->ioSwitch(IODevs::Inter_DTU_TO_HOST_1);
    // cthread[1]->ioSwDbg();

    // cthread[0]->ioSwitch(IODevs::Inter_DTU_TO_0_USER | IODevs::Inter_USER_TO_0_DTU);
    // cthread[0]->ioSwDbg();
    // cthread[1]->ioSwitch(IODevs::Inter_DTU_TO_1_USER | IODevs::Inter_USER_TO_1_DTU);
    // cthread[1]->ioSwDbg();
    // cthread[2]->ioSwitch(IODevs::Inter_TO_3 | IODevs::WRAPPER_PIPE_MID);
    // cthread[2]->ioSwDbg();
    // cthread[3]->ioSwitch(IODevs::Inter_TO_0 | IODevs::WRAPPER_PIPE_MID);
    // cthread[3]->ioSwDbg();

    for (int j = 0; j < n_regions; j++) {
        for (int i = 0; i < max_size / 8; i++) {
            ((uint32_t *)hMem[j])[i] = 0;
        }
    }

    // for (int i = 0; i < 32 / 8; i++) {        
    //     std::cout << std::hex << std::setw(2) << std::setfill('0') << ((uint64_t *)hMem[0])[i] << " ";
    // }
    // std::cout << std::endl;

    // exit(1);
    // ---------------------------------------------------------------
    // Runs 
    // ---------------------------------------------------------------
    cBench bench(nBenchRuns);
    uint32_t n_runs;

    PR_HEADER("PERF HOST");
    while(curr_size <= max_size) {
        
#ifdef EN_THR_TESTS        
        // Prep for throughput test
        for(int i = 0; i < n_regions; i++) {
            cthread[i]->clearCompleted();
            sg[i].local.src_len = curr_size; sg[i].local.dst_len = curr_size;
        }
        n_runs = 0;
        
        // Throughput test
        auto benchmark_thr = [&]() {
            bool k = false;
            n_runs++;

            // Transfer the data
            for(int i = 0; i < n_reps_thr; i++) {
                // for(int j = 0; j < n_regions; j++) 
                //     cthread[j]->invoke(CoyoteOper::LOCAL_WRITE, &sg[j], {true, false, false});
                cthread[0]->invoke(CoyoteOper::LOCAL_READ, &sg[0], {true, false, false});
                cthread[1]->invoke(CoyoteOper::LOCAL_WRITE, &sg[1], {true, false, false});
            }
                
            int i = 0;
            while(!k) {
                k = true;
                // for(int i = 0; i < n_regions; i++) 
                //     if(cthread[i]->checkCompleted(CoyoteOper::LOCAL_WRITE) != n_reps_thr * n_runs) k = false;
                // std::cout << "n_reps_thr * n_runs: " << cthread[0]->checkCompleted(CoyoteOper::LOCAL_READ) << std::endl;
                if(cthread[0]->checkCompleted(CoyoteOper::LOCAL_READ) != n_reps_thr * n_runs) {
                    k = false;
                    std::cout << "read problem" << std::endl;
                }

                // std::cout << "n_reps_thr * n_runs: " << cthread[0]->checkCompleted(CoyoteOper::LOCAL_WRITE) << std::endl;
                if(cthread[1]->checkCompleted(CoyoteOper::LOCAL_WRITE) != n_reps_thr * n_runs) {
                    k = false;
                    std::cout << "write problem" << std::endl;
                }
                    //if(cthread[i]->checkCompleted(CoyoteOper::LOCAL_TRANSFER) != n_reps_thr * n_runs) k = false;
                if(stalled.load()) throw std::runtime_error("Stalled, SIGINT caught");
                i++;
                if (i > 10) 
                    break;
            }  
        };
        bench.runtime(benchmark_thr);
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Size: " << std::setw(8) << curr_size << ", thr: " << std::setw(8) << ((double) n_regions * 1000 * curr_size) / (bench.getAvg() / n_reps_thr) << " MB/s";
    #ifndef EN_LAT_TESTS
        std::cout << std::endl;
    #endif
#endif

#ifdef EN_LAT_TESTS
        // Prep for latency test
        for(int i = 0; i < n_regions; i++) {
            cthread[i]->clearCompleted();
            sg[i].local.src_len = curr_size; sg[i].local.dst_len = curr_size;
        }
        n_runs = 0;

        // Latency test
        auto benchmark_lat = [&]() {
            // Transfer the data
            for(int i = 0; i < n_reps_lat; i++) {
                // for(int j = 0; j < 2; j++) {
                //     // for direct connection
                //     // cthread[0]->invoke(CoyoteOper::LOCAL_TRANSFER, &sg[0], {true, true, false});
                //     // cthread[1]->invoke(CoyoteOper::LOCAL_TRANSFER, &sg[1], {true, true, false});
                //     // while(cthread[1]->checkCompleted(CoyoteOper::LOCAL_WRITE) != 1) 
                //     //     if(stalled.load()) throw std::runtime_error("Stalled, SIGINT caught");         
                    
                //     // for ul0 to ul1
                //     cthread[j]->invoke(CoyoteOper::LOCAL_TRANSFER, &sg[j], {true, true, false});
                //     while(cthread[j]->checkCompleted(CoyoteOper::LOCAL_WRITE) != 1) 
                //         if(stalled.load()) throw std::runtime_error("Stalled, SIGINT caught");           
                // }

                // for(int j = 0; j < 1; j++) {
                //     cthread[j]->invoke(CoyoteOper::LOCAL_TRANSFER, &sg[j], {true, true, false});
                //     while(cthread[j]->checkCompleted(CoyoteOper::LOCAL_WRITE) != 1) 
                //         if(stalled.load()) throw std::runtime_error("Stalled, SIGINT caught");           
                // }
                
                // for ul0 -> host -> ul1
                for(int j = 0; j < 1; j++) {
                    cthread[0]->invoke(CoyoteOper::LOCAL_TRANSFER, &sg[0], {true, true, false});
                    // while(cthread[0]->checkCompleted(CoyoteOper::LOCAL_WRITE) != 1) 
                    //     if(stalled.load()) throw std::runtime_error("Stalled, SIGINT caught");
                    
                    // check if data has been received, try to run in pipeline fashion
                    while (true) {
                        if (((uint32_t *)hMem[0])[0] == 1)
                            break;
                    }

                    // std::cout << "Number: " << ((uint32_t *)hMem[0])[0] << std::endl;
                    
                    cthread[1]->invoke(CoyoteOper::LOCAL_TRANSFER, &sg[1], {true, true, false});
                    while(cthread[1]->checkCompleted(CoyoteOper::LOCAL_WRITE) != 1) 
                        if(stalled.load()) throw std::runtime_error("Stalled, SIGINT caught");   
                }

                // for(int j = 1; j < n_regions; j++) {
                //     cthread[j]->invoke(CoyoteOper::LOCAL_TRANSFER, &sg[j], {true, true, false});
                //     while(cthread[j]->checkCompleted(CoyoteOper::LOCAL_WRITE) != 1) 
                //         if(stalled.load()) throw std::runtime_error("Stalled, SIGINT caught");           
                // }
            }
        };
        bench.runtime(benchmark_lat);
        std::cout << "size: " << curr_size;
        std::cout << ", lat: " << std::setw(8) << bench.getAvg() / (n_reps_lat) << " ns" << std::endl;
#endif
        // std::cout << "size: " << curr_size << std::endl;

        curr_size *= 2;

        for (int j = 0; j < n_regions; j++) {
            for (int i = 0; i < max_size / 8; i++) {
                ((uint32_t *)hMem[j])[i] = 0;
            }
        }
    }

    std::cout << std::endl;

    for (int j = 0; j < n_regions; j++) {
        std::cout << "Data for vFPGA " << j << std::endl;
        for (int i = 0; i < 32 / 8; i++) {
            std::cout << "Number " << i << ": " << ((uint32_t *)hMem[j])[i] << std::endl;
        }
    }
    // ---------------------------------------------------------------
    // Release 
    // ---------------------------------------------------------------
    
    // Print status
    for (int i = 0; i < n_regions; i++) {
        if(!mapped) {
            if(!huge) free(hMem[i]);
            else      munmap(hMem[i], max_size);  
        }
        cthread[i]->printDebug();
    }
    
    return EXIT_SUCCESS;
}
