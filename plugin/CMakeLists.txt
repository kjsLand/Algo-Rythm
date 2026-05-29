cmake_minimum_required(VERSION 3.22)

project(JUCEProject VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE - assumes JUCE is cloned as a subdirectory or found via find_package
# Option A: JUCE as a subdirectory (recommended for beginners)
add_subdirectory(JUCE)

# Option B: If JUCE is installed system-wide or via package manager, use:
# find_package(JUCE CONFIG REQUIRED)

#==============================================================================
juce_add_gui_app(JUCEProject
    PRODUCT_NAME "JUCE Project"
    VERSION "1.0.0"
    COMPANY_NAME "MyCompany"
    BUNDLE_ID "com.mycompany.juceproject"
)

# Add source files
target_sources(JUCEProject
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MainComponent.h
)

# Set include directories
target_include_directories(JUCEProject
    PRIVATE
        Source
)

# Link JUCE modules
target_link_libraries(JUCEProject
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
target_compile_definitions(JUCEProject
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:JUCEProject,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:JUCEProject,JUCE_VERSION>"
)
