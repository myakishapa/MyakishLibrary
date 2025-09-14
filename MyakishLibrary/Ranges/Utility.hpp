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


    struct FindElementOrFunctor : functional::ExtensionMethod
    {
        template<std::ranges::input_range Range, class OrElse, class Predicate, class Projection = std::identity>
        std::ranges::range_value_t<Range> operator()(Range&& range, OrElse&& orElse, const Predicate& predicate, Projection projection = {}) const
        {
            auto it = std::ranges::find_if(range, predicate, projection);
            if (it != std::ranges::end(range)) return *it;
            else return std::forward<OrElse>(orElse);
        }
    };
    inline constexpr FindElementOrFunctor FindElementOr;


    template<std::ranges::range Underlying>
    struct ForceBorrowedView : std::ranges::view_interface<ForceBorrowedView<Underlying>>
    {
        Underlying&& range;

        ForceBorrowedView(Underlying&& range) : range(std::forward<Underlying>(range)) {}

        auto begin() const
        {
            return std::ranges::begin(range);
        }
        auto end() const
        {
            return std::ranges::end(range);
        }
    };
    template<std::ranges::range Underlying>
    ForceBorrowedView(Underlying&&) -> ForceBorrowedView<Underlying>;

    struct BorrowFunctor : functional::ExtensionMethod
    {
        template<std::ranges::range Underlying>
        auto operator()(Underlying&& range) const
        {
            return ForceBorrowedView(std::forward<Underlying>(range));
        }
    };
    inline constexpr BorrowFunctor Borrow;
}

template<std::ranges::range Underlying>
constexpr bool std::ranges::enable_borrowed_range<myakish::ranges::ForceBorrowedView<Underlying>> = true;