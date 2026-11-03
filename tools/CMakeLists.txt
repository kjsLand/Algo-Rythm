# Developer tools. These can write to the console, because they are not in the core.
foreach(tool fretboard_print bjorklund_demo)
    add_executable(${tool} ${tool}.cpp)
    target_link_libraries(${tool} PRIVATE algo::core)
    algo_set_warnings(${tool})
endforeach()
