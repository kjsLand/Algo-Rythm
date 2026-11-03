# The desktop app (SPEC-11) and the UI library that the app and the plugin share (T16-01).
# The root CMakeLists.txt adds this folder and gets JUCE (cmake/Dependencies.cmake).

# An INTERFACE library: each target that links it compiles the UI sources with its own JUCE settings.
add_library(algo_ui INTERFACE)
target_sources(algo_ui
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/AbTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/AppState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/AudioEngine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/AudioSettingsDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ChordDiagram.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ControlsPanel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ExportPanel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/FretboardView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Library.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/LibraryPanel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/MainComponent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluckVoice.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Preferences.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/RatingControl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Sequencer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/SongView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/StrumView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/TransportBar.cpp
)
target_include_directories(algo_ui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_compile_definitions(algo_ui
    INTERFACE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)
target_link_libraries(algo_ui
    INTERFACE
        algo::core
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
)

if (ALGO_BUILD_APP)
    juce_add_gui_app(AlgoRythm
        PRODUCT_NAME "Algo-Rythm"
        VERSION "${PROJECT_VERSION}"
        COMPANY_NAME "kjsLand"
        BUNDLE_ID "com.kjsland.algorythm"
    )

    target_sources(AlgoRythm PRIVATE Source/Main.cpp)

    target_link_libraries(AlgoRythm
        PRIVATE
            algo_ui
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )

    target_compile_definitions(AlgoRythm
        PRIVATE
            JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:AlgoRythm,JUCE_PRODUCT_NAME>"
            JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:AlgoRythm,JUCE_VERSION>"
    )
endif()

# The app tests link JUCE and the app sources, except Main.cpp (T12-06).
if (ALGO_BUILD_TESTS)
    juce_add_console_app(algo_app_tests PRODUCT_NAME "algo_app_tests")
    target_sources(algo_app_tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/tests/app/sequencer_timing_test.cpp
            ${PROJECT_SOURCE_DIR}/tests/app/pluck_voice_test.cpp
            ${PROJECT_SOURCE_DIR}/tests/app/app_misc_test.cpp
            ${PROJECT_SOURCE_DIR}/tests/app/host_sync_test.cpp
            ${PROJECT_SOURCE_DIR}/tests/app/allocation_counter.cpp
    )
    target_link_libraries(algo_app_tests
        PRIVATE
            algo_ui
            Catch2::Catch2WithMain
            juce::juce_recommended_config_flags
    )
    catch_discover_tests(algo_app_tests TEST_PREFIX "app: ")
endif()
