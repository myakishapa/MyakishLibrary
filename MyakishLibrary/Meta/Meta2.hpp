#pragma once

#include <type_traits>
#include <utility>

namespace myakish::meta2
{
    struct Undefined {};

    template<typename Result>
    struct ReturnType
    {
        using type = Result;
    };

    template<auto Value>
    struct ReturnValue
    {
        inline constexpr auto value = Value;
    };

    template<typename...>
    struct TypeList {};

    template<typename Arg>
    struct ExtractArguments : Undefined {};

    template<template<typename...> typename Template, typename ...Args>
    struct ExtractArguments<Template<Args...>> : ReturnType<TypeList<Args...>> {};


    template<template<typename...> typename Template, typename Arg>
    struct InstanceOf : ReturnValue<false> {};

    template<template<typename...> typename Template, typename ...Args>
    struct InstanceOf<Template, Template<Args...>> : ReturnValue<true> {};

}