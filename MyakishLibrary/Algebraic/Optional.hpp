#pragma once

#include <MyakishLibrary/Algebraic/Algebraic.hpp>

namespace myakish::algebraic
{
    template<typename Type>
    using Optional = std::optional<functional::WrappedT<Type&&>>;

    template<typename Type, template<typename, typename> typename Comparator = meta::SameBase, template<typename, typename> typename Transformer = meta::ExactOrUnqualified>
    struct OptionalLike : meta::ReturnType<meta::UndefinedType>, std::false_type {};

    template<SumConcept Type, template<typename, typename> typename Comparator, template<typename, typename> typename Transformer> requires(!meta::InstanceOfConcept<Type, std::optional>)
    struct OptionalLike<Type, Comparator, Transformer> 
    {
        using Values = ValueTypes<Type>::type;

        using Unique = meta::Unique<Values, Comparator, Transformer>::type;

        using IsNullopt = meta::LeftCurry<meta::SameBase, std::nullopt_t>;

        inline constexpr static bool HasNullopt = meta::ValueDefinedConcept<meta::QuotedFirst<IsNullopt, Unique>>;
        inline constexpr static bool OneAlternative = meta::Length<Unique>::value == 2;

        using NonNull = typename meta::QuotedFirst<meta::QuotedNot<IsNullopt>, Unique>::type;

        inline constexpr static bool value = HasNullopt && OneAlternative;

        using type = Optional<NonNull>;
    };

    template<meta::InstanceOfConcept<std::optional> Type, template<typename, typename> typename Comparator, template<typename, typename> typename Transformer>
    struct OptionalLike<Type, Comparator, Transformer> 
    {
        using NonNull = ValueType<Type, 1>::type;

        inline constexpr static bool value = true;

        using type = Type;
    };

    template<typename Type, template<typename, typename> typename Comparator = meta::SameBase, template<typename, typename> typename Transformer = meta::ExactOrUnqualified>
    concept OptionalLikeConcept = OptionalLike<Type>::value;

    template<typename Type, template<typename, typename> typename Comparator = meta::SameBase, template<typename, typename> typename Transformer = meta::ExactOrUnqualified>
        requires OptionalLikeConcept<Type, Comparator, Transformer>
    using OptionalLikeT = OptionalLike<Type>::type;

    template<typename Type, template<typename, typename> typename Comparator = meta::SameBase, template<typename, typename> typename Transformer = meta::ExactOrUnqualified>
        requires OptionalLikeConcept<Type, Comparator, Transformer>
    using OptionalValueType = OptionalLike<Type>::NonNull;


    namespace detail
    {
        template<typename Type, template<typename, typename> typename Comparator = meta::SameBase, template<typename, typename> typename Transformer = meta::ExactOrUnqualified>
        concept StandardOptional = meta::InstanceOfConcept<Type, std::optional> && OptionalLikeConcept<Type, Comparator, Transformer>;
    }

    template<template<typename, typename> typename Comparator = meta::SameBase, template<typename, typename> typename Transformer = meta::ExactOrUnqualified>
    struct IntoOptionalFunctor : functional::ExtensionMethod
    {
        template<OptionalLikeConcept<Comparator, Transformer> Type>
            constexpr auto operator()(Type&& sum) const
        {
            return Visit(std::forward<Type>(sum), functional::Construct<OptionalLikeT<Type&&, Comparator, Transformer>>);
        }

        template<detail::StandardOptional<Comparator, Transformer> Type>
        constexpr auto operator()(Type&& opt) const
        {
            return std::forward<Type>(opt);
        }
    };
    template<template<typename, typename> typename Comparator = meta::SameBase, template<typename, typename> typename Transformer = meta::ExactOrUnqualified>
    inline constexpr IntoOptionalFunctor<Comparator, Transformer> IntoOptionalVia;
    inline constexpr IntoOptionalFunctor<> IntoOptional;


    template<template<typename, typename> typename Comparator = meta::SameBase, template<typename, typename> typename Transformer = meta::ExactOrUnqualified>
    struct OptionalValueFunctor : functional::ExtensionMethod
    {
        template<OptionalLikeConcept<Comparator, Transformer> Type>
        constexpr decltype(auto) operator()(Type&& opt) const
        {
            return GetByType<OptionalValueType<Type, Comparator, Transformer>>(std::forward<Type>(opt));
        }

        template<detail::StandardOptional<Comparator, Transformer> Type>
        constexpr decltype(auto) operator()(Type&& opt) const
        {
            return Get<1>(std::forward<Type>(opt));
        }
    };
    template<template<typename, typename> typename Comparator = meta::SameBase, template<typename, typename> typename Transformer = meta::ExactOrUnqualified>
    inline constexpr OptionalValueFunctor<Comparator, Transformer> OptValueVia;
    inline constexpr OptionalValueFunctor<> OptValue;

    struct HasValueFunctor : functional::ExtensionMethod
    {
        template<SumConcept Type>
        constexpr decltype(auto) operator()(Type&& sum) const
        {
            return !Is<std::nullopt_t>(std::forward<Type>(sum));
        }
    };
    inline constexpr HasValueFunctor HasValue;

    template<template<typename, typename> typename Comparator = meta::SameBase, template<typename, typename> typename Transformer = meta::ExactOrUnqualified>
    struct OptionalMapFunctor : functional::ExtensionMethod
    {
        template<OptionalLikeConcept<Comparator, Transformer> Type, typename Function, typename ReturnType = Optional<std::invoke_result_t<Function&&, OptionalValueType<Type, Comparator, Transformer>>> >
        constexpr ReturnType operator()(Type&& opt, Function&& function) const
        {
            if (HasValue(std::forward<Type>(opt))) return ReturnType(std::invoke(std::forward<Function>(function), OptValue(std::forward<Type>(opt))));
            else return std::nullopt;
        }
    };
    template<template<typename, typename> typename Comparator = meta::SameBase, template<typename, typename> typename Transformer = meta::ExactOrUnqualified>
    inline constexpr OptionalMapFunctor<Comparator, Transformer> OptMapVia;
    inline constexpr OptionalMapFunctor<> OptMap;
}