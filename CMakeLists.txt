cmake_minimum_required(VERSION 3.22)

# macOS 11 (Big Sur) on Intel is the minimum; build universal (x86_64 + arm64) on CI.
# No -march flags anywhere: the code is scalar C++ and runs on any x86-64 (no AVX2 needed).
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
endif()
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")  # static CRT: no VC++ redist needed
endif()
project(RotaryPrism VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Core engine (no JUCE dependency, unit-testable) ----
add_library(rotaryprism_engine STATIC
    Source/engine/Engine.cpp
    Source/engine/Params.cpp
    Source/engine/Presets.cpp)
target_include_directories(rotaryprism_engine PUBLIC Source/engine)
set_target_properties(rotaryprism_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)

option(ROTARYPRISM_BUILD_TESTS "Build engine tests" ON)
if(ROTARYPRISM_BUILD_TESTS)
    add_executable(rotaryprism_tests tests/EngineTests.cpp)
    target_link_libraries(rotaryprism_tests PRIVATE rotaryprism_engine)
    enable_testing()
    add_test(NAME engine COMMAND rotaryprism_tests)
endif()

option(ROTARYPRISM_BUILD_PLUGIN "Build the JUCE plugin" ON)
if(NOT ROTARYPRISM_BUILD_PLUGIN)
    return()
endif()

# ---- JUCE ----
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(RP_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND RP_FORMATS AU)
endif()

juce_add_plugin(RotaryPrism
    COMPANY_NAME "Rotary Prism Audio"
    PLUGIN_MANUFACTURER_CODE Rprz
    PLUGIN_CODE Rpz1
    FORMATS ${RP_FORMATS}
    PRODUCT_NAME "Rotary Prism"
    BUNDLE_ID com.spenceradam98.rotaryprism
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicEffect
    VST3_CATEGORIES Fx Modulation Spatial)

target_sources(RotaryPrism PRIVATE
    Source/plugin/PluginProcessor.cpp
    Source/plugin/PluginEditor.cpp
    Source/plugin/Themes.cpp)

target_compile_definitions(RotaryPrism PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(RotaryPrism
    PRIVATE
        rotaryprism_engine
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---- optional: UI screenshots for the docs ----
option(ROTARYPRISM_BUILD_SNAPSHOTS "Build the UI screenshot tool" OFF)
if(ROTARYPRISM_BUILD_SNAPSHOTS)
    juce_add_console_app(rotaryprism_snapshots PRODUCT_NAME "Rotary Prism Snapshots")
    target_sources(rotaryprism_snapshots PRIVATE tools/Snapshots.cpp Source/plugin/PluginProcessor.cpp
        Source/plugin/PluginEditor.cpp Source/plugin/Themes.cpp)
    target_compile_definitions(rotaryprism_snapshots PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Rotary Prism"
        JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(rotaryprism_snapshots PRIVATE rotaryprism_engine juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
