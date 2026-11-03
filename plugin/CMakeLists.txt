# The root CMakeLists.txt adds this folder and gets JUCE (cmake/Dependencies.cmake).
juce_add_gui_app(AlgoRythm
    PRODUCT_NAME "Algo-Rythm"
    VERSION "1.0.0"
    COMPANY_NAME "kjsLand"
    BUNDLE_ID "com.kjsland.algorythm"
)

# Add source files
target_sources(AlgoRythm
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MainComponent.h
        Source/Fretboard.cpp
        Source/Fretboard.h
        Source/StrumPattern.h
)

# Set include directories
target_include_directories(AlgoRythm
    PRIVATE
        Source
)

# Link JUCE modules
target_link_libraries(AlgoRythm
    PRIVATE
        algo::core
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Compile definitions
target_compile_definitions(AlgoRythm
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:AlgoRythm,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:AlgoRythm,JUCE_VERSION>"
)
