#pragma once

#include <ranges>
#include <optional>
#include <utility>

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>
#include <MyakishLibrary/Algebraic/Algebraic.hpp>

namespace myakish::ranges
{
    struct FindElementFunctor : functional::ExtensionMethod
    {
        template<std::ranges::input_range Range, class Type, class Projection = std::identity>
        algebraic::Optional<std::ranges::range_reference_t<Range>> operator()(Range&& range, const Type& value, Projection projection = {}) const
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
        algebraic::Optional<std::ranges::range_reference_t<Range>> operator()(Range&& range, const Predicate& predicate, Projection projection = {}) const
        {
            auto it = std::ranges::find_if(range, predicate, projection);
            if (it != std::ranges::end(range)) return *it;
            else return std::nullopt;
        }
    };
    inline constexpr FindElementIfFunctor FindElementIf;



    template<std::ranges::range Underlying>
    struct ForceBorrowedView : std::ranges::view_interface<ForceBorrowedView<Underlying>>
    {
        std::reference_wrapper<std::remove_reference_t<Underlying>> range;

        ForceBorrowedView(Underlying&& range) : range(range) {}

        auto begin() const
        {
            return std::ranges::begin(range.get());
        }
        auto end() const
        {
            return std::ranges::end(range.get());
        }
    };
    template<std::ranges::range Underlying>
    ForceBorrowedView(Underlying&&) -> ForceBorrowedView<Underlying>;

    struct BorrowFunctor : functional::ExtensionMethod
    {
        template<std::ranges::range Wrapped>
        auto operator()(Wrapped&& range) const
        {
            return ForceBorrowedView(std::forward<Wrapped>(range));
        }

        template<std::ranges::borrowed_range Borrowed>
        decltype(auto) operator()(Borrowed&& range) const
        {
            return std::forward<Borrowed>(range);
        }
    };
    inline constexpr BorrowFunctor Borrow;
}

template<std::ranges::range Underlying>
constexpr bool std::ranges::enable_borrowed_range<myakish::ranges::ForceBorrowedView<Underlying>> = true;