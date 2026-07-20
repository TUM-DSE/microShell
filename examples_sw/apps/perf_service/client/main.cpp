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
#include <x86intrin.h>
#include <boost/program_options.hpp>
#include <sys/socket.h>
#include <sys/un.h>
#include <sstream>
#include <sys/mman.h>

#include "cLib.hpp"
#include "types.hpp"

using namespace std;
using namespace fpga;

// Runtime 
constexpr auto const defRunHLL = false;
constexpr auto const defRunDtrees = false;
constexpr auto const defMinSize = 1 * 1024;


int main(int argc, char *argv[]) 
{

    // Read arguments
    boost::program_options::options_description programDescription("Options:");
    programDescription.add_options()
        ("tuples,t", boost::program_options::value<bool>(), "Number of tuples")
        ("features,f", boost::program_options::value<bool>(), "Number of features")
        ("one,1", boost::program_options::value<bool>(), "Run HyperLogLog")
        ("two,2", boost::program_options::value<bool>(), "Run Decision Trees")
        ("three,3", boost::program_options::value<bool>(), "Run Decision Trees")
        ("four,4", boost::program_options::value<bool>(), "Run Decision Trees");

    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    bool runOne = 0;
    bool runTwo = 0;
    bool runThree = 0;
    bool runFour = 0;
    uint32_t size = defMinSize;

    if(commandLineArgs.count("one") > 0) runOne = commandLineArgs["one"].as<bool>();
    if(commandLineArgs.count("two") > 0) runTwo = commandLineArgs["two"].as<bool>();
    if(commandLineArgs.count("three") > 0) runThree = commandLineArgs["three"].as<bool>();
    if(commandLineArgs.count("four") > 0) runFour = commandLineArgs["four"].as<bool>();

    // send the request 5 times
    for (int i = 0; i < 5; i++) {
        //
        // HyperLogLog operator
        //
        if(runOne) {
            // Connect to service
            cLib<double, uint64_t, uint32_t> clib_perf_1("/tmp/coyote-daemon-vfid-0-streaming", fid_perf_1); 
            // cLib<double, uint64_t, uint32_t> clib_perf_1("/tmp/coyote-daemon-vfid-1-streaming", fid_perf_1); 

            // Let's get some buffers and fill it with some random data ...
            uint32_t* dMem = (uint32_t*) memalign(axiDataWidth, size);

            for (int i = 0; i < size / 8; i++) {
                dMem[i] = 0;
            }
            
            // Execute the HLL
            // This is the only place of interaction with Coyote ...
            double cmpl_ev = clib_perf_1.task(opPriority, (uint64_t)dMem, size);

            PR_HEADER("perf_local_1");
            std::cout << std::fixed << std::setprecision(2) << std::dec;
            std::cout << "Estimation completed, run time: " << cmpl_ev << " us" << std::endl;
            
            free(dMem);
        }
        
        //
        // Decision trees operator
        //
        if(runTwo) {
            // Connect to service
            cLib<double, uint64_t, uint32_t> clib_perf_2("/tmp/coyote-daemon-vfid-0-streaming", fid_perf_2);
            // cLib<double, uint64_t, uint32_t> clib_perf_2("/tmp/coyote-daemon-vfid-1-streaming", fid_perf_2);

            // Let's get some buffers and fill it with some random data ...
            uint32_t* dMem = (uint32_t*) memalign(axiDataWidth, size);

            for (int i = 0; i < size / 8; i++) {
                dMem[i] = 0;
            }
            
            // Execute the HLL
            // This is the only place of interaction with Coyote ...
            double cmpl_ev = clib_perf_2.task(opPriority, (uint64_t)dMem, size);

            PR_HEADER("perf_local_2");
            std::cout << std::fixed << std::setprecision(2) << std::dec;
            std::cout << "Estimation completed, run time: " << cmpl_ev << " us" << std::endl;
            
            free(dMem);
        }

        if(runThree) {
            // Connect to service
            cLib<double, uint64_t, uint32_t> clib_perf_3("/tmp/coyote-daemon-vfid-0-streaming", fid_perf_3);
            // cLib<double, uint64_t, uint32_t> clib_perf_2("/tmp/coyote-daemon-vfid-1-streaming", fid_perf_2);

            // Let's get some buffers and fill it with some random data ...
            uint32_t* dMem = (uint32_t*) memalign(axiDataWidth, size);

            for (int i = 0; i < size / 8; i++) {
                dMem[i] = 0;
            }
            
            // Execute the HLL
            // This is the only place of interaction with Coyote ...
            double cmpl_ev = clib_perf_3.task(opPriority, (uint64_t)dMem, size);

            PR_HEADER("perf_local_3");
            std::cout << std::fixed << std::setprecision(2) << std::dec;
            std::cout << "Estimation completed, run time: " << cmpl_ev << " us" << std::endl;
            
            free(dMem);
        }
        

        if(runFour) {
            // Connect to service
            cLib<double, uint64_t, uint32_t> clib_perf_4("/tmp/coyote-daemon-vfid-0-streaming", fid_perf_4);
            // cLib<double, uint64_t, uint32_t> clib_perf_2("/tmp/coyote-daemon-vfid-1-streaming", fid_perf_2);

            // Let's get some buffers and fill it with some random data ...
            uint32_t* dMem = (uint32_t*) memalign(axiDataWidth, size);

            for (int i = 0; i < size / 8; i++) {
                dMem[i] = 0;
            }
            
            // Execute the HLL
            // This is the only place of interaction with Coyote ...
            double cmpl_ev = clib_perf_4.task(opPriority, (uint64_t)dMem, size);

            PR_HEADER("perf_local_4");
            std::cout << std::fixed << std::setprecision(2) << std::dec;
            std::cout << "Estimation completed, run time: " << cmpl_ev << " us" << std::endl;
            
            free(dMem);
        }
    }

    return (EXIT_SUCCESS);
}
