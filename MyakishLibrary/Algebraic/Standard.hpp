#pragma once

#include <MyakishLibrary/Algebraic/Algebraic.hpp>

#include <tuple>
#include <optional>
#include <variant>

namespace myakish::algebraic
{
    template<meta::InstanceOfConcept<std::variant> Variant>
    inline constexpr bool EnableAlgebraicFor<Variant> = true;

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
    inline constexpr bool EnableAlgebraicFor<Tuple> = true;

    template<Size Index, meta::InstanceOfConcept<std::tuple> Tuple>
    decltype(auto) GetADL(Tuple&& tuple) requires (Index < std::tuple_size_v<std::remove_cvref_t<Tuple>>)
    {
        return std::get<Index>(std::forward<Tuple>(tuple));
    }



    template<Size Index, meta::InstanceOfConcept<std::optional> Optional>
    decltype(auto) GetADL(Optional&& optional)
    {
        if constexpr (Index == 0) return std::forward<Optional>(optional).value();
        else return std::nullopt;
    }


    template<typename ForceDependentName, meta::InstanceOfConcept<std::optional> Optional>
    Size IndexADL(Optional&& optional)
    {
        return optional.has_value();
    }

}
