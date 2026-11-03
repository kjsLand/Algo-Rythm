add_executable(algo_tests
    smoke_test.cpp
    strum_regression_test.cpp
    fretboard_regression_test.cpp
    ${PROJECT_SOURCE_DIR}/plugin/Source/Fretboard.cpp
)

target_include_directories(algo_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/plugin/Source
)

target_link_libraries(algo_tests
    PRIVATE
        Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(algo_tests)
