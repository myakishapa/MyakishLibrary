#pragma once

#include <concepts>
#include <ranges>
#include <type_traits>

#include <MyakishLibrary/Meta/Functions.hpp>

namespace myakish::meta
{

    template<typename ...Args>
    struct Pack
    {
        template<template<typename...> typename Dst>
        struct Apply
        {
            using type = Dst<Args...>;
        };

        template<template<typename> typename F>
        struct Transform
        {
            using type = Pack<typename F<Args>::type...>;
        };

        template<typename ...Args2>
        struct Concat
        {
            using type = Pack<Args..., Args2...>;
        };

        template<typename ...Args2>
        struct Concat<Pack<Args2...>>
        {
            using type = Pack<Args..., Args2...>;
        };

        template<typename T>
        struct AddUnique
        {
            using type = std::conditional_t<OneOfV<T, Args...>, Pack<Args...>, Pack<Args..., T>>;
        };


        constexpr static auto Count = sizeof...(Args);
    };

    template<typename First, typename ...Rest>
    struct Unique
    {
        using type = typename Unique<Rest...>::type::template AddUnique<First>::type;
    };

    template<typename Only>
    struct Unique<Only>
    {
        using type = Pack<Only>;
    };



    template<typename Pack, template<typename> typename F>
    struct CopyIf
    {

    };

    template<template<typename> typename F>
    struct CopyIf<Pack<>, F>
    {
        using type = Pack<>;
    };

    template<typename First, template<typename> typename F, typename... Rest>
    struct CopyIf<Pack<First, Rest...>, F>
    {
        using type = std::conditional_t<F<First>::value, Pack<First>, Pack<>>::template Concat<typename CopyIf<Pack<Rest...>, F>::type>::type;
    };

    

    template<typename First, typename ...Packs>
    struct ConcatPacks
    {
        using type = typename First::template Concat<typename ConcatPacks<Packs...>::type>::type;
    };

    template<typename Only>
    struct ConcatPacks<Only>
    {
        using type = Only;
    };


    template<typename Type>
    struct ExtractAsPack
    {

    };

    template<template<typename> typename Template, typename ...Args>
    struct ExtractAsPack<Template<Args...>>
    {
        using type = Pack<Args...>;
    };
}