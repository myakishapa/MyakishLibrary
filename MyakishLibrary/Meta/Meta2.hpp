#pragma once

#include <type_traits>
#include <utility>

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

}