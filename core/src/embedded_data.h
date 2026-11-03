#pragma once

#include <cstddef>

namespace algo::detail
{
    struct EmbeddedFile
    {
        const char*          name;
        const unsigned char* bytes;
        std::size_t          size;
    };

    // CMake generates these from the files in core/data/. Refer to cmake/EmbedData.cmake.
    extern const EmbeddedFile kEmbeddedFiles[];
    extern const std::size_t  kEmbeddedFileCount;
}
