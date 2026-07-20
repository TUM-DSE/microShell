#ifndef SCHEDULER_HLL_TYPES
#define SCHEDULER_HLL_TYPES

// Define NEW_SCHEDULER to use the proposed scheduler in cSched.
#define NEW_SCHEDULER

#include <iostream>
#include <string>
#include <malloc.h>
#include <chrono>
#include <x86intrin.h>
#include <signal.h>
#include <boost/program_options.hpp>

#include "cService.hpp"
#include "cThread.hpp"
#include "cDefs.hpp"
#include "cFunc.hpp"
#include "cLib.hpp"

using namespace fpga;
namespace po = boost::program_options;

// Device IDs
constexpr uint32_t const def_dev  = 0;  // Default Device (which FPGA card)
constexpr uint32_t const def_vfid = 0;  // Default vFPGA index

// Bitstreams
constexpr int32_t const total_apps = 3;    // Total number of bitstreams available
// Specify the bitstream directory of the targeted build
const std::string bstream_dir = "path/to/build/bitstreams/";
const std::string service_name = "streaming";  // Service name (used for socket)

constexpr auto const opPriority = 1;

constexpr auto const defDW = 4;

#endif  // SCHEDULER_HLL_TYPES