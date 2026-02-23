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
  CoeffLabels.cpp
)

target_compile_definitions(
  ${PROJECT_NAME}
  PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
)

# juce_add_binary_data(AudioPluginData SOURCES ...)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  juce::juce_audio_utils
  juce::juce_dsp
  juce::juce_gui_basics
  PUBLIC
  juce::juce_recommended_config_flags
  juce::juce_recommended_lto_flags
  juce::juce_recommended_warning_flags
)
