cmake_minimum_required(VERSION 3.22)

project(DigitalFilterVisualizer VERSION 0.0.1)

set(CMAKE_BUILD_TYPE Debug)

# find_package(JUCE CONFIG REQUIRED)        # If you've installed JUCE to your system
add_subdirectory(JUCE)

# juce_set_vst2_sdk_path(...)
# juce_set_aax_sdk_path(...)

juce_add_plugin(
  ${PROJECT_NAME}
  IS_SYNTH                    FALSE
  FORMATS                     VST3 Standalone
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD     TRUE
  PLUGIN_MANUFACTURER_CODE    APWG
  PLUGIN_CODE                 Dfve
  COMPANY_NAME                AudioProjectWorkgroup
  PRODUCT_NAME                DigitalFilterVisualizer
)

# juce_generate_juce_header(AudioPluginExample)

target_sources(
  ${PROJECT_NAME}
  PRIVATE
  PluginEditor.cpp
  PluginProcessor.cpp
)

target_compile_definitions(
  ${PROJECT_NAME}
  PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
)

juce_add_binary_data(
  ${PROJECT_NAME}_BinaryData
  SOURCES
    "Assets/Save.png"
    "Assets/Undo.png"
    "Assets/Redo.png"
    "Assets/Load.png"
    "Assets/Play.png"
    "Assets/Pause.png"
    "Assets/Stop.png"
    "Assets/Loop.png"
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  ${PROJECT_NAME}_BinaryData
  juce::juce_audio_utils
  juce::juce_dsp
  PUBLIC
  juce::juce_recommended_config_flags
  juce::juce_recommended_lto_flags
  juce::juce_recommended_warning_flags
)

juce_add_console_app(DigitalFilterVisualizer_Tests
    PRODUCT_NAME "DigitalFilterVisualizer_Tests"
)

target_sources(DigitalFilterVisualizer_Tests 
	PRIVATE 
	../tests/Main.cpp
    PluginProcessor.cpp
)

target_link_libraries(DigitalFilterVisualizer_Tests
    PRIVATE
        ${PROJECT_NAME}
        juce::juce_core
        juce::juce_events
        juce::juce_data_structures
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_gui_basics
        juce::juce_dsp
)