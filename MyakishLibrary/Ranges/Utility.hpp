#pragma once

#include <ranges>
#include <optional>

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>
#include <MyakishLibrary/Algebraic/Algebraic.hpp>

namespace myakish::ranges
{
    struct FindElementFunctor : functional::ExtensionMethod
    {
        template<std::ranges::input_range Range, class Type, class Projection = std::identity>
        algebraic::Variant<std::nullopt_t, std::ranges::range_reference_t<Range>> operator()(Range&& range, const Type& value, Projection projection = {}) const
        {
            auto it = std::ranges::find(range, value, projection);
            if (it != std::ranges::end(range)) return *it;
            else return std::nullopt;
        }
    };
    inline constexpr FindElementFunctor FindElement;

    struct FindElementIfFunctor : functional::ExtensionMethod
    {
        template<std::ranges::input_range Range, class Predicate, class Projection = std::identity>
        algebraic::Variant<std::nullopt_t, std::ranges::range_reference_t<Range>> operator()(Range&& range, const Predicate& predicate, Projection projection = {}) const
        {
            auto it = std::ranges::find_if(range, predicate, projection);
            if (it != std::ranges::end(range)) return *it;
            else return std::nullopt;
        }
    };
    inline constexpr FindElementIfFunctor FindElementIf;

}