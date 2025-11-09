#pragma once

#include <type_traits>
#include <utility>

#include <MyakishLibrary/Core.hpp>

namespace myakish::meta
{
    struct UndefinedType {};
    inline constexpr UndefinedType UndefinedValue;

    constexpr bool operator==(UndefinedType, UndefinedType)
    {
        return true;
    }
    constexpr bool operator==(UndefinedType, auto)
    {
        return false;
    }

    struct Undefined 
    {
        using type = UndefinedType; 
        inline constexpr static auto value = UndefinedValue;
    };

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



    template<typename InstantiatedFunction>
    concept TypeDefinedConcept = !std::same_as<typename InstantiatedFunction::type, UndefinedType>;

    template<typename InstantiatedFunction>
    struct TypeDefined : ReturnValue<TypeDefinedConcept<InstantiatedFunction>> {};

    template<typename Result, typename Or>
    struct TypeOr : std::conditional<std::same_as<Result, UndefinedType>, Or, Result> {};


    template<typename InstantiatedFunction>
    concept ValueDefinedConcept = InstantiatedFunction::value != UndefinedValue;

    template<typename InstantiatedFunction>
    struct ValueDefined : ReturnValue<ValueDefinedConcept<InstantiatedFunction>> {};


    template<typename NonList>
    struct Length : Undefined {};

    template<typename ...Args>
    struct Length<TypeList<Args...>> : ReturnValue<Size(sizeof...(Args))> {};


    template<Size Index, typename NonList>
    struct At : Undefined {};

    template<Size Index, typename First, typename ...Rest>
    struct At<Index, TypeList<First, Rest...>> : At<Index - 1, TypeList<Rest...>> {};

    template<typename First, typename ...Rest>
    struct At<Size(0), TypeList<First, Rest...>> : ReturnType<First> {};

    template<typename IndexType, typename List>
    struct QuotedAt : At<IndexType::value, List> {};


    template<bool Condition, auto True, auto False>
    struct ValueConditional : ReturnValue<True> {};
    template<auto True, auto False>
    struct ValueConditional<false, True, False> : ReturnValue<False> {};


    namespace detail
    {
        template<template<typename> typename Predicate, Size Index, typename NonList>
        struct First : Undefined {};

        template<template<typename> typename Predicate, Size Index, typename Begin, typename ...Rest>
        struct First<Predicate, Index, TypeList<Begin, Rest...>>
        {
            inline constexpr static auto value = ValueConditional<Predicate<Begin>::value, Index, First<Predicate, Index + 1, TypeList<Rest...>>::value>::value;
            using type = std::conditional_t<Predicate<Begin>::value, Begin, typename First<Predicate, Index + 1, TypeList<Rest...>>::type>;
        };
    }

    template<template<typename> typename Predicate, typename Arg>
    struct First : detail::First<Predicate, 0, Arg> {};

    template<typename Quoted, typename Arg>
    struct QuotedFirst : First<Quoted::template Function, Arg> {};


    template<auto Value>
    struct ValueType : ReturnValue<Value> {};

    template<auto... Values>
    struct AsTypes : ReturnType<TypeList<ValueType<Values>...>> {};


    template<typename Type>
    struct TypeValueType : ReturnType<Type> {};
    template<typename Type>
    inline constexpr TypeValueType<Type> TypeValue;


    template<template<typename...> typename BaseFunction>
    struct LiftToType
    {
        template<typename ...Args>
        struct Function
        {
            using type = ValueType<BaseFunction<Args...>::value>;
        };
    };
    
    template<typename Quoted>
    struct QuotedLiftToType : LiftToType<Quoted::template Function> {};


    template<typename Type>
    struct ValueProjection : Type {};


    template<typename Type>
    struct Constant
    {
        template<typename...>
        struct Function : ReturnType<Type> {};
    };


    namespace detail
    {
        template<typename>
        struct IntegerSequence {};

        template<std::integral Type, Type... Values>
        struct IntegerSequence<std::integer_sequence<Type, Values...>> : AsTypes<Values...> {};
    }

    template<std::integral Type, myakish::Size Count>
    struct IntegerSequence : detail::IntegerSequence<std::make_integer_sequence<Type, Count>> {};


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

    template<typename Type>
    concept TypeListConcept = InstanceOfConcept<Type, TypeList>;


    template<typename Type, typename Base>
    concept DerivedFrom = std::derived_from<std::remove_cvref_t<Type>, Base>;


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




    template<template<typename, typename> typename Func, typename Init, typename NonList>
    struct ExclusiveScan : Undefined {};

    template<template<typename, typename> typename Func, typename Init, typename First, typename ...Rest>
    struct ExclusiveScan<Func, Init, TypeList<First, Rest...>>
    {
        using type = Concat<TypeList<Init>, typename ExclusiveScan<Func, typename Func<Init, First>::type, TypeList<Rest...>>::type>::type;
    };

    template<template<typename, typename> typename Func, typename Init>
    struct ExclusiveScan<Func, Init, TypeList<>> : ReturnType<TypeList<Init>> {};

    template<typename Quoted, typename Init, typename List>
    struct QuotedExclusiveScan : ExclusiveScan<Quoted::template Function, Init, List> {};




    template<Size Count, typename Type>
    struct Repeat : Concat<TypeList<Type>, typename Repeat<Count - 1, Type>::type> {};

    template<typename Type>
    struct Repeat<Size(0), Type> : ReturnType<TypeList<>> {};


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
    struct Map : Undefined {};

    template<template<typename> typename Func, typename ...Types>
    struct Map<Func, TypeList<Types...>> : ReturnType<TypeList<typename Func<Types>::type...>> {};

    template<typename Quoted, typename List>
    struct QuotedMap : Map<Quoted::template Function, List> {};



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


    template<template<typename> typename Predicate, typename NonList>
    struct AnyOf : Undefined {};

    template<template<typename> typename Predicate>
    struct AnyOf<Predicate, TypeList<>>  : ReturnValue<false> {};

    template<template<typename> typename Predicate, typename First, typename ...Rest>
    struct AnyOf<Predicate, TypeList<First, Rest...>> : ReturnValue<Predicate<First>::value || AnyOf<Predicate, TypeList<Rest...>>::value> {};

    template<typename Quoted, typename List>
    struct QuotedAnyOf : AnyOf<Quoted::template Function, List> {};


    template<template<typename> typename Predicate, typename NonList>
    struct AllOf : Undefined {};

    template<template<typename> typename Predicate>
    struct AllOf<Predicate, TypeList<>> : ReturnValue<true> {};

    template<template<typename> typename Predicate, typename First, typename ...Rest>
    struct AllOf<Predicate, TypeList<First, Rest...>> : ReturnValue<Predicate<First>::value && AllOf<Predicate, TypeList<Rest...>>::value> {};

    template<typename Quoted, typename List>
    struct QuotedAllOf : AllOf<Quoted::template Function, List> {};


    template<template<typename> typename Predicate, typename NonList>
    struct NoneOf : Undefined {};

    template<template<typename> typename Predicate>
    struct NoneOf<Predicate, TypeList<>> : ReturnValue<true> {};

    template<template<typename> typename Predicate, typename First, typename ...Rest>
    struct NoneOf<Predicate, TypeList<First, Rest...>> : ReturnValue<!Predicate<First>::value && NoneOf<Predicate, TypeList<Rest...>>::value> {};

    template<typename Quoted, typename List>
    struct QuotedNoneOf : NoneOf<Quoted::template Function, List> {};



    template<typename NonList, template<typename, typename> typename Comparator = std::is_same>
    struct Unique : Undefined {};

    template<template<typename, typename> typename Comparator, typename First, typename ...Rest>
    struct Unique<TypeList<First, Rest...>, Comparator>
    {
    private:

        using Predicate = LeftCurry<Comparator, First>;

        using begin = std::conditional_t<QuotedNoneOf<Predicate, TypeList<Rest...>>::value, TypeList<First>, TypeList<>>;

    public:

        using type = Concat<begin, typename Unique<TypeList<Rest...>, Comparator>::type>::type;
    };

    template<template<typename, typename> typename Comparator>
    struct Unique<TypeList<>, Comparator>
    {
        using type = TypeList<>;
    };

    template<typename Quoted, typename List>
    struct QuotedUnique : Unique<List, Quoted::template Function> {};



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
        struct Function : Compose<Functions...>::template Function<typename First<Args...>::type> {};
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


    template<typename First, typename Second>
    struct SameBase : std::is_same<std::remove_cvref_t<First>, std::remove_cvref_t<Second>> {};


    template<typename Type>
    concept TriviallyCopyableConcept = std::is_trivially_copyable_v<Type>;

    template<typename First, typename Second>
    concept SameBaseConcept = SameBase<First, Second>::value;

    template<typename Type>
    concept EnumConcept = std::is_scoped_enum_v<Type>;


    template<typename NonList>
    struct ForEachFunctor {};

    template<typename ...Args>
    struct ForEachFunctor<TypeList<Args...>>
    {
        template<typename Function>
        constexpr Function&& operator()(Function&& func) const
        {
            ((std::invoke(std::forward<Function>(func), TypeValue<Args>)), ...);
            return std::forward<Function>(func);
        }
    };
    template<typename List>
    inline constexpr ForEachFunctor<List> ForEach;

    template<auto ConstexprFunction>
    struct IntoMetafunction
    {
        template<typename ...Types>
        struct Function : ReturnValue<std::invoke(ConstexprFunction, Types::value...)> {};
    };


    template<typename Type, template<typename> typename Predicate, template<typename> typename Transform = std::remove_cvref>
    concept UnreferencesToConcept = Predicate<typename Transform<Type>::type>::value;


    template<typename Type, typename RangeValue>
    concept RangeOfConcept = std::ranges::range<Type> && std::same_as<std::ranges::range_value_t<Type>, RangeValue>;




    template<typename Expression>
    struct IsLambdaExpression : std::false_type {};

    template<typename Expression>
    concept LambdaExpressionConcept = IsLambdaExpression<Expression>::value;


    template<typename SimpleType>
    struct Lambda : Constant<SimpleType> {};

    template<typename Underlying>
    struct IsLambdaExpression<Lambda<Underlying>> : std::true_type {};


    struct LambdaExpressionTag {};

    template<std::derived_from<LambdaExpressionTag> Expression>
    struct IsLambdaExpression<Expression> : std::true_type {};



    template<LambdaExpressionConcept Expression>
    struct Lambda<Expression> : Expression {};

    template<template<typename...> typename Underlying, typename... Bound>
    struct Lambda<Underlying<Bound...>>
    {
        template<typename ...Args>
        struct Function : Underlying<typename Lambda<Bound>::template Function<Args...>::type...> {};
    };

    template<Size Index>
    struct Arg : LambdaExpressionTag
    {
        template<typename ...Args>
        struct Function : At<Index, TypeList<Args...>> {};
    };


    template<typename Type>
    struct Protect : LambdaExpressionTag, Constant<Type> {};

    template<Size ListIndex, Size ArgIndex = 0>
    struct AtArg : LambdaExpressionTag, Lambda<QuotedAt<ValueType<ListIndex>, Arg<ArgIndex>>> {};


    namespace shorthands
    {
        template<Size Index>
        struct $ : Arg<Index> {};

        template<Size ListIndex, Size ArgIndex = 0>
        struct $i : AtArg<ListIndex, ArgIndex> {};
    }


    template<template<typename, typename> typename Comparator, typename NonList>
    struct Sort : Undefined {};

    template<template<typename, typename> typename Comparator, typename First, typename... Rest>
    struct Sort<Comparator, TypeList<First, Rest...>>
    {
        using LessThanFirst = RightCurry<Comparator, First>;
        using RestList = TypeList<Rest...>;

        using Lesser = QuotedFilter<LessThanFirst, RestList>::type;
        using Greater = QuotedFilter<QuotedNot<LessThanFirst>, RestList>::type;

        using type = Concat<typename Sort<Comparator, Lesser>::type, TypeList<First>, typename Sort<Comparator, Greater>::type>::type;
    };

    template<template<typename, typename> typename Comparator>
    struct Sort<Comparator, TypeList<>> : ReturnType<TypeList<>> {};

    template<typename QuotedComparator, typename List>
    struct QuotedSort : Sort<QuotedComparator::template Function, List> {};

}