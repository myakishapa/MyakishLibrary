#pragma once

#include <type_traits>
#include <utility>

namespace myakish::meta2
{
    namespace list
    {
        template<typename...>
        struct TypeList {};

        template<template<typename, typename> typename Function, typename List>
        struct FoldLeftFirst
        {

        };

        template<template<typename, typename> typename Function, template<typename...> typename List, typename First, typename... Rest>
        struct FoldLeftFirst<Function, List<First, Rest...>>
        {
            using type = std::conditional_t<sizeof...(Rest) == 0,
                First,
                typename Function<
                    First,
                    typename FoldLeftFirst<Function, TypeList<Rest...>>::type
                        >::type;
        };
    }
}