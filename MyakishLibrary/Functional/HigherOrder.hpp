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
            return[...rest = std::move(rest), first = std::move(first)]<typename ...Args>(Args&&... args) -> decltype(auto)
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

    struct AsMutableFunctor : ExtensionMethod
    {
        template<typename Arg>
        constexpr decltype(auto) operator()(Arg&& arg) const
        {
            return const_cast<std::remove_cvref_t<Arg>&>(arg);
        }
    };
    inline constexpr AsMutableFunctor AsMutable;

    struct IdentityFunctor : ExtensionMethod
    {
        template<typename Arg>
        constexpr Arg&& operator()(Arg&& arg) const
        {
            return std::forward<Arg>(arg);
        }
    };
    inline constexpr IdentityFunctor Identity;

    template<typename Type>
    struct ConstructFunctor : ExtensionMethod
    {
        template<typename ...Args>
        constexpr Type operator()(Args&&... args) const
        {
            return Type(std::forward<Args>(args)...);
        }
    };
    template<typename Type>
    inline constexpr ConstructFunctor<Type> Construct;


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