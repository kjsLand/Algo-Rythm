add_library(algo_core STATIC
    src/version.cpp
    src/json.cpp
    src/scale_registry.cpp
    src/chord_registry.cpp
    src/progression_library.cpp
    src/data_validation.cpp
    src/data.cpp
    src/pitch.cpp
    src/scale.cpp
    src/diatonic.cpp
    src/nashville.cpp
    src/fretboard.cpp
    src/euclid.cpp
)
add_library(algo::core ALIAS algo_core)

target_include_directories(algo_core
    PUBLIC
        include
    PRIVATE
        src
)

target_link_libraries(algo_core
    PUBLIC
        nlohmann_json::nlohmann_json
)

algo_set_warnings(algo_core)
algo_embed_data(algo_core ${CMAKE_CURRENT_SOURCE_DIR}/data)
