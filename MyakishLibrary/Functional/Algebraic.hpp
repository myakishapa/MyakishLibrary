#pragma once
#include <variant>
#include <tuple>

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

#include <MyakishLibrary/Meta/Meta2.hpp>

namespace myakish::functional::algebraic
{

    //variant multitransform
    namespace detail
    {
        template<typename Type>
        concept Variant = meta2::InstanceOfConcept<Type, std::variant>;

        template<Variant VariantType, typename... Functions>
        struct VariantMultitransformReturnType
        {
            using TypesList = meta2::ExtractArguments<std::remove_cvref_t<VariantType>>::type;

            using CopyQualifiersFromVariant = meta2::LeftCurry<meta2::CopyQualifiers, VariantType>;
            using QualifiedTypesList = meta2::QuotedApply<CopyQualifiersFromVariant, TypesList>::type;

            using TypesFunctionsZipped = meta2::Zip<meta2::TypeList<Functions...>, QualifiedTypesList>::type;

            using ResultMetafunction = meta2::LeftCurry<meta2::QuotedInvoke, meta2::Quote<std::invoke_result>>;
            using Results = meta2::QuotedApply<ResultMetafunction, TypesFunctionsZipped>::type;

            using type = meta2::QuotedInvoke<meta2::Instantiate<std::variant>, Results>::type;
        };


        template<std::size_t CurrentIndex, Variant VariantType, typename ...Functions> requires (CurrentIndex >= sizeof...(Functions))
        VariantMultitransformReturnType<VariantType&&, Functions...>::type Multitransform(VariantType&& variant, std::tuple<Functions...> functions)
        {
            std::unreachable();
        }

        template<std::size_t CurrentIndex, Variant VariantType, typename ...Functions> requires (CurrentIndex < sizeof...(Functions))
        VariantMultitransformReturnType<VariantType&&, Functions...>::type Multitransform(VariantType&& variant, std::tuple<Functions...> functions)
        {
            if (variant.index() == CurrentIndex) return  
                std::invoke(
                    std::get<CurrentIndex>(std::move(functions)), 
                    std::get<CurrentIndex>(std::forward<VariantType>(variant)) 
                           );
            else return Multitransform<CurrentIndex + 1>(std::forward<VariantType>(variant), std::move(functions));
        }
    }


    //tuple multitransform
    namespace detail
    {
        template<typename Type>
        concept Tuple = meta2::InstanceOfConcept<Type, std::tuple>;

        template<Tuple TupleType, typename ...Functions, std::size_t ...Indices>
        auto Multitransform(TupleType &&tuple, std::index_sequence<Indices...>, std::tuple<Functions...> functions)
        {
            return std::tuple(std::invoke(std::get<Indices>(std::move(functions)), std::get<Indices>(std::forward<TupleType>(tuple)))...);
        }
    }

    struct MultitransformFunctor : ExtensionMethod
    {
        template<detail::Variant VariantType, typename ...Types, typename ...Functions> requires (std::variant_size_v<std::remove_cvref_t<VariantType>> == sizeof...(Functions))
        constexpr auto ExtensionInvoke(VariantType&& variant, std::tuple<Functions...> functions) const
        {
            return detail::Multitransform<0>(std::forward<VariantType>(variant), std::move(functions));
        }

        template<detail::Variant VariantType, typename ...Types, typename ...Functions> requires (std::variant_size_v<std::remove_cvref_t<VariantType>> == sizeof...(Functions))
        constexpr auto ExtensionInvoke(VariantType&& variant, Functions&&... functions) const
        {
            return ExtensionInvoke(std::forward<VariantType>(variant), std::forward_as_tuple(std::forward<Functions>(functions)...));
        }


        template<detail::Tuple TupleType, typename ...Functions> requires (std::tuple_size_v<std::remove_cvref_t<TupleType>> == sizeof...(Functions))
        constexpr auto ExtensionInvoke(TupleType&& tuple, std::tuple<Functions...> functions) const
        {
            return detail::Multitransform(std::forward<TupleType>(tuple), std::make_index_sequence<sizeof...(Functions)>{}, std::move(functions));
        }

        template<detail::Tuple TupleType, typename ...Functions> requires (std::tuple_size_v<std::remove_cvref_t<TupleType>> == sizeof...(Functions))
        constexpr auto ExtensionInvoke(TupleType&& tuple, Functions&&... functions) const
        {
            return ExtensionInvoke(std::forward<TupleType>(tuple), std::forward_as_tuple(std::forward<Functions>(functions)...));
        }
    };
    inline constexpr MultitransformFunctor Multitransform;



    //variant transform
    namespace detail
    {
        template<Variant VariantType, typename Function>
        struct VariantTransformReturnType
        {
            using TypesList = meta2::ExtractArguments<std::remove_cvref_t<VariantType>>::type;

            using CopyQualifiersFromVariant = meta2::LeftCurry<meta2::CopyQualifiers, VariantType>;
            using QualifiedTypesList = meta2::QuotedApply<CopyQualifiersFromVariant, TypesList>::type;

            using ResultMetafunction = meta2::LeftCurry<std::invoke_result, Function>;
            using Results = meta2::QuotedApply<ResultMetafunction, QualifiedTypesList>::type;

            using type = meta2::QuotedInvoke<meta2::Instantiate<std::variant>, Results>::type;
        };

        template<Variant VariantType, typename Function>
        auto Transform(VariantType&& variant, Function&& function)
        {
            using ReturnType = VariantTransformReturnType<VariantType&&, Function&&>::type;

            auto aux = [&]<typename Arg>(Arg &&arg) -> ReturnType
                {
                    return std::invoke(std::forward<Function>(function), std::forward<Arg>(arg));
                };

            return std::visit(aux, std::forward<VariantType>(variant));
        }
    }

    //tuple transform
    namespace detail
    {
        template<detail::Tuple TupleType, typename Function, std::size_t ...Indices>
        auto Transform(std::index_sequence<Indices...>, TupleType&& tuple, Function&& function)
        {
            return std::tuple(std::invoke(std::forward<Function>(function), std::get<Indices>(std::forward<TupleType>(tuple)))...);
        }
    }

    struct TransformFunctor : ExtensionMethod
    {
        template<detail::Variant VariantType, typename Function>
        constexpr auto ExtensionInvoke(VariantType&& variant, Function&& function) const
        {
            return detail::Transform(std::forward<VariantType>(variant), std::forward<Function>(function));
        }

        template<detail::Tuple TupleType, typename Function>
        constexpr auto ExtensionInvoke(TupleType&& tuple, Function&& function) const
        {
            return detail::Transform(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<TupleType>>>{}, std::forward<TupleType>(tuple), std::forward<Function>(function));
        }
    };
    inline constexpr TransformFunctor Transform;


    //select
    namespace detail
    {
        template<Tuple TupleType>
        struct SelectReturnType
        {
            using TypesList = meta2::ExtractArguments<std::remove_cvref_t<TupleType>>::type;

            using type = meta2::QuotedInvoke<meta2::Instantiate<std::variant>, TypesList>::type;
        };

        template<std::size_t CurrentIndex, Tuple TupleType> requires (CurrentIndex >= std::tuple_size_v<std::remove_cvref_t<TupleType>>)
        SelectReturnType<TupleType&&>::type Select(TupleType&& tuple, std::size_t index)
        {
            std::unreachable();
        }

        template<std::size_t CurrentIndex, Tuple TupleType> requires (CurrentIndex < std::tuple_size_v<std::remove_cvref_t<TupleType>>)
        SelectReturnType<TupleType&&>::type Select(TupleType&& tuple, std::size_t index)
        {
            if (index == CurrentIndex) return std::get<CurrentIndex>(std::forward<TupleType>(tuple));
            else return Select<CurrentIndex + 1>(std::forward<TupleType>(tuple), index);
        }
    }

    struct SelectFunctor : ExtensionMethod
    {
        template<detail::Tuple TupleType>
        constexpr auto ExtensionInvoke(TupleType&& tuple, std::size_t runtimeIndex) const
        {
            return detail::Select<0>(std::forward<TupleType>(tuple), runtimeIndex);
        }
    };
    inline constexpr SelectFunctor Select;

    template<detail::Variant To>
    struct CastFunctor : ExtensionMethod
    {
        template<detail::Variant From>
        constexpr auto ExtensionInvoke(From&& from) const
        {
            auto cast = []<typename Arg>(Arg && arg) -> To
            {
                if constexpr (std::constructible_from<To, Arg&&>) return To(std::forward<Arg>(arg));
                else std::unreachable();
            };

            return std::visit(cast, std::forward<From>(from));
        }

        template<detail::Variant From> requires std::same_as<std::remove_cvref_t<From>, To>
        constexpr To ExtensionInvoke(From&& from) const
        {
            return std::forward<From>(from);
        }
    };
    template<detail::Variant To>
    inline constexpr CastFunctor<To> Cast;


    //synthesize
    namespace detail
    {
        template<std::size_t CurrentIndex, Variant VariantType> requires (CurrentIndex >= std::variant_size_v<VariantType>)
        VariantType Synthesize(std::size_t index)
        {
            std::unreachable();
        }

        template<std::size_t CurrentIndex, Variant VariantType> requires (CurrentIndex < std::variant_size_v<VariantType>)
        VariantType Synthesize(std::size_t index)
        {
            if (index == CurrentIndex) return VariantType(std::in_place_index<CurrentIndex>);
            else return Synthesize<CurrentIndex + 1, VariantType>(index);
        }
    }

    template<detail::Variant VariantType>
    struct SynthesizeFunctor : ExtensionMethod
    {
        constexpr auto ExtensionInvoke(std::size_t runtimeIndex) const
        {
            return detail::Synthesize<0, VariantType>(runtimeIndex);
        }
    };
    template<detail::Variant VariantType>
    inline constexpr SynthesizeFunctor<VariantType> Synthesize;

}
