#pragma once
#include <variant>
#include <tuple>

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

#include <MyakishLibrary/Meta/Meta2.hpp>

namespace myakish::functional::algebraic
{
    template<typename To, typename... SrcTypes>
    To CastVariant(std::variant<SrcTypes...> variant)
    {
        auto transform = []<typename Arg>(Arg arg) -> To 
        { 
            if constexpr (std::convertible_to<Arg, To>) return To(arg);
            else std::unreachable();
        };
        return std::visit(transform, variant);
    }

    //variant multitransform
    namespace detail
    {
        template<typename OnlyType, typename OnlyFunc>
        auto Multitransform(std::variant<OnlyType> variant, OnlyFunc func)
        {
            return std::variant<std::invoke_result_t<OnlyFunc, OnlyType>>(func(std::get<0>(variant)));
        }

        template<typename FirstType, typename ...Types, typename FirstFunc, typename ...Functions>
        auto Multitransform(std::variant<FirstType, Types...> variant, FirstFunc func, Functions... functions)
        {
            using ReturnType = std::variant<std::invoke_result_t<FirstFunc, FirstType>, std::invoke_result_t<Functions, Types>...>;

            if (variant.index() == 0) return ReturnType(func(std::get<0>(variant)));
            else return CastVariant<ReturnType>(Multitransform(CastVariant<std::variant<Types...>>(variant), functions...));
        }
    }

    //tuple multitransform
    namespace detail
    {
        template<typename ...Types, typename ...Functions, std::size_t ...Indices>
        auto Multitransform(std::index_sequence<Indices...>, std::tuple<Types...> tuple, Functions... functions)
        {
            return std::tuple<std::invoke_result_t<Functions, Types>...>(functions(std::get<Indices>(tuple))...);
        }
    }

    struct MultitransformFunctor : ExtensionMethod
    {
        template<typename ...Types, typename ...Functions>
        constexpr auto ExtensionInvoke(std::variant<Types...> variant, Functions... functions) const
        {
            return detail::Multitransform(variant, functions...);
        }

        template<typename ...Types, typename ...Functions>
        constexpr auto ExtensionInvoke(std::tuple<Types...> tuple, Functions... functions) const
        {
            return detail::Multitransform(std::make_index_sequence<sizeof...(Types)>{}, tuple, functions...);
        }
    };
    inline constexpr MultitransformFunctor Multitransform;


    //tuple transform
    namespace detail
    {
        template<typename ...Types, typename Function, std::size_t ...Indices>
        auto Transform(std::index_sequence<Indices...>, std::tuple<Types...> tuple, Function function)
        {
            return std::tuple<std::invoke_result_t<Function, Types>...>(function(std::get<Indices>(tuple))...);
        }
    }

    struct TransformFunctor : ExtensionMethod
    {
        template<typename ...Types, typename Function>
        constexpr auto ExtensionInvoke(std::variant<Types...> variant, Function function) const
        {
            using ReturnType = std::variant<std::invoke_result_t<Function, Types>...>;
            auto aux = [&](auto arg) -> ReturnType
                {
                    return function(arg);
                };

            return std::visit(aux, variant);
        }

        template<typename ...Types, typename Function>
        constexpr auto ExtensionInvoke(std::tuple<Types...> tuple, Function function) const
        {
            return detail::Transform(std::make_index_sequence<sizeof...(Types)>{}, tuple, function);
        }
    };
    inline constexpr TransformFunctor Transform;
}
