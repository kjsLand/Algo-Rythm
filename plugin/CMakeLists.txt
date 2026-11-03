cmake_minimum_required(VERSION 3.22)

project(AlgoRythm VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE - assumes JUCE is cloned as a subdirectory or found via find_package
# Option A: JUCE as a subdirectory (recommended for beginners)
add_subdirectory(JUCE)

# Option B: If JUCE is installed system-wide or via package manager, use:
# find_package(JUCE CONFIG REQUIRED)

#==============================================================================
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
        Source/Bjorklund.h
        Source/ChordRegistry.h
        Source/ScaleRegistry.h
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
