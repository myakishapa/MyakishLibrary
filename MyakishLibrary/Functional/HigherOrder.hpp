#pragma once
#include <concepts>
#include <tuple>
#include <ranges>

#include <MyakishLibrary/Functional/Pipeline.hpp>
#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

namespace myakish::functional::higher_order
{
    struct ConstantFunctor : ExtensionMethod, DisallowDirectInvoke, RightCurry
    {
        template<typename Type>
        constexpr Type ExtensionInvoke(Type value, auto&&... _) const
        {
            return value;
        }
    };
    inline constexpr ConstantFunctor Constant;

    constexpr auto DecayCompose()
    {
        return std::identity{};
    }

    template<typename First, typename ...Rest>
    constexpr auto DecayCompose(First first, Rest... rest)
    {
        return [...rest = std::move(rest), first = std::move(first)]<typename ...Args>(Args&&... args) -> auto
        {
            return DecayCompose(std::move(rest)...)(std::invoke(first, std::forward<Args>(args)...));
        };
    }

    struct DecayThenFunctor : ExtensionMethod, DisallowDirectInvoke
    {
        template<typename ...Functions>
        constexpr auto ExtensionInvoke(Functions... functions) const
        {
            return DecayCompose(std::move(functions)...);
        }
    };
    inline constexpr DecayThenFunctor DecayThen;


    struct MakeCopyFunctor : ExtensionMethod
    {
        template<typename Arg>
        constexpr auto ExtensionInvoke(Arg&& arg) const
        {
            return std::forward<Arg>(arg);
        }
    };
    inline constexpr MakeCopyFunctor MakeCopy;

    template<typename Type>
    struct StaticCastFunctor : ExtensionMethod
    {
        template<typename Arg>
        constexpr auto ExtensionInvoke(Arg&& arg) const
        {
            return static_cast<Type>(std::forward<Arg>(arg));
        }
    };
    template<typename Type>
    inline constexpr StaticCastFunctor<Type> StaticCast;
}