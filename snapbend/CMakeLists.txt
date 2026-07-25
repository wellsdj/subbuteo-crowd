cmake_minimum_required(VERSION 3.22)

project(SnapBend VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Logic requires an Audio Unit, and Audio Units are macOS only. Everything under
# core/ is plain C++ though, so the engine and its tests build anywhere — which
# is how the bend maths gets tested without a Mac in the loop.
option(SNAPBEND_BUILD_PLUGIN "Build the Audio Unit (requires macOS + Xcode)" ${APPLE})
option(SNAPBEND_BUILD_TESTS  "Build the core unit tests" ON)

# ---------------------------------------------------------------------------
# Core engine — no JUCE, no dependencies
# ---------------------------------------------------------------------------

add_library(snapbend_core STATIC
    core/BendCurve.cpp
    core/BendMath.cpp
    core/BendEmitter.cpp)

target_include_directories(snapbend_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(snapbend_core PRIVATE /W4)
else()
    target_compile_options(snapbend_core PRIVATE -Wall -Wextra)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

if(SNAPBEND_BUILD_TESTS)
    enable_testing()

    add_executable(snapbend_tests tests/CoreTests.cpp)
    target_link_libraries(snapbend_tests PRIVATE snapbend_core)

    add_test(NAME snapbend_core_tests COMMAND snapbend_tests)
endif()

# ---------------------------------------------------------------------------
# The plugin
# ---------------------------------------------------------------------------

if(SNAPBEND_BUILD_PLUGIN)
    include(FetchContent)

    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)

    FetchContent_MakeAvailable(JUCE)

    # Logic needs the AU, which only exists on macOS. Elsewhere we still build a
    # standalone binary — not because anyone will use it, but because it
    # compiles every line of the plugin, so the UI can be checked on Linux CI
    # without a Mac.
    if(APPLE)
        set(SNAPBEND_FORMATS AU Standalone)
    else()
        set(SNAPBEND_FORMATS Standalone)
    endif()

    juce_add_plugin(SnapBend
        PRODUCT_NAME            "SnapBend"
        COMPANY_NAME            "SnapBend"
        BUNDLE_ID               "com.snapbend.snapbend"
        PLUGIN_MANUFACTURER_CODE Snpb
        PLUGIN_CODE             Snb1

        # This combination is what puts the plugin in Logic's MIDI FX slot,
        # above the instrument, rather than in the audio insert chain.
        FORMATS                 ${SNAPBEND_FORMATS}
        IS_MIDI_EFFECT          TRUE
        NEEDS_MIDI_INPUT        TRUE
        NEEDS_MIDI_OUTPUT       TRUE
        AU_MAIN_TYPE            kAudioUnitType_MIDIProcessor

        COPY_PLUGIN_AFTER_BUILD TRUE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE)

    target_sources(SnapBend PRIVATE
        plugin/PluginProcessor.cpp
        plugin/PluginEditor.cpp
        plugin/ui/BendLaneComponent.cpp
        plugin/ui/RangeWarningPanel.cpp
        plugin/ui/SnapBendLookAndFeel.cpp)

    target_compile_definitions(SnapBend PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(SnapBend PRIVATE
        snapbend_core
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
endif()
