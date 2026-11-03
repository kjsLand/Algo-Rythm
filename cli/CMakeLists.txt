# The command-line tool. It links only to the core and CLI11, never to JUCE (REQ-10-1).
add_executable(algo-cli
    main.cpp
    settings_args.cpp
    cmd_generate.cpp
    cmd_render.cpp
    cmd_batch.cpp
    cmd_check.cpp
)

target_link_libraries(algo-cli
    PRIVATE
        algo::core
        CLI11::CLI11
)

algo_set_warnings(algo-cli)

if (ALGO_BUILD_TESTS)
    set(cli $<TARGET_FILE:algo-cli>)
    set(work ${CMAKE_CURRENT_BINARY_DIR}/smoke)
    set(expect ${CMAKE_CURRENT_SOURCE_DIR}/tests/expect_exit.cmake)

    # algo_cli_test(<name> <exit code> <output regex or ""> <arguments>...)
    function(algo_cli_test name code match)
        set(command "${cli};${ARGN}")
        if (match STREQUAL "")
            add_test(NAME cli_${name} COMMAND ${CMAKE_COMMAND} -DEXPECT=${code} "-DCMD=${command}" -P ${expect})
        else()
            add_test(NAME cli_${name} COMMAND ${CMAKE_COMMAND} -DEXPECT=${code} "-DCMD=${command}" "-DOUTPUT_MATCH=${match}" -P ${expect})
        endif()
    endfunction()

    algo_cli_test(version 0 "0\\.1\\.0" --version)
    algo_cli_test(help 0 "generate.*render.*batch.*check" --help)
    foreach (sub generate render batch check)
        algo_cli_test(help_${sub} 0 "OPTIONS" ${sub} --help)
    endforeach()
    algo_cli_test(no_command 2 "" )
    algo_cli_test(bad_key 2 "Unknown key: H" generate --key H --seed 1 --out ${work}/bad)
    algo_cli_test(bad_bpm 2 "BPM" generate --bpm fast --seed 1 --out ${work}/bad)
    algo_cli_test(bad_option 2 "" generate --nope)

    algo_cli_test(generate 0 "Seed: 81723" generate --key G --mood bright --bpm 96 --form vcb --difficulty beginner --seed 81723 --out ${work}/generate)
    set_tests_properties(cli_generate PROPERTIES FIXTURES_SETUP cli_generated)

    algo_cli_test(generate_all_formats 0 "musicxml" generate --seed 5 --hook --difficulty advanced --tuning DADGAD --formats json,mid,tab,musicxml --out ${work}/all)

    # generate, then check its output.
    algo_cli_test(check_generated 0 "OK" check ${work}/generate/G-Major-96bpm-81723.json)
    set_tests_properties(cli_check_generated PROPERTIES FIXTURES_REQUIRED cli_generated)

    algo_cli_test(render_mid 0 "" render ${work}/generate/G-Major-96bpm-81723.json --out ${work}/render/starter.mid)
    algo_cli_test(render_tab 0 "" render ${work}/generate/G-Major-96bpm-81723.json --out ${work}/render/starter.txt)
    algo_cli_test(render_bad_extension 2 "extension" render ${work}/generate/G-Major-96bpm-81723.json --out ${work}/render/starter.wav)
    set_tests_properties(cli_render_mid cli_render_tab cli_render_bad_extension PROPERTIES FIXTURES_REQUIRED cli_generated)

    algo_cli_test(batch 0 "Generated 60 of 60" batch --count 60 --start-seed 100 --formats json --out ${work}/batch)

    file(GLOB golden_json ${PROJECT_SOURCE_DIR}/tests/golden/json/*.json)
    algo_cli_test(check_golden 0 "" check ${golden_json})
    algo_cli_test(check_broken 1 "one-note-per-string" check ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/two-notes-one-string.json)
    algo_cli_test(check_missing 1 "load" check ${work}/no-such-file.json)

    add_test(NAME cli_same_seed_same_files
        COMMAND ${CMAKE_COMMAND} "-DCMD=${cli};generate;--seed;42;--hook;--formats;json,mid,tab" -DWORK=${work}/same -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/same_files.cmake
    )
endif()
