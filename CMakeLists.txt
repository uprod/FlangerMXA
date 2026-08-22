cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Binaires universels Mac")
project(FlangerMXA VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# JUCE : submodule du depot si initialise, sinon dossier voisin partage (suite MXA).
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/JUCE JUCE)
else()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../JUCE JUCE)
endif()

juce_add_plugin(FlangerMXA
    COMPANY_NAME "MXAudio"
    BUNDLE_ID com.n2xlabs.flangermxa
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE N2xL
    PLUGIN_CODE Flmx
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "FlangerMXA"
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(FlangerMXA PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/FlangerEngine.cpp
    Source/ManualStyle.cpp
    Source/ResponsePlot.cpp
    Source/SchematicDiagram.cpp)

# Polices embarquees du monde "Service Manual" (voir Assets/LICENSES.md).
juce_add_binary_data(FlangerMXAAssets SOURCES
    Assets/RoutedGothic-Regular.ttf
    Assets/RoutedGothicWide-Regular.ttf
    Assets/CourierPrime-Regular.ttf
    Assets/CourierPrime-Bold.ttf)

# Outil de capture hors-ecran (revue de design uniquement, jamais livre).
juce_add_console_app(FlangerMXASnapshot PRODUCT_NAME "FlangerMXASnapshot")
target_sources(FlangerMXASnapshot PRIVATE
    Tools/SnapshotMain.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/FlangerEngine.cpp
    Source/ManualStyle.cpp
    Source/ResponsePlot.cpp
    Source/SchematicDiagram.cpp)
target_compile_definitions(FlangerMXASnapshot PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(FlangerMXASnapshot PRIVATE
    FlangerMXAAssets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics)

target_compile_definitions(FlangerMXA PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(FlangerMXA PRIVATE
    FlangerMXAAssets
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
