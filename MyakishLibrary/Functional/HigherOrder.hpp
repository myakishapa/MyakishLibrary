#pragma once
#include <concepts>
#include <tuple>
#include <ranges>

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

namespace myakish::functional::higher_order
{
    struct ConstantFunctor
    {
        constexpr auto operator()(auto value) const
        {
            return [=](auto&&...)
                {
                    return value;
                };
        }
    };
    inline constexpr ConstantFunctor Constant;



    namespace detail
    {
        constexpr auto DecayCompose()
        {
            return std::identity{};
        }

        template<typename First, typename ...Rest>
        constexpr auto DecayCompose(First first, Rest... rest)
        {
            return[...rest = std::move(rest), first = std::move(first)]<typename ...Args>(Args&&... args) -> auto
            {
                return DecayCompose(std::move(rest)...)(std::invoke(first, std::forward<Args>(args)...));
            };
        }
    }

    struct DecayComposeFunctor : ExtensionMethod
    {
        template<typename ...Functions>
        constexpr auto operator()(Functions... functions) const
        {
            return detail::DecayCompose(std::move(functions)...);
        }
    };
    inline constexpr DecayComposeFunctor DecayCompose;


    struct MakeCopyFunctor : ExtensionMethod
    {
        template<typename Arg>
        constexpr auto operator()(Arg&& arg) const
        {
            return std::forward<Arg>(arg);
        }
    };
    inline constexpr MakeCopyFunctor MakeCopy;

    template<typename Type>
    struct StaticCastFunctor : ExtensionMethod
    {
        template<typename Arg>
        constexpr auto operator()(Arg&& arg) const
        {
            return static_cast<Type>(std::forward<Arg>(arg));
        }
    };
    template<typename Type>
    inline constexpr StaticCastFunctor<Type> StaticCast;
}