cmake_minimum_required(VERSION 3.22)

project(UltimateTranceGate VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# On Mac build a universal binary (Apple Silicon + Intel) that runs on macOS 11+
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    endif()
endif()

# ---- JUCE ------------------------------------------------------------------
# Use a local JUCE checkout if JUCE_DIR is given, otherwise download it.
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.9
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---- Plugin ----------------------------------------------------------------
if(APPLE)
    set(TG_FORMATS VST3 AU)
    set(TG_COPY TRUE)      # installs into ~/Library/Audio/Plug-Ins after building
else()
    set(TG_FORMATS VST3)
    set(TG_COPY FALSE)
endif()

juce_add_plugin(UltimateTranceGate
    COMPANY_NAME                "mikita_laurenau"
    PLUGIN_MANUFACTURER_CODE    Mklr
    PLUGIN_CODE                 UtG1
    PRODUCT_NAME                "Ultimate Trance Gate"
    FORMATS                     ${TG_FORMATS}
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${TG_COPY}
    VST3_CATEGORIES             Fx Dynamics
    AU_MAIN_TYPE                kAudioUnitType_MusicEffect
    BUNDLE_ID                   com.mikitalaurenau.ultimatetrancegate)

set(TG_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/StepGrid.cpp)

target_sources(UltimateTranceGate PRIVATE ${TG_SOURCES})

# Figtree font (SIL Open Font License) embedded as binary data
juce_add_binary_data(UTGBinaryData SOURCES
    Resources/fonts/Figtree-Regular.ttf
    Resources/fonts/Figtree-SemiBold.ttf
    Resources/fonts/Figtree-Bold.ttf
    Resources/fonts/Figtree-ExtraBold.ttf)
set_target_properties(UTGBinaryData PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(TG_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(UNIX AND NOT APPLE)
    # keep the Linux build light (only used for testing)
    list(APPEND TG_DEFINITIONS JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_compile_definitions(UltimateTranceGate PUBLIC ${TG_DEFINITIONS})
if(MSVC)
    target_compile_options(UltimateTranceGate PRIVATE /utf-8)
endif()

target_link_libraries(UltimateTranceGate
    PRIVATE
        UTGBinaryData
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---- Offline render test (optional) ----------------------------------------
option(TG_BUILD_TESTS "Build offline render test" OFF)
if(TG_BUILD_TESTS)
    juce_add_console_app(UTGRenderTest PRODUCT_NAME "UTGRenderTest")
    target_sources(UTGRenderTest PRIVATE tests/RenderTest.cpp ${TG_SOURCES})
    target_include_directories(UTGRenderTest PRIVATE Source)
    target_compile_definitions(UTGRenderTest PRIVATE ${TG_DEFINITIONS} JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(UTGRenderTest PRIVATE
        UTGBinaryData
        juce::juce_audio_processors juce::juce_audio_formats juce::juce_gui_basics juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
