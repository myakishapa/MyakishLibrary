#pragma once
#include <cstdint>

namespace myakish
{
    using Size = std::int64_t;

    namespace literals
    {
        constexpr Size operator ""_ssize(unsigned long long i)
        {
            return i;
        }
    }
}