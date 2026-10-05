# =============================================================================
# RubberBand Library - CMake Configuration
# Layer: third_party (no DAW layer dependency)
# =============================================================================

cmake_minimum_required(VERSION 3.20)
project(RubberBand LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11) # RubberBand requires C++11
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------
# Core static library using the "Single-File" build aggregator
# -----------------------------------------------------------------------------
add_library(rubberband_lib STATIC single/RubberBandSingle.cpp)

target_include_directories(rubberband_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/rubberband>
)

# -----------------------------------------------------------------------------
# Build Configuration
# -----------------------------------------------------------------------------
if(APPLE)
    target_link_libraries(rubberband_lib PUBLIC "-framework Accelerate")
    target_compile_definitions(rubberband_lib PRIVATE HAVE_VDSP=1)
else()
    target_compile_definitions(rubberband_lib PRIVATE USE_BUILTIN_FFT=1)
endif()

target_compile_definitions(rubberband_lib 
    PRIVATE 
        USE_BQRESAMPLER=1
        NO_TIMING=1
        NO_THREADING=1
        NO_THREAD_CHECKS=1
)

# -----------------------------------------------------------------------------
# Warning Suppression (Legacy code)
# -----------------------------------------------------------------------------
if(NOT MSVC)
    target_compile_options(rubberband_lib PRIVATE
        -Wno-unused-parameter
        -Wno-sign-compare
        -Wno-old-style-cast
        -Wno-double-promotion
        -Wno-unused-variable
        -Wno-reorder
        -Wno-implicit-fallthrough
        -Wno-sign-conversion
        -Wno-cast-align
    )
endif()
