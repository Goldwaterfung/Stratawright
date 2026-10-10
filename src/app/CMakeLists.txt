# Main Application
cmake_minimum_required(VERSION 3.20)

# Main executable
add_executable(${PROJECT_EXECUTABLE_NAME} MACOSX_BUNDLE WIN32
    main.cpp
    application.cpp
    composition_root.cpp
    pipeline_builders/track_pipeline_builder_impl.cpp
    pipeline_builders/preview_pipeline_builder_impl.cpp
    ${CMAKE_SOURCE_DIR}/src/Presentation/resources.qrc
)

# Link all layers (dependency order matters)
target_link_libraries(${PROJECT_EXECUTABLE_NAME} PRIVATE
    # Layer 7
    layer7_views
    layer7_controllers
    # Layer 1
    layer1_audio
    layer1_midi
    # Layer 8
    agentic_server
    # Middle Bridge
    middle_bridge
    # Layer 6 Media Management
    layer5_command_history
    layer5_recording
    layer6_registry
    layer6_codecs
    layer6_analysis
    layer6_waveforms
    layer6_intake
    layer6_browser
    layer6_presets
    layer6_library
    layer6_export
    layer6_recording
    # Layer 3
    layer3_scheduler
    layer3_transport
    layer3_engine
    layer3_plugin
    layer3_automation_impl
    layer3_sidechain
    layer3_sidechain_impl
    # Common
    common_utils
    common_dsp
    dsp_audio_input
    dsp_monitor_switch
    layer4_tracks
    # External libraries
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    Threads::Threads
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Svg
)

# Platform-specific libraries
if(APPLE)
    target_link_libraries(${PROJECT_EXECUTABLE_NAME} PRIVATE
        "-framework CoreFoundation"
        "-framework CoreServices"
    )
elseif(WIN32)
    target_link_libraries(${PROJECT_EXECUTABLE_NAME} PRIVATE
        setupapi
        ole32
        shell32
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_EXECUTABLE_NAME} PRIVATE
        dl
        rt
    )
endif()

# Include directories
target_include_directories(${PROJECT_EXECUTABLE_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Set output directory and bundle properties
set_target_properties(${PROJECT_EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.stratawright.daw"
    MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_DISPLAY_NAME}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation
install(TARGETS ${PROJECT_EXECUTABLE_NAME}
    BUNDLE DESTINATION . COMPONENT Application
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Application
)

# Bundle Qt frameworks into the .app at install time so it launches on
# machines without Qt installed. macdeployqt copies Qt frameworks/plugins
# into Contents/Frameworks and rewrites @rpath links to
# @executable_path/../Frameworks (covered by CMAKE_INSTALL_RPATH).
if(APPLE)
    # Prefer the macdeployqt sibling of the Qt actually linked (via qmake),
    # never a stray copy (e.g. vcpkg tools) that may be a different version.
    set(MACDEPLOYQT_EXECUTABLE "")
    if(TARGET Qt6::qmake)
        get_target_property(_qmake_loc Qt6::qmake IMPORTED_LOCATION)
        if(_qmake_loc)
            get_filename_component(_qt_bin_dir "${_qmake_loc}" DIRECTORY)
            if(EXISTS "${_qt_bin_dir}/macdeployqt")
                set(MACDEPLOYQT_EXECUTABLE "${_qt_bin_dir}/macdeployqt")
            endif()
        endif()
    endif()
    if(NOT MACDEPLOYQT_EXECUTABLE)
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt
            HINTS "${Qt6_DIR}/../../bin" "$ENV{HOME}/Qt/6.8.0/macos/bin"
            NO_DEFAULT_PATH)
    endif()
    if(MACDEPLOYQT_EXECUTABLE)
        message(STATUS "macdeployqt: ${MACDEPLOYQT_EXECUTABLE}")
        install(CODE "
            file(GLOB _apps \"\${CMAKE_INSTALL_PREFIX}/*.app\")
            foreach(_app IN LISTS _apps)
                if(NOT EXISTS \"\${_app}/Contents/Info.plist\")
                    continue()
                endif()
                message(STATUS \"Running macdeployqt on \${_app}...\")
                execute_process(
                    COMMAND \"${MACDEPLOYQT_EXECUTABLE}\" \"\${_app}\" -codesign=-
                    RESULT_VARIABLE _mdqt_res)
                if(NOT _mdqt_res EQUAL 0)
                    message(WARNING \"macdeployqt failed for \${_app}: \${_mdqt_res}\")
                endif()
                # Re-sign adhoc: install_name_tool rewrites above invalidate
                # the link-time signature, so seal the final bundle here.
                execute_process(
                    COMMAND codesign --force --deep --sign - \"\${_app}\"
                    RESULT_VARIABLE _cs_res)
                if(NOT _cs_res EQUAL 0)
                    message(WARNING \"codesign failed for \${_app}: \${_cs_res}\")
                endif()
            endforeach()
        " COMPONENT Application)
    else()
        message(WARNING "macdeployqt not found; installed .app will require Qt on the target machine")
    endif()
endif()


