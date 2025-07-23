#pragma once

#include <ranges>
#include <optional>

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

namespace myakish::ranges
{
    struct FindElementFunctor : functional::ExtensionMethod
    {
        template<std::ranges::input_range Range, class Type, class Projection = std::identity>
        std::optional<std::ranges::range_value_t<Range>> ExtensionInvoke(Range&& range, const Type& value, Projection projection = {}) const
        {
            auto it = std::ranges::find(range, value, projection);
            if (it != std::ranges::end(range)) return *it;
            else return std::nullopt;
        }
    };
    inline constexpr FindElementFunctor FindElement;
}