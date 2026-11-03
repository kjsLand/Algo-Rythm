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
    src/rng.cpp
    src/seeds.cpp
    src/settings.cpp
    src/transitions.cpp
    src/plan.cpp
    src/chords.cpp
    src/fingering.cpp
    src/voicing_shapes.cpp
    src/voicing_search.cpp
    src/voicing_constraints.cpp
    src/voicing_cost.cpp
    src/voicing_sequence.cpp
    src/voicing_stage.cpp
    src/strum.cpp
    src/events.cpp
    src/render.cpp
    src/smf_writer.cpp
    src/tab.cpp
    src/musicxml.cpp
    src/contour.cpp
    src/hook.cpp
    src/engine.cpp
    src/files.cpp
    src/validate.cpp
    src/ratings.cpp
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

# The VST3 plugin is a shared library, so the core must be position-independent code.
set_target_properties(algo_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
