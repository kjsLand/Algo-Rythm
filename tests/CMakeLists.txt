add_executable(algo_tests
    smoke_test.cpp
    property_helper_test.cpp
    strum_regression_test.cpp
    fretboard_regression_test.cpp
    ${PROJECT_SOURCE_DIR}/plugin/Source/Fretboard.cpp
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
