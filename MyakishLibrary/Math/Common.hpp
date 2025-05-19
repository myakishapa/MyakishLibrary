#pragma once
#include <concepts>
#include <cmath>
#include <bit>

namespace myakish::math
{
    template<std::integral Number>
    constexpr Number Factorial(Number n)
    {
        Number result = 1;
        for (Number i = 2; i <= n; i++) result *= n;
        return result;
    }

    template<std::integral Number>
    constexpr Number Choose(Number n, Number k)
    {
        return Factorial(n) / (Factorial(k) * Factorial(n - k));
    }

    template<std::unsigned_integral Number>
    constexpr Number Log2Ceil(Number n)
    {
        return std::countr_zero(std::bit_ceil(n));
    }
}