# Script mode. Converts one file to a C++ source file with a byte array.
# The array ends with an extra 0 byte, so an empty file still compiles.
if (CMAKE_SCRIPT_MODE_FILE)
    file(READ "${INPUT}" bytes HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${bytes}")
    file(WRITE "${OUTPUT}"
        "#include <cstddef>\n"
        "namespace algo::detail\n{\n"
        "    extern const unsigned char ${SYMBOL}[];\n"
        "    extern const std::size_t ${SYMBOL}_size;\n"
        "    const unsigned char ${SYMBOL}[] = { ${bytes} 0x00 };\n"
        "    const std::size_t ${SYMBOL}_size = sizeof (${SYMBOL}) - 1;\n"
        "}\n"
    )
    return()
endif()

# Compiles each .json file in data_dir into the target.
# A changed file is converted again in the next build. A new file runs CMake again.
function(algo_embed_data target data_dir)
    file(GLOB data_files CONFIGURE_DEPENDS "${data_dir}/*.json")
    set(out_dir ${CMAKE_CURRENT_BINARY_DIR}/embedded)

    set(declarations "")
    set(rows "")
    set(sources "")

    foreach (data_file IN LISTS data_files)
        get_filename_component(file_name ${data_file} NAME)
        string(MAKE_C_IDENTIFIER "data_${file_name}" symbol)
        set(out_file ${out_dir}/${symbol}.cpp)

        add_custom_command(
            OUTPUT ${out_file}
            COMMAND ${CMAKE_COMMAND}
                -DINPUT=${data_file}
                -DOUTPUT=${out_file}
                -DSYMBOL=${symbol}
                -P ${CMAKE_CURRENT_FUNCTION_LIST_FILE}
            DEPENDS ${data_file} ${CMAKE_CURRENT_FUNCTION_LIST_FILE}
            COMMENT "Embedding ${file_name}"
            VERBATIM
        )

        list(APPEND sources ${out_file})
        string(APPEND declarations
            "    extern const unsigned char ${symbol}[];\n"
            "    extern const std::size_t ${symbol}_size;\n"
        )
        string(APPEND rows "        { \"${file_name}\", ${symbol}, ${symbol}_size },\n")
    endforeach()

    list(LENGTH data_files file_count)
    set(table_file ${out_dir}/embedded_table.cpp)
    file(CONFIGURE
        OUTPUT ${table_file}
        CONTENT
"#include \"embedded_data.h\"

namespace algo::detail
{
${declarations}
    const EmbeddedFile kEmbeddedFiles[] = {
${rows}    };
    const std::size_t kEmbeddedFileCount = ${file_count};
}
"
        @ONLY
    )

    target_sources(${target} PRIVATE ${sources} ${table_file})
endfunction()
