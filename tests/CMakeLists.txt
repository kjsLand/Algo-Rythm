add_executable(algo_tests
    smoke_test.cpp
    property_helper_test.cpp
    strum_regression_test.cpp
    fretboard_regression_test.cpp
    version_test.cpp
    types_test.cpp
    json_test.cpp
    scale_registry_test.cpp
    chord_registry_test.cpp
    data_validation_test.cpp
    data_test.cpp
    progression_library_test.cpp
    strum_data_test.cpp
    shape_data_test.cpp
    pitch_test.cpp
    scale_test.cpp
    diatonic_test.cpp
    nashville_test.cpp
    fretboard_test.cpp
    euclid_test.cpp
    rng_test.cpp
    seeds_test.cpp
    settings_test.cpp
    transitions_test.cpp
    plan_test.cpp
    chords_test.cpp
    colour_test.cpp
)

target_include_directories(algo_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/plugin/Source
)

target_compile_definitions(algo_tests
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS> # std::getenv
)

target_link_libraries(algo_tests
    PRIVATE
        Catch2::Catch2WithMain
        algo::core
)

include(Catch)
catch_discover_tests(algo_tests)

# The fixture fails on purpose. This test passes if the failure output shows the seed.
add_test(NAME property_failure_shows_seed
    COMMAND algo_tests "[property-failure-fixture]"
)
set_tests_properties(property_failure_shows_seed
    PROPERTIES PASS_REGULAR_EXPRESSION "Property failed for seed 3"
)
