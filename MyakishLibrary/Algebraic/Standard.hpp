#pragma once

#include <MyakishLibrary/Algebraic/Algebraic.hpp>

#include <tuple>
#include <optional>
#include <variant>
#include <utility>

namespace myakish::algebraic
{
    template<meta::InstanceOfConcept<std::variant> Variant>
    struct EnableAlgebraicFor<Variant> : std::true_type {};

    template<Size Index, meta::InstanceOfConcept<std::variant> Variant>
    decltype(auto) GetADL(Variant&& variant) requires (Index < std::variant_size_v<std::remove_cvref_t<Variant>>)
    {
        return std::get<Index>(std::forward<Variant>(variant));
    }

    template<typename ForceDependentName, meta::InstanceOfConcept<std::variant> Variant>
    auto IndexADL(Variant&& variant)
    {
        return variant.index();
    }


    template<meta::InstanceOfConcept<std::tuple> Tuple>
    struct EnableAlgebraicFor<Tuple> : std::true_type {};

    template<Size Index, meta::InstanceOfConcept<std::tuple> Tuple>
    decltype(auto) GetADL(Tuple&& tuple) requires (Index < std::tuple_size_v<std::remove_cvref_t<Tuple>>)
    {
        return std::get<Index>(std::forward<Tuple>(tuple));
    }


    template<meta::InstanceOfConcept<std::optional> Optional>
    struct EnableAlgebraicFor<Optional> : std::true_type {};

    template<Size Index, meta::InstanceOfConcept<std::optional> Optional>
    decltype(auto) GetADL(Optional&& optional) requires (0 <= Index && Index <= 1)
    {
        if constexpr (Index == 1) return std::forward<Optional>(optional).value();
        else return std::nullopt;
    }

    template<typename ForceDependentName, meta::InstanceOfConcept<std::optional> Optional>
    Size IndexADL(Optional&& optional)
    {
        return static_cast<Size>(optional.has_value());
    }

}
