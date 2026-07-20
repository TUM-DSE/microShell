cmake_minimum_required(VERSION 3.5)
project(test)

set(CYT_DIR ${CMAKE_SOURCE_DIR}/../)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CYT_DIR}/cmake)

find_package(CoyoteSW REQUIRED)

#
# Pass an additional command-line argument to specify the verbosity of the compilation 
# If VERBOSITY is passed as an argument, VERBOSE is set as preprocessor-macro
# 

set(VERBOSITY OFF CACHE BOOL "Enable VERBOSITY for compilation")

if (VERBOSITY)
    message("You selected VERBOSITY for compilation")
    add_compile_definitions(VERBOSE)
else()
    message("VERBOSITY is not selected for compilation")
endif()


#
# Shell reconfiguration
#
if(EXAMPLE STREQUAL "reconfigure_shell")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/reconfigure_shell")
    message("** Shell load")
endif()

#
# TEST
#
if(EXAMPLE STREQUAL "perf_gpu")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/perf_gpu")
    message("** Example: Validation tests - GPU perf")
endif()

#
# Example builds
#

# Used in conjunction with the default shell
if(EXAMPLE STREQUAL "perf_local")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/perf_local")
    message("** Example: Validation tests - Link perf")
endif()

# Used in conjunction with the default shell
if(EXAMPLE STREQUAL "perf_mem")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/perf_mem")
    message("** Example: Test using fpga memory")
endif()

# Used in conjunction with the default shell
if(EXAMPLE STREQUAL "perf_counter")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/perf_counter")
    message("** Example: Test perf couter")
endif()

# Used in conjunction with the microshell
if(EXAMPLE STREQUAL "pipeline")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/pipeline")
    message("** Example: Validation tests - Link perf")
endif()

# FPGA perf benchmark
if(EXAMPLE STREQUAL "perf_fpga")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/perf_fpga")
    message("** Example: FPGA perf benchmark")
endif()

# scheduler app
if(EXAMPLE STREQUAL "scheduler_client")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/scheduler/client")
    message("** HLL Streaming test client")
    include_directories(${TARGET_DIR}/../include ${TARGET_DIR}/include)
endif()

# Streaming services example
if(EXAMPLE STREQUAL "streaming_server")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/streaming_service/server")
    message("** Example: Streaming service - Server (uShell daemon)")
    include_directories(${TARGET_DIR}/../include ${TARGET_DIR}/include)
endif()

if(EXAMPLE STREQUAL "streaming_client")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/streaming_service/client")
    message("** Example: Streaming service - Client")
    include_directories(${TARGET_DIR}/../include)
endif()

# Perf services example
if(EXAMPLE STREQUAL "perf_server")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/perf_service/server")
    message("** Example: perf_local service - Server (uShell daemon)")
    include_directories(${TARGET_DIR}/../include ${TARGET_DIR}/include)
endif()

if(EXAMPLE STREQUAL "perf_client")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/perf_service/client")
    message("** Example: perf_local service - Client")
    include_directories(${TARGET_DIR}/../include)
endif()



if(EXAMPLE STREQUAL "multithreading")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/multithreading")
    message("** Example: Multithreading")
endif()

# Kmeans example (unified memory)
if(EXAMPLE STREQUAL "kmeans")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/kmeans")
    message("** Example: K-Means kernel - Unified memory")
endif()

# Network - RDMA
if(EXAMPLE STREQUAL "rdma_server")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/rdma_service/server")
    message("** Example: RDMA service - Server (Coyote daemon)")
    include_directories(${TARGET_DIR}/../include)
endif()

if(EXAMPLE STREQUAL "rdma_client")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/rdma_service/client")
    message("** Example: RDMA service - Client")
    include_directories(${TARGET_DIR}/../include)
endif()

# Network - TCP
if(EXAMPLE STREQUAL "tcp_iperf")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/tcp_iperf")
    message("**  Example: TCP - iperf")
endif()

# Application examples

# FFT
if(EXAMPLE STREQUAL "fft")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/fft")
    message("** Example: FFT (µShell daemon)")
endif()

# AES-CTR
if(EXAMPLE STREQUAL "aes_ctr")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/aes_ctr")
    message("** Example: AES-CTR (µShell daemon)")
endif()

# RLE
if(EXAMPLE STREQUAL "rle")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/rle")
    message("** Example: RLE (µShell daemon)")
endif()

# RSA
if(EXAMPLE STREQUAL "rsa")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/rsa")
    message("** Example: RSA (µShell daemon)")
endif()

# SHA2
if(EXAMPLE STREQUAL "sha2")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/sha2")
    message("** Example: SHA2 (µShell daemon)")
endif()

# SVM
if(EXAMPLE STREQUAL "svm")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/svm")
    message("** Example: SVM (µShell daemon)")
endif()

# QUANT
if(EXAMPLE STREQUAL "quant")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/quant")
    message("** Example: FFT Quantization (µShell daemon)")
endif()

# Audio Processing
if(EXAMPLE STREQUAL "audio")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/audio")
    message("** Example: Audio Processing (µShell daemon)")
endif()

#Audio_Mono
if(EXAMPLE STREQUAL "audio_mono")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/audio_mono")
    message("** Example: Audio Processing Monolithic (µShell daemon)")
endif()

# Digital Signature App
if(EXAMPLE STREQUAL "digi_sign")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/digi_sign")
    message("** Example: Digital Signature (µShell daemon)")
endif()

#DigiSign_Mono
if(EXAMPLE STREQUAL "digi_sign_mono")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/digi_sign_mono")
    message("** Example: Digital Signature Monolithic (µShell daemon)")
endif()

# Secure Storage App
if(EXAMPLE STREQUAL "secure")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/secure")
    message("** Example: Secure Storage (µShell daemon)")
endif()

#Secure_Mono
if(EXAMPLE STREQUAL "secure_mono")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/secure_mono")
    message("** Example: Secure Storage Monolithic (µShell daemon)")
endif()

# Signed Compression App
if(EXAMPLE STREQUAL "signcomp")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/signcomp")
    message("** Example: Signed Compression (µShell daemon)")
endif()

#SignComp_Mono
if(EXAMPLE STREQUAL "signcomp_mono")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/signcomp_mono")
    message("** Example: Signed Compression Monolithic (µShell daemon)")
endif()

# Speech Recognition App
if(EXAMPLE STREQUAL "speech")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/speech")
    message("** Example: Speech Recognition (µShell daemon)")
endif()

#Speech_Mono
if(EXAMPLE STREQUAL "speech_mono")
    set(TARGET_DIR "${CYT_DIR}/examples_sw/apps/speech_mono")
    message("** Example: Speech Recognition Monolithic (µShell daemon)")
endif()

#
# Create build targets
#
set(EXEC test)
add_executable(${EXEC} ${TARGET_DIR}/main.cpp)
target_link_libraries(${EXEC} PUBLIC Coyote)
target_link_directories(${EXEC} PUBLIC /usr/local/lib)

if(EN_GPU)
    set(AMD_GPU "gfx908" CACHE STRING "AMD GPU device to use")
    set_property(TARGET ${EXEC} PROPERTY HIP_ARCHITECTURES ${AMD_GPU})
    set_target_properties(${EXEC} PROPERTIES LINKER_LANGUAGE CXX)
    target_link_libraries(${EXEC} PUBLIC hip::device  numa pthread drm drm_amdgpu rt dl hsa-runtime64 hsakmt)
endif()
