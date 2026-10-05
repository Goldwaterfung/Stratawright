# Layer4: timepitch
cmake_minimum_required(VERSION 3.20)

add_library(layer4_timepitch STATIC
    time_pitch_node.h
    time_pitch_node.cpp
    sound_touch_node.h
    sound_touch_node.cpp
)

target_include_directories(layer4_timepitch PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    # SoundTouch headers (SoundTouch.h); do not rely solely on transitive
    # propagation from soundtouch_lib (fails in fresh CI builds).
    ${CMAKE_SOURCE_DIR}/src/third_party/soundtouch/include
    ${CMAKE_SOURCE_DIR}/src/third_party/soundtouch/source/SoundTouch
)
target_link_libraries(layer4_timepitch PUBLIC 
    common_utils 
    soundtouch_lib
    rubberband_lib
)
