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
    voicing_shapes_test.cpp
    voicing_search_test.cpp
    voicing_constraints_test.cpp
    voicing_cost_test.cpp
    voicing_sequence_test.cpp
    fingering_test.cpp
    voicing_stage_test.cpp
    voicing_property_test.cpp
    voicing_bench_test.cpp
    strum_test.cpp
    strum_property_test.cpp
    render_test.cpp
    smf_test.cpp
    smf_roundtrip_test.cpp
    tab_test.cpp
    engine_test.cpp
    files_test.cpp
    contour_test.cpp
    hook_property_test.cpp
    locks_test.cpp
    reroll_test.cpp
    validate_test.cpp
    musicxml_test.cpp
    tables_test.cpp
    support/smf_reader.cpp
)

target_include_directories(algo_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(algo_tests
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS> # std::getenv
        ALGO_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden"
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
