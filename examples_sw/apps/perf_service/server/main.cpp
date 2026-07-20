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

#include <dirent.h>
#include <iterator>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include <vector>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iomanip>
#include <chrono>
#include <thread>
#include <limits>
#include <assert.h>
#include <stdio.h>
#include <sys/un.h>
#include <errno.h>
#include <wait.h>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <boost/program_options.hpp>

#include "cService.hpp"
#include "cFunc.hpp"
#include "gbm_dtrees.hpp"
#include "types.hpp"

using namespace std;
using namespace fpga;

/**
 * @brief Main
 *  
 */
int main(int argc, char *argv[]) 
{   
    /* Args */
    boost::program_options::options_description programDescription("Options:");
    programDescription.add_options()
        ("device,d", boost::program_options::value<uint32_t>(), "Target device")
        ("vfid,i", boost::program_options::value<uint32_t>(), "Target vFPGA");

    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    uint32_t cs_dev = defDevice; 
    uint32_t vfid = 0;
    // uint32_t vfid = 1;

    if(commandLineArgs.count("device") > 0) cs_dev = commandLineArgs["device"].as<uint32_t>();
    if(commandLineArgs.count("vfid") > 0) vfid = commandLineArgs["vfid"].as<uint32_t>();

    /**
     * @brief Load all service functions and start the server
    */
    cService *cservice = cService::getInstance("streaming", false, vfid, cs_dev);
    std::cout << std::endl << "Shell loading ..." << std::endl << std::endl;
    cservice->shellReconfigure("shell_top.bin");


    /**
     * @brief Load all operators (partial images) into the scheduler and start the operation
     * 
     */
    // load_apps (
    //     VFPGA_C0_0 "apps/hyperloglog"    
    //     VFPGA_C1_0 "apps/gbm_dtrees"    
    // )
    cservice->addBitstream("vfpga_c0_0.bin", operator_perf_1);
    cservice->addBitstream("vfpga_c1_0.bin", operator_perf_2);
    cservice->addBitstream("vfpga_c2_0.bin", operator_perf_3);
    cservice->addBitstream("vfpga_c3_0.bin", operator_perf_4);

    // cservice->addBitstream("vfpga_c1_0.bin", operator_perf_1);
    // cservice->addBitstream("vfpga_c1_1.bin", operator_perf_2);

    // The Hyper-Log-Log task
    cservice->addFunction(fid_perf_1, std::unique_ptr<bFunc>(new cFunc<double, uint64_t, uint32_t>(operator_perf_1,
        [=] (cThread<double> *cthread, uint64_t h_mem, uint32_t size) -> double { // returns time
            void* hMem = (void*) h_mem;

            syslog(LOG_NOTICE, "Executing perf_local task, params: dMem %lx, tuples %d", (uint64_t)hMem, size);
            
            // SG entries --------------------------------------------------------------------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            sgEntry sg;
            memset(&sg, 0, sizeof(localSg));
            sg.local.src_addr = hMem; sg.local.src_len = size;
            sg.local.dst_addr = hMem; sg.local.dst_len = size;

            // User map ----------------------------------------------------------------------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            cthread->userMap(hMem, size);
            
            // Lock vFPGA (scheduler will load the required bitstream if necessary) ----------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            cthread->pLock(operator_perf_1, opPriority); 

            // Invoke (move the data) --------------------------------------------------------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            auto begin_time = chrono::high_resolution_clock::now();
            // cthread->invoke(CoyoteOper::LOCAL_TRANSFER, &sg, {true, true, true});
            auto end_time = chrono::high_resolution_clock::now();

            // Unlock vFPGA ------------------------------------------------------------------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            cthread->pUnlock();

            // User unmap
            cthread->userUnmap(hMem);
            
            double time = chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time).count();
            syslog(LOG_NOTICE, "Task HLL executed, time %f", time);
            
            return { time };
        }
    )));
    
    // Load Decision trees task
    cservice->addFunction(fid_perf_2, std::unique_ptr<bFunc>(new cFunc<double, uint64_t, uint32_t>(operator_perf_2,
        [=] (cThread<double> *cthread, uint64_t h_mem, uint32_t size) -> double { // returns time
            void* hMem = (void*) h_mem;

            syslog(LOG_NOTICE, "Executing perf_local task, params: dMem %lx, tuples %d", (uint64_t)hMem, size);
            
            // SG entries --------------------------------------------------------------------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            sgEntry sg;
            memset(&sg, 0, sizeof(localSg));
            sg.local.src_addr = hMem; sg.local.src_len = size;
            sg.local.dst_addr = hMem; sg.local.dst_len = size;

            // User map ----------------------------------------------------------------------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            cthread->userMap(hMem, size);
            
            // Lock vFPGA (scheduler will load the required bitstream if necessary) ----------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            cthread->pLock(operator_perf_1, opPriority); 

            // Invoke (move the data) --------------------------------------------------------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            auto begin_time = chrono::high_resolution_clock::now();
            // cthread->invoke(CoyoteOper::LOCAL_TRANSFER, &sg, {true, true, true});
            auto end_time = chrono::high_resolution_clock::now();

            // Unlock vFPGA ------------------------------------------------------------------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            cthread->pUnlock();

            // User unmap
            cthread->userUnmap(hMem);
            
            double time = chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time).count();
            syslog(LOG_NOTICE, "Task HLL executed, time %f", time);
            
            return { time };
        }
    )));

        // Load Decision trees task
    cservice->addFunction(fid_perf_3, std::unique_ptr<bFunc>(new cFunc<double, uint64_t, uint32_t>(operator_perf_3,
        [=] (cThread<double> *cthread, uint64_t h_mem, uint32_t size) -> double { // returns time
            void* hMem = (void*) h_mem;

            syslog(LOG_NOTICE, "Executing perf_local task, params: dMem %lx, tuples %d", (uint64_t)hMem, size);
            
            // SG entries --------------------------------------------------------------------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            sgEntry sg;
            memset(&sg, 0, sizeof(localSg));
            sg.local.src_addr = hMem; sg.local.src_len = size;
            sg.local.dst_addr = hMem; sg.local.dst_len = size;

            // User map ----------------------------------------------------------------------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            cthread->userMap(hMem, size);
            
            // Lock vFPGA (scheduler will load the required bitstream if necessary) ----------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            cthread->pLock(operator_perf_1, opPriority); 

            // Invoke (move the data) --------------------------------------------------------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            auto begin_time = chrono::high_resolution_clock::now();
            // cthread->invoke(CoyoteOper::LOCAL_TRANSFER, &sg, {true, true, true});
            auto end_time = chrono::high_resolution_clock::now();

            // Unlock vFPGA ------------------------------------------------------------------------------------------------
            // -------------------------------------------------------------------------------------------------------------
            cthread->pUnlock();

            // User unmap
            cthread->userUnmap(hMem);
            
            double time = chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time).count();
            syslog(LOG_NOTICE, "Task HLL executed, time %f", time);
            
            return { time };
        }
    )));

    // // Load Decision trees task
    // cservice->addFunction(fid_perf_4, std::unique_ptr<bFunc>(new cFunc<double, uint64_t, uint32_t>(operator_perf_4,
    //     [=] (cThread<double> *cthread, uint64_t h_mem, uint32_t size) -> double { // returns time
    //         void* hMem = (void*) h_mem;

    //         syslog(LOG_NOTICE, "Executing perf_local task, params: dMem %lx, tuples %d", (uint64_t)hMem, size);
            
    //         // SG entries --------------------------------------------------------------------------------------------------
    //         // -------------------------------------------------------------------------------------------------------------
    //         sgEntry sg;
    //         memset(&sg, 0, sizeof(localSg));
    //         sg.local.src_addr = hMem; sg.local.src_len = size;
    //         sg.local.dst_addr = hMem; sg.local.dst_len = size;

    //         // User map ----------------------------------------------------------------------------------------------------
    //         // -------------------------------------------------------------------------------------------------------------
    //         cthread->userMap(hMem, size);
            
    //         // Lock vFPGA (scheduler will load the required bitstream if necessary) ----------------------------------------
    //         // -------------------------------------------------------------------------------------------------------------
    //         cthread->pLock(operator_perf_1, opPriority); 

    //         // Invoke (move the data) --------------------------------------------------------------------------------------
    //         // -------------------------------------------------------------------------------------------------------------
    //         auto begin_time = chrono::high_resolution_clock::now();
    //         // cthread->invoke(CoyoteOper::LOCAL_TRANSFER, &sg, {true, true, true});
    //         auto end_time = chrono::high_resolution_clock::now();

    //         // Unlock vFPGA ------------------------------------------------------------------------------------------------
    //         // -------------------------------------------------------------------------------------------------------------
    //         cthread->pUnlock();

    //         // User unmap
    //         cthread->userUnmap(hMem);
            
    //         double time = chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time).count();
    //         syslog(LOG_NOTICE, "Task HLL executed, time %f", time);
            
    //         return { time };
    //     }
    // )));
    //
    // Start a daemon
    //
    std::cout << "Forking ..." << std::endl << std::endl;
    cservice->start();
}

