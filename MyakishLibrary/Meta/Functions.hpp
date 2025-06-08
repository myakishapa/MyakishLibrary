#pragma once

#include <concepts>
#include <ranges>
#include <type_traits>

namespace myakish::meta
{
    template<typename Result>
    struct Metafunction
    {
        using type = Result;
    };

    template<template<typename, typename> typename F, typename T1, typename ...Args>
    struct RightFold
    {
        using type = typename F<T1, typename RightFold<F, Args...>::type>::type;
    };

    template<template<typename, typename> typename F, typename T1, typename T2>
    struct RightFold<F, T1, T2>
    {
        using type = typename F<T1, T2>::type;
    };


    template<typename Type, typename First, typename ...Rest>
    struct OneOf
    {
        constexpr static bool value = std::same_as<Type, First> || OneOf<Type, Rest...>::value;
    };

    template<typename Type, typename First>
    struct OneOf<Type, First>
    {
        constexpr static bool value = std::same_as<Type, First>;
    };

    template<typename Type, typename ...Types>
    constexpr bool OneOfV = OneOf<Type, Types...>::value;


    template<template<typename> typename F>
    struct Not
    {
        template<typename Arg>
        struct type
        {
            static constexpr bool value = !(F<Arg>::value);
        };
    };


    template<template<typename...> typename Template>
    struct InstanceOf
    {
        template<typename T>
        struct Func : std::false_type {};

        template<typename ...Args>
        struct Func<Template<Args...>> : std::true_type {};
    };

    template<typename Type, bool Apply, template<typename> typename F>
    struct ApplyIf
    {
        using type = std::conditional_t<Apply, typename F<Type>::type, Type>;
    };

    template<typename Type, bool Const>
    struct ConstIf : ApplyIf<Type, Const, std::add_const> {};

    template<typename Type, bool Const>
    using ConstIfT = ConstIf<Type, Const>::type;

    template<typename Base, typename Copy>
    struct CopyConst : Metafunction<ConstIfT<Base, std::is_const_v<Copy>>> {};
}