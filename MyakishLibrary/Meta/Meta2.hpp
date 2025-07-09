#pragma once

#include <type_traits>
#include <utility>

#include <MyakishLibrary/Core.hpp>

namespace myakish::meta2
{
    struct UndefinedResult {};

    struct Undefined { using type = UndefinedResult; };

    template<typename Result>
    struct ReturnType
    {
        using type = Result;
    };

    template<auto Value>
    struct ReturnValue
    {
        inline constexpr static auto value = Value;
    };

    template<typename...>
    struct TypeList {};


    template<Size Index, typename NonList>
    struct At : Undefined {};

    template<Size Index, typename First, typename ...Rest>
    struct At<Index, TypeList<First, Rest...>> : At<Index - 1, TypeList<Rest...>> {};

    template<typename First, typename ...Rest>
    struct At<Size(0), TypeList<First, Rest...>> : ReturnType<First> {};


    template<typename Arg>
    struct ExtractArguments : Undefined {};

    template<template<typename...> typename Template, typename ...Args>
    struct ExtractArguments<Template<Args...>> : ReturnType<TypeList<Args...>> {};

    
    template<template<typename...> typename Template>
    struct Instantiate
    {
        template<typename... Args>
        struct Function : ReturnType<Template<Args...>> {};
    };


    template<template<typename...> typename Function, typename NonList>
    struct Invoke : Undefined {};

    template<template<typename...> typename Function, typename ...Types>
    struct Invoke<Function, TypeList<Types...>> : Function<Types...> {};

    
    template<typename Quoted, typename List>
    struct QuotedInvoke : Invoke<Quoted::template Function, List> {};



    template<template<typename...> typename BaseFunction, typename ...BaseArgs>
    struct LeftCurry
    {
        template<typename ...Args>
        struct Function : BaseFunction<BaseArgs..., Args...> {};
    };

    template<template<typename...> typename BaseFunction, typename ...BaseArgs>
    struct RightCurry
    {
        template<typename ...Args>
        struct Function : BaseFunction<Args..., BaseArgs...> {};
    };


    template<template<typename...> typename Template, typename Arg>
    struct InstanceOf : ReturnValue<false> {};

    template<template<typename...> typename Template, typename ...Args>
    struct InstanceOf<Template, Template<Args...>> : ReturnValue<true> {};


    template<typename Type, template<typename...> typename Template>
    concept InstanceOfConcept = InstanceOf<Template, std::remove_cvref_t<Type>>::value;


    
    template<template<typename...> typename Underlying>
    struct Quote
    {
        template<typename... Args>
        struct Function : Underlying<Args...> {};
    };

    

    
    template<template<typename, typename> typename Func, typename NonList>
    struct RightFold : Undefined {};

    template<template<typename, typename> typename Func, typename First, typename ...Rest>
    struct RightFold<Func, TypeList<First, Rest...>>
    {
        using type = typename Func<First, typename RightFold<Func, TypeList<Rest...>>::type>::type;
    };

    template<template<typename, typename> typename Func, typename Only>
    struct RightFold<Func, TypeList<Only>> : ReturnType<Only> {};


    template<typename Quoted, typename List>
    struct QuotedRightFold : RightFold<Quoted::template Function, List> {};



    namespace detail
    {
        template<typename NonList>
        struct BinaryConcatImpl : Undefined {};

        template<typename ...Types1>
        struct BinaryConcatImpl<TypeList<Types1...>>
        {
            template<typename NonList>
            struct With : Undefined {};

            template<typename ...Types2>
            struct With<TypeList<Types2...>> : ReturnType<TypeList<Types1..., Types2...>> {};
        };

        template<typename List1, typename List2>
        struct BinaryConcat : BinaryConcatImpl<List1>::template With<List2> {};
    }

    template<typename... Lists> 
    struct Concat : RightFold<detail::BinaryConcat, TypeList<Lists...>> {};



    namespace detail
    {
        template<typename NonList>
        struct BinaryZipImpl : Undefined {};

        template<typename ...Types1>
        struct BinaryZipImpl<TypeList<Types1...>>
        {
            template<typename NonList>
            struct With : Undefined {};

            template<typename ...Types2>
            struct With<TypeList<Types2...>> : ReturnType<TypeList<TypeList<Types1, Types2>...>> {};
        };

        template<typename List1, typename List2>
        struct BinaryZip : BinaryZipImpl<List1>::template With<List2> {};
    }

    template<typename... Lists>
    struct Zip : RightFold<detail::BinaryZip, TypeList<Lists...>> {};



    template<template<typename> typename Func, typename NonList>
    struct Apply : Undefined {};

    template<template<typename> typename Func, typename ...Types>
    struct Apply<Func, TypeList<Types...>> : ReturnType<TypeList<typename Func<Types>::type...>> {};

    template<typename Quoted, typename List>
    struct QuotedApply : Apply<Quoted::template Function, List> {};



    template<template<typename> typename Predicate, typename NonList>
    struct Filter : Undefined {};

    template<template<typename> typename Func, typename First, typename ...Rest>
    struct Filter<Func, TypeList<First, Rest...>>
    {
    private:

        using begin = std::conditional_t<Func<First>::value, TypeList<First>, TypeList<>>;

    public:
        
        using type = Concat<begin, typename Filter<Func, TypeList<Rest...>>::type>::type;
    };

    template<template<typename> typename Func>
    struct Filter<Func, TypeList<>>
    {
        using type = TypeList<>;
    };

    template<typename Quoted, typename List>
    struct QuotedFilter : Filter<Quoted::template Function, List> {};



    template<template<typename> typename BaseFunction>
    struct Not
    {
        template<typename... Args>
        struct Function : ReturnValue<!BaseFunction<Args...>::value> {};
    };

    template<typename Quoted>
    struct QuotedNot : Not<Quoted::template Function> {};



    struct QuotedIdentity
    {
        template<typename Arg>
        struct Function : ReturnType<Arg> {};
    };



    template<template<typename...> typename First, template<typename...> typename ...Functions>
    struct Compose
    {
        template<typename... Args>
        struct Function
        {
            using type = typename Compose<Functions...>::template Function<typename First<Args...>::type>::type;
        };
    };

    template<template<typename...> typename Only>
    struct Compose<Only>
    {
        template<typename... Args>
        struct Function : Only<Args...> {};
    };

    template<typename... Quoted>
    struct QuotedCompose : Compose<Quoted::template Function...> {};



    template<typename From, typename To>
    struct CopyConst : std::conditional<std::is_const_v<std::remove_reference_t<From>>, std::add_const_t<To>, To> {};

    template<typename From, typename To>
    struct CopyLReference : std::conditional<std::is_lvalue_reference_v<From>, std::add_lvalue_reference_t<To>, To> {};

    template<typename From, typename To>
    struct CopyRReference : std::conditional<std::is_rvalue_reference_v<From>, std::add_rvalue_reference_t<To>, To> {};

    template<typename From, typename To>
    struct CopyQualifiers
    {
    private:

        using Const = LeftCurry<CopyConst, From>;
        using LRef = LeftCurry<CopyLReference, From>;
        using RRef = LeftCurry<CopyRReference, From>;

    public:

        using type = QuotedCompose<Const, LRef, RRef>::template Function<To>::type;
    };

}