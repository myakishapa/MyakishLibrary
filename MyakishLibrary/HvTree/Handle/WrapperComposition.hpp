#pragma once

#include <MyakishLibrary/HvTree/HvTree.hpp>
#include <MyakishLibrary/HvTree/Handle/CombinedHashHandle.hpp>
#include <MyakishLibrary/HvTree/Handle/HierarchicalHandle.hpp>

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
            return (... / (Resolve(FamilyTag<Family>{}, std::get<Indices>(wrappers) )) );
        }

        template<typename Family, std::size_t... Indices>
        auto ResolveComposition() const
        {
            return ResolveComposition<Family>(std::make_index_sequence<sizeof...(Wrappers)>{});
        }
    };

    template<typename ...LhsWrappers, typename ...RhsWrappers>
    auto operator/(WrapperComposition<LhsWrappers...> lhs, WrapperComposition<RhsWrappers...> rhs) -> WrapperComposition<LhsWrappers..., RhsWrappers...>
    {
        return { std::tuple_cat(lhs.wrappers, rhs.wrappers) };
    }

    template<typename ...LhsWrappers, WrapperOrHandle Rhs>
    auto operator/(WrapperComposition<LhsWrappers...> lhs, Rhs rhs) -> WrapperComposition<LhsWrappers..., Rhs>
    {
        return { std::tuple_cat(lhs.wrappers, std::tie(rhs)) };
    }

    template<WrapperOrHandle Lhs, typename ...LhsWrappers>
    auto operator/(Lhs lhs, WrapperComposition<LhsWrappers...> rhs) -> WrapperComposition<Lhs, LhsWrappers...>
    {
        return { std::tuple_cat(std::tie(rhs), lhs.wrappers) };
    }

    template<Wrapper Lhs, Wrapper Rhs>
    auto operator/(Lhs lhs, Rhs rhs) -> WrapperComposition<Lhs, Rhs>
    {
        return { std::tie(lhs, rhs) };
    }

    template<typename Family, typename ...Args>
    auto Resolve(FamilyTag<Family>, WrapperComposition<Args...> handle)
    {
        return handle.ResolveComposition<Family>();
    }
}
