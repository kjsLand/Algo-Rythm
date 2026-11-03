# The VST3, AU and Standalone plugin (SPEC-16). It uses the core library and the UI library of the app (REQ-16-1).
# Build it with -DALGO_BUILD_PLUGIN=ON.

if (APPLE)
    set(algo_plugin_formats VST3 AU Standalone)
else()
    set(algo_plugin_formats VST3 Standalone)
endif()

juce_add_plugin(AlgoRythmPlugin
    PRODUCT_NAME "Algo-Rythm"
    VERSION "${PROJECT_VERSION}"
    COMPANY_NAME "kjsLand"
    BUNDLE_ID "com.kjsland.algorythm.plugin"
    PLUGIN_MANUFACTURER_CODE Kjsl
    PLUGIN_CODE Algr
    FORMATS ${algo_plugin_formats}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Generator
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
)

target_sources(AlgoRythmPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(AlgoRythmPlugin PRIVATE Source)

target_link_libraries(AlgoRythmPlugin
    PRIVATE
        algo_ui
        juce::juce_audio_processors
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(AlgoRythmPlugin
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0 # The plugin has no VST2 version
        JUCE_DISPLAY_SPLASH_SCREEN=0
)
