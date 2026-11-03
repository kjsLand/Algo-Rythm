# Makes each compiler warning in our own code an error.
# Do not call this on a target that links JUCE modules. JUCE adds its source
# files to the target that links it, so these flags would apply to JUCE too.
function(algo_set_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()
