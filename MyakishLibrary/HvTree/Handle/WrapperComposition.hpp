#pragma once

#include <MyakishLibrary/HvTree/HvTree.hpp>
//#include <MyakishLibrary/HvTree/Handle/CombinedHashHandle.hpp>
//#include <MyakishLibrary/HvTree/Handle/HierarchicalHandle.hpp>

#include <utility>
#include <tuple>

namespace myakish::tree::handle
{
    template<typename ...Wrappers>
    struct WrapperComposition
    {
        std::tuple<Wrappers...> wrappers;

        template<typename Family, std::size_t... Indices>
        auto ResolveComposition(std::index_sequence<Indices...>) const
        {
            return (... / (Resolve(Family{}, std::get<Indices>(wrappers) )) );
        }

        template<typename Family>
        auto ResolveComposition() const
        {
            return ResolveComposition<Family>(std::make_index_sequence<sizeof...(Wrappers)>{});
        }
    };

    template<typename Type>
    inline constexpr bool EnableComposition = false;

    template<Handle Type>
    inline constexpr bool EnableComposition<Type> = true;

    template<typename Type>
    concept Composable = EnableComposition<Type>;

    template<typename ...LhsWrappers, typename ...RhsWrappers>
    auto operator/(WrapperComposition<LhsWrappers...> lhs, WrapperComposition<RhsWrappers...> rhs) -> WrapperComposition<LhsWrappers..., RhsWrappers...>
    {
        return { std::tuple_cat(lhs.wrappers, rhs.wrappers) };
    }

    template<typename ...LhsWrappers, Composable Rhs>
    auto operator/(WrapperComposition<LhsWrappers...> lhs, Rhs rhs) -> WrapperComposition<LhsWrappers..., Rhs>
    {
        return { std::tuple_cat(lhs.wrappers, std::tie(rhs)) };
    }

    template<Composable Lhs, typename ...LhsWrappers>
    auto operator/(Lhs lhs, WrapperComposition<LhsWrappers...> rhs) -> WrapperComposition<Lhs, LhsWrappers...>
    {
        return { std::tuple_cat(std::tie(rhs), lhs.wrappers) };
    }

    template<Composable Lhs, Composable Rhs>
    auto operator/(Lhs lhs, Rhs rhs) -> WrapperComposition<Lhs, Rhs>
    {
        return { std::tie(lhs, rhs) };
    }


    template<typename Family, typename ...Args>
    auto ResolveADL(Family, WrapperComposition<Args...> handle)
    {
        return handle.ResolveComposition<Family>();
    }
}
