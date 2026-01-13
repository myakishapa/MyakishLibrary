#pragma once

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

#include <MyakishLibrary/Meta.hpp>

#include <MyakishLibrary/Utility.hpp>

#include <tuple>
#include <variant>
#include <optional>

namespace myakish::algebraic
{
    template<Size Index>
    using FromIndexType = meta::ValueType<Index>;
    template<Size Index>
    inline constexpr FromIndexType<Index> FromIndex;

    template<typename Type>
    struct EnableAlgebraicFor : std::false_type {};

    template<typename Type>
    concept AlgebraicConcept = EnableAlgebraicFor<Type>::value;


    struct AlgebraicTag {};
    
    template<meta::DerivedFrom<AlgebraicTag> Type>
    struct EnableAlgebraicFor<Type> : std::true_type {};
     

    namespace detail::get
    {
        template<typename Algebraic, Size Index>
        concept HasMember = requires(Algebraic && algebraic)
        {
            std::forward<Algebraic>(algebraic).template Get<Index>();
        };

        template<typename Algebraic, Size Index>
        concept HasADL = requires(Algebraic && algebraic)
        {
            GetADL<Index>(std::forward<Algebraic>(algebraic));
        };

        enum class Strategy
        {
            None,
            Member,
            ADL
        };

        template<AlgebraicConcept Algebraic, Size Index>
        consteval static Strategy ChooseStrategy()
        {
            using enum Strategy;

            if constexpr (HasMember<Algebraic, Index>) return Member;
            else if constexpr (HasADL<Algebraic, Index>) return ADL;
            else return None;
        }
    }

    template<Size Index>
    struct GetFunctor : functional::ExtensionMethod
    {
        template<AlgebraicConcept Algebraic> requires(detail::get::ChooseStrategy<Algebraic&&, Index>() != detail::get::Strategy::None)
        constexpr decltype(auto) operator()(Algebraic&& algebraic) const
        {
            constexpr auto Strategy = detail::get::ChooseStrategy<Algebraic&&, Index>();

            using enum detail::get::Strategy;

            if constexpr (Strategy == Member) return std::forward<Algebraic>(algebraic).template Get<Index>();
            else if constexpr (Strategy == ADL) return GetADL<Index>(std::forward<Algebraic>(algebraic));
            else static_assert(false);
        }
    };
    template<Size Index>
    inline constexpr GetFunctor<Index> Get;


    template<typename Type, Size Index>
    concept HasUnderlying = AlgebraicConcept<Type> && requires(Type && algebraic)
    {
        Get<Index>(std::forward<Type>(algebraic));
    };

    template<AlgebraicConcept Type, Size Index> requires HasUnderlying<Type, Index>
    struct ValueType
    {
        using type = decltype(Get<Index>(std::declval<Type>()));
    };


    namespace detail
    {
        template<AlgebraicConcept Type, Size BeginIndex> //requires HasUnderlying<Type, BeginIndex>
        struct ValueTypesImpl
        {
            using type = meta::Concat<meta::TypeList<typename ValueType<Type, BeginIndex>::type>, typename ValueTypesImpl<Type, BeginIndex + 1>::type>::type;
        };

        template<AlgebraicConcept Type, Size BeginIndex> requires (!HasUnderlying<Type, BeginIndex>)
            struct ValueTypesImpl<Type, BeginIndex>
        {
            using type = meta::TypeList<>;
        };
    }

    template<AlgebraicConcept Type>
    struct ValueTypes : detail::ValueTypesImpl<Type, 0> {};

    template<AlgebraicConcept Type>
    inline constexpr Size Count = meta::Length<typename ValueTypes<Type>::type>::value;



    namespace detail::index
    {
        template<typename Sum>
        concept HasMember = requires(Sum && sum)
        {
            { std::forward<Sum>(sum).Index() } -> std::convertible_to<myakish::Size>;
        };

        template<typename Sum>
        concept HasADL = requires(Sum && sum)
        {
            { IndexADL<Sum>(std::forward<Sum>(sum)) } -> std::convertible_to<myakish::Size>;
        };


        enum class Strategy
        {
            None,
            Member,
            ADL
        };

        template<AlgebraicConcept Algebraic>
        consteval static Strategy ChooseStrategy()
        {
            using enum Strategy;

            if constexpr (HasMember<Algebraic>) return Member;
            else if constexpr (HasADL<Algebraic>) return ADL;
            else return None;
        }
    }

    struct IndexFunctor : functional::ExtensionMethod
    {
        template<AlgebraicConcept Sum> requires(detail::index::ChooseStrategy<Sum&&>() != detail::index::Strategy::None)
        constexpr myakish::Size operator()(Sum&& sum) const
        {
            constexpr auto Strategy = detail::index::ChooseStrategy<Sum&&>();

            using enum detail::index::Strategy;

            if constexpr (Strategy == Member) return std::forward<Sum>(sum).Index();
            else if constexpr (Strategy == ADL) return IndexADL<Sum>(std::forward<Sum>(sum));
            else static_assert(false);
        }
    };
    inline constexpr IndexFunctor Index;

    template<typename Type>
    concept SumConcept = AlgebraicConcept<Type> && requires(Type && type)
    {
        Index(std::forward<Type>(type));
    };

    template<typename Type>
    concept ProductConcept = AlgebraicConcept<Type> && !SumConcept<Type>;


    template<typename ...Types>
    using Variant = std::variant<functional::WrappedT<Types&&>...>;

    template<typename ...Types>
    using Tuple = std::tuple<functional::WrappedT<Types&&>...>;


    template<meta::InstanceOfConcept<std::variant> Variant>
    struct EnableAlgebraicFor<Variant> : std::true_type {};

    template<Size Index, meta::InstanceOfConcept<std::variant> Variant>
    constexpr decltype(auto) GetADL(Variant&& variant) requires (Index < std::variant_size_v<std::remove_cvref_t<Variant>>)
    {
        return functional::UnwrapOr(std::get<Index>(std::forward<Variant>(variant)));
    }

    template<typename ForceDependentName, meta::InstanceOfConcept<std::variant> Variant>
    constexpr Size IndexADL(Variant&& variant)
    {
        return variant.index();
    }


    template<meta::InstanceOfConcept<std::tuple> Tuple>
    struct EnableAlgebraicFor<Tuple> : std::true_type {};

    template<Size Index, meta::InstanceOfConcept<std::tuple> Tuple>
    constexpr decltype(auto) GetADL(Tuple&& tuple) requires (Index < std::tuple_size_v<std::remove_cvref_t<Tuple>>)
    {
        return functional::UnwrapOr(std::get<Index>(std::forward<Tuple>(tuple)));
    }


    template<meta::InstanceOfConcept<std::optional> Optional>
    struct EnableAlgebraicFor<Optional> : std::true_type {};

    template<Size Index, meta::InstanceOfConcept<std::optional> Optional>
    constexpr decltype(auto) GetADL(Optional&& optional) requires (0 <= Index && Index <= 1)
    {
        if constexpr (Index == 1) return functional::UnwrapOr(std::forward<Optional>(optional).value());
        else return std::nullopt;
    }

    template<typename ForceDependentName, meta::InstanceOfConcept<std::optional> Optional>
    constexpr Size IndexADL(Optional&& optional)
    {
        return static_cast<Size>(optional.has_value());
    }


  
    


    namespace detail
    {
        template<typename Value, Size Count>
        struct RepeatProduct : AlgebraicTag
        {
            Value value;

            constexpr RepeatProduct(Value value) : value(std::move(value)) {}

            template<Size Index, typename Self> requires(Index < Count)
            constexpr decltype(auto) Get(this Self&& self)
            {
                return functional::UnwrapOr(std::forward<Self>(self).value);
            }
        };
    }

    template<Size Count>
    struct RepeatFunctor : functional::ExtensionMethod
    {
        template<typename Type>
        constexpr auto operator()(Type&& value) const
        {
            return detail::RepeatProduct<functional::WrappedT<Type&&>, Count>(std::forward<Type>(value));
        }
    };
    template<Size Count>
    inline constexpr RepeatFunctor<Count> Repeat;



    struct ForwardAsTupleFunctor : functional::ExtensionMethod
    {
        template<typename ...Args>
        constexpr auto operator()(Args&&... args) const
        {
            return Tuple<Args&&...>(std::forward<Args>(args)...);
        }
    };
    inline constexpr ForwardAsTupleFunctor ForwardAsTuple;



    struct VariadicOverloads
    {
        template<typename Self, AlgebraicConcept Type, typename ...Functions> requires (Count<Type&&> == sizeof...(Functions))
        constexpr decltype(auto) operator()(this Self&& self, Type&& algebraic, Functions&&... functions) requires requires { std::forward<Self>(self)(std::forward<Type>(algebraic), ForwardAsTuple(std::forward<Functions>(functions)...)); }
        {
            return std::forward<Self>(self)(std::forward<Type>(algebraic), ForwardAsTuple(std::forward<Functions>(functions)...));
        }

        template<typename Self, AlgebraicConcept Type, typename Function> requires ((Count<Type&&> != 1) && !ProductConcept<Function>)
        constexpr decltype(auto) operator()(this Self&& self, Type&& algebraic, Function&& function) requires requires { std::forward<Self>(self)(std::forward<Type>(algebraic), Repeat<Count<Type&&>>(std::forward<Function>(function))); }
        {
            return std::forward<Self>(self)(std::forward<Type>(algebraic), Repeat<Count<Type&&>>(std::forward<Function>(function)));
        }
    };

    struct DirectVariadicOverloads
    {
        template<typename Self, AlgebraicConcept Type, typename First, typename ...Functions> requires(!ProductConcept<First>)
        constexpr decltype(auto) operator()(this Self&& self, Type&& algebraic, First&& firstFunction, Functions&&... functions) requires requires { std::forward<Self>(self)(std::forward<Type>(algebraic), ForwardAsTuple(std::forward<First>(firstFunction), std::forward<Functions>(functions)...)); }
        {
            return std::forward<Self>(self)(std::forward<Type>(algebraic), ForwardAsTuple(std::forward<First>(firstFunction), std::forward<Functions>(functions)...));
        }
    };


    namespace detail
    {
        template<Size CurrentIndex, Size TypesCount, typename Function>
        constexpr decltype(auto) Visit(Size Index, Function&& function) requires (CurrentIndex < TypesCount)
        {
            if (Index == CurrentIndex) return std::invoke(std::forward<Function>(function), FromIndex<CurrentIndex>);
            else
            {
                if constexpr (CurrentIndex != (TypesCount - 1)) return Visit<CurrentIndex + 1, TypesCount>(Index, std::forward<Function>(function));
                else std::unreachable();
            }
        }

    }

    struct VisitByIndexFunctor : functional::ExtensionMethod
    {
        template<typename First, typename Second>
        struct SameOrUndefined : std::conditional<std::same_as<First, Second>, First, meta::UndefinedType> {};

        template<Size LastIndex, typename Function>
        struct ReturnType
        {
            using Indices = meta::IntegerSequence<Size, LastIndex>::type;

            using Results = meta::QuotedMap<meta::LeftCurry<std::invoke_result, Function>, Indices>::type;

            using type = meta::RightFold<SameOrUndefined, Results>::type;
        };

        template<SumConcept Type, typename Function>
        constexpr decltype(auto) operator()(Type&& sum, Function&& function) const requires requires { operator()(Index(std::forward<Type>(sum)), FromIndex<Count<Type&&>>, std::forward<Function>(function)); }
        {
            return operator()(Index(std::forward<Type>(sum)), FromIndex<Count<Type&&>>, std::forward<Function>(function));
        }

        template<Size LastIndex, typename Function> requires(meta::TypeDefinedConcept<ReturnType<LastIndex, Function>>)
        constexpr ReturnType<LastIndex, Function>::type operator()(Size index, FromIndexType<LastIndex>, Function&& function) const
        {
            return detail::Visit<0, LastIndex>(index, std::forward<Function>(function));
        }
    };
    inline constexpr VisitByIndexFunctor VisitByIndex;

    struct VisitFunctor : functional::ExtensionMethod, VariadicOverloads
    {
        using VariadicOverloads::operator();

        template<SumConcept Type, ProductConcept Functions>
        struct VisitTarget
        {
            Type&& sum;
            Functions&& functions;

            VisitTarget(Type&& sum, Functions&& functions) : sum(std::forward<Type>(sum)), functions(std::forward<Functions>(functions)) {}

            template<Size Index>
            decltype(auto) operator()(FromIndexType<Index>) const
            {
                return std::invoke(Get<Index>(std::forward<Functions>(functions)), Get<Index>(std::forward<Type>(sum)));
            }
        };
        template<SumConcept Type, typename Function>
        VisitTarget(Type&&, Function&&) -> VisitTarget<Type, Function>;

        template<SumConcept Type, ProductConcept Functions>
        constexpr decltype(auto) operator()(Type&& sum, Functions&& functions) const requires std::invocable<VisitByIndexFunctor, Type, VisitTarget<Type, Functions>>
        {
            return VisitByIndex(std::forward<Type>(sum), VisitTarget(std::forward<Type>(sum), std::forward<Functions>(functions)));
        }
    };
    inline constexpr VisitFunctor Visit;

    template<template<typename> typename Predicate>
    struct IsPredicateFunctor : functional::ExtensionMethod
    {
        template<SumConcept Type>
        constexpr auto operator()(Type&& sum) const
        {
            auto Func = [&]<typename Arg>(Arg&&)
            {
                return Predicate<Arg&&>::value;
            };

            return Visit(std::forward<Type>(sum), Func);
        }

    };
    template<typename Quoted>
    struct QuotedIsPredicateFunctor : IsPredicateFunctor<Quoted::template Function> {};

    template<template<typename> typename Predicate>
    inline constexpr IsPredicateFunctor<Predicate> IsPredicate;
    template<typename Quoted>
    inline constexpr QuotedIsPredicateFunctor<Quoted> QuotedIsPredicate;

    template<typename Alternative, template<typename, typename> typename Comparator = meta::SameBase>
    inline constexpr QuotedIsPredicateFunctor<meta::LeftCurry<Comparator, Alternative>> Is;


    template<typename Alternative, template<typename, typename> typename Comparator = meta::SameBase>
    struct GetByTypeFunctor : functional::ExtensionMethod
    {
        template<SumConcept Type>
        constexpr Alternative operator()(Type&& sum) const
        {
            auto Func = [&]<typename Arg>(Arg&& arg) -> Alternative
            {
                if constexpr (Comparator<Arg&&, Alternative>::value) return static_cast<Alternative>(arg);
                else std::unreachable();
            };

            return Visit(std::forward<Type>(sum), Func);
        }
    };
    template<typename Alternative, template<typename, typename> typename Comparator = meta::SameBase>
    inline constexpr GetByTypeFunctor<Alternative, Comparator> GetByType;

    template<Size Index>
    struct MaybeIndexFunctor : functional::ExtensionMethod
    {
        template<ProductConcept Product> requires HasUnderlying<Product, Index>
            constexpr decltype(auto) operator()(Product&& product, auto&&) const
        {
                return Get<Index>(std::forward<Product>(product));
        }

        template<SumConcept Sum> requires HasUnderlying<Sum, Index>
        constexpr decltype(auto) operator()(Sum&& sum, typename ValueType<Sum&&, Index>::type orElse) const
        {
            if (algebraic::Index(std::forward<Sum>(sum)) == Index) return Get<Index>(std::forward<Sum>(sum));
            else return static_cast<typename ValueType<Sum&&, Index>::type>(orElse);
        }
    };
    template<Size Index>
    inline constexpr MaybeIndexFunctor<Index> MaybeIndex;

    template<typename Alternative, template<typename, typename> typename Comparator = meta::SameBase>
    struct MaybeFunctor : functional::ExtensionMethod
    {
        template<SumConcept Type>
        constexpr Alternative operator()(Type&& sum, Alternative orElse) const
        {
            auto Func = [&]<typename Arg>(Arg && arg) -> Alternative
            {
                if constexpr (Comparator<Arg&&, Alternative>::value) return static_cast<Alternative>(arg);
                else return static_cast<Alternative>(orElse);
            };

            return Visit(std::forward<Type>(sum), Func);
        }
    };
    template<typename Alternative, template<typename, typename> typename Comparator = meta::SameBase>
    inline constexpr MaybeFunctor<Alternative, Comparator> Maybe;





    namespace detail
    {
        template<Size CurrentIndex, Size TypesCount, typename Function>
        constexpr void Iterate(Function&& function) requires(CurrentIndex == TypesCount)
        {
            //noop
        }

        template<Size CurrentIndex, Size TypesCount, typename Function>
        constexpr void Iterate(Function&& function)
        {
            std::invoke(std::forward<Function>(function), FromIndex<CurrentIndex>);
            Iterate<CurrentIndex + 1, TypesCount>(std::forward<Function>(function));
        }
    }

    struct IterateByIndexFunctor : functional::ExtensionMethod
    {
        template<ProductConcept Type, typename Function>
        constexpr Function&& operator()(Type&&, Function&& function) const
        {
            return operator()(FromIndex<Count<Type&&>>, std::forward<Function>(function));
        }

        template<Size LastIndex, typename Function>
        constexpr Function&& operator()(FromIndexType<LastIndex>, Function&& function) const
        {
            detail::Iterate<0, LastIndex>(std::forward<Function>(function));
            return std::forward<Function>(function);
        }
    };
    inline constexpr IterateByIndexFunctor IterateByIndex;

    struct IterateFunctor : functional::ExtensionMethod
    {
        template<ProductConcept Type, typename Function>
        constexpr decltype(auto) operator()(Type&& product, Function&& function) const
        {
            auto Func = [&]<Size Index>(FromIndexType<Index>)
            {
                std::invoke(std::forward<Function>(function), Get<Index>(std::forward<Type>(product)));
            };

            IterateByIndex(std::forward<Type>(product), Func);

            return std::forward<Function>(function);
        }

    };
    inline constexpr IterateFunctor Iterate;




    namespace detail
    {
        template<typename Source, typename Transform>
        struct MappedAlgebraic : AlgebraicTag
        {
            Source source;
            Transform transform;

            constexpr MappedAlgebraic(Source source, Transform transform) : source(std::move(source)), transform(std::move(transform)) {}

            template<Size Index, typename Self> requires(Index < Count<functional::UnwrappedLikeT<Source, Self>>)
            constexpr decltype(auto) Get(this Self&& self)
            {
                return std::invoke(algebraic::Get<Index>(functional::UnwrapOr(std::forward<Self>(self).transform)), algebraic::Get<Index>(functional::UnwrapOr(std::forward<Self>(self).source)));
            }

            template<typename Self>
            constexpr Size Index(this Self&& self) requires SumConcept<functional::UnwrappedLikeT<Source, Self>>
            {
                return algebraic::Index(functional::UnwrapOr(std::forward<Self>(self).source));
            }
        };
        template<typename Source, typename Transform>
        MappedAlgebraic(Source&&, Transform&&) -> MappedAlgebraic< functional::WrappedT<Source&&>, functional::WrappedT<Transform&&>>;

    }

    struct MapFunctor : functional::ExtensionMethod, VariadicOverloads
    {
        using VariadicOverloads::operator();

        template<AlgebraicConcept Type, ProductConcept Functions>
        struct ConstraintType
        {
            using Values = ValueTypes<Type>::type;
            using FunctionTypes = ValueTypes<Functions>::type;

            using Zipped = meta::Zip<FunctionTypes, Values>::type;

            using InvocableMetafunction = meta::LeftCurry<meta::QuotedInvoke, meta::Quote<std::is_invocable>>;

            inline constexpr static auto value = meta::QuotedAllOf<InvocableMetafunction, Zipped>::value;
        };

        template<AlgebraicConcept Type, ProductConcept Functions> requires ((Count<Type&&> == Count<Functions&&>) && ConstraintType<Type, Functions>::value)
        constexpr auto operator()(Type&& algebraic, Functions&& functions) const
        {
            return detail::MappedAlgebraic(std::forward<Type>(algebraic), std::forward<Functions>(functions));
        }
    };
    inline constexpr MapFunctor Map;


    namespace detail
    {
        template<typename Function, ProductConcept Type, myakish::Size ...Indices>
        constexpr decltype(auto) Apply(Type&& product, Function&& function, std::integer_sequence<myakish::Size, Indices...>)
        {
            return std::invoke(std::forward<Function>(function), Get<Indices>(std::forward<Type>(product))...);
        }
    }

    struct ApplyFunctor : functional::ExtensionMethod
    {
        template<typename Function, ProductConcept Type>
        constexpr decltype(auto) operator()(Type&& product, Function&& function) const
        {
            return detail::Apply(std::forward<Type>(product), std::forward<Function>(function), std::make_integer_sequence<myakish::Size, Count<Type&&>>{});
        }
    };
    inline constexpr ApplyFunctor Apply;


    struct EvaluateFunctor : functional::ExtensionMethod
    {
        template<AlgebraicConcept Type>
        struct ReturnType
        {
            using Values = ValueTypes<Type>::type;

            using InstantiateMetafunction = std::conditional_t<SumConcept<Type>, meta::Instantiate<Variant>, meta::Instantiate<Tuple>>;

            using type = meta::QuotedInvoke<InstantiateMetafunction, Values>::type;
        };


        template<SumConcept Type>
        constexpr auto operator()(Type&& sum) const
        {
            return Visit(std::forward<Type>(sum), functional::Construct<typename ReturnType<Type&&>::type>);
        }

        template<ProductConcept Type>
        constexpr auto operator()(Type&& product) const
        {
            return Apply(std::forward<Type>(product), functional::Construct<typename ReturnType<Type&&>::type>);
        }
    };
    inline constexpr EvaluateFunctor Evaluate;


    template<SumConcept Type>
    struct SynthesizeFunctor : functional::ExtensionMethod
    {
        using Unqualified = std::remove_cvref_t<Type>;

        template<typename ...Args>
        constexpr auto operator()(Size index, Args&&... args) const
        {
            auto CreateFunc = [&]<Size Index>(FromIndexType<Index>)
            {
                return Unqualified(std::in_place_index<Index>, std::forward<Args>(args)...);
            };

            return VisitByIndex(index, FromIndex<Count<Type>>, CreateFunc);
        }
    };
    template<SumConcept Type>
    inline constexpr SynthesizeFunctor<Type> Synthesize;




    template<typename Underlying, typename FunctionTransform>
    struct FunctionTransformerFunctor : functional::ExtensionMethod, DirectVariadicOverloads
    {
        using DirectVariadicOverloads::operator();

        Underlying underlying;
        FunctionTransform functionTransform;

        constexpr FunctionTransformerFunctor(Underlying underlying, FunctionTransform functionTransform) : underlying(std::move(underlying)), functionTransform(std::move(functionTransform)) {}

        template<typename Self, AlgebraicConcept Type, ProductConcept Functions>
        constexpr decltype(auto) operator()(this Self&& self, Type&& algebraic, Functions&& functions)
            requires std::invocable<functional::UnwrappedLikeT<Underlying, Self>, Type&&, std::invoke_result_t<ApplyFunctor, Functions, functional::UnwrappedLikeT<FunctionTransform, Self>>>
        {
            return std::invoke(functional::UnwrapOr(std::forward<Self>(self).underlying), std::forward<Type>(algebraic), Apply(std::forward<Functions>(functions), functional::UnwrapOr(std::forward<Self>(self).functionTransform)));
        }
    };
    template<typename Underlying, typename FunctionTransform>
    FunctionTransformerFunctor(Underlying&&, FunctionTransform&&) -> FunctionTransformerFunctor<functional::WrappedT<Underlying&&>, functional::WrappedT<FunctionTransform&&>>;

    inline constexpr auto FirstMap = FunctionTransformerFunctor(Map, functional::First);
    inline constexpr auto FirstVisit = FunctionTransformerFunctor(Visit, functional::First);
    inline constexpr auto FitMap = FunctionTransformerFunctor(Map, functional::Overload);
    inline constexpr auto FitVisit = FunctionTransformerFunctor(Visit, functional::Overload);



    template<template<typename, typename> typename Comparator = meta::SameBase, template<typename, typename> typename Transformer = meta::ExactOrUnqualified>
    struct UniqueFunctor : functional::ExtensionMethod
    {
        template<SumConcept Type>
        struct ReturnType
        {
            using Values = ValueTypes<Type>::type;

            using Unique = meta::Unique<Values, Comparator, Transformer>::type;

            using type = meta::QuotedInvoke<meta::Instantiate<Variant>, Unique>::type;
        };

        template<SumConcept Type, typename ResultType = ReturnType<Type&&>::type>
        ResultType operator()(Type&& sum) const
        {           
            return Visit(std::forward<Type>(sum), functional::Construct<ResultType>);
        }
    };
    template<template<typename, typename> typename Comparator = meta::SameBase, template<typename, typename> typename Transformer = meta::ExactOrUnqualified>
    inline constexpr UniqueFunctor<Comparator, Transformer> UniqueVia;
    inline constexpr UniqueFunctor<> Unique;



    struct ValuesFunctor : functional::ExtensionMethod
    {
        template<AlgebraicConcept Type>
        constexpr auto operator()(Type&& algebraic) const
        {
            return std::forward<Type>(algebraic) | Map[functional::MakeCopy] | Evaluate;
        }
    };
    inline constexpr ValuesFunctor Values;

    /*namespace detail
    {
        template<myakish::Size... Indices, ProductConcept ...Products>
        auto Zip(std::integer_sequence<myakish::Size, Indices...>, Products&&... products)
        {
            auto ForwardOne = [&]<myakish::Size Index>
            {
                return ForwardAsTuple(Get<Index>(std::forward<Products>(products))...);
            };

            return ForwardAsTuple(
                ForwardOne.template operator()<Indices>()...
            );
        }
    }

    /*struct ZipFunctor : functional::ExtensionMethod
    {
        template<ProductConcept First, ProductConcept ...Rest>
        auto operator()(First&& first, Rest&&... rest) const
        {
            return detail::Zip(
                std::make_integer_sequence<myakish::Size, Count<First>>{},
                std::forward<First>(first),
                std::forward<Rest>(rest)...
            );
        }

        Tuple<> operator()() const
        {
            return {};
        }
    };
    inline constexpr ZipFunctor Zip;

    /*
    template<template<typename, typename> typename Comparator>
    struct SortFunctor : functional::ExtensionMethod
    {
        template<ProductConcept Type>
        struct ReturnType
        {
            using ValueTypes = ValueTypes<Type>::type;

            using Indices = meta::IntegerSequence<Size, meta::Length<ValueTypes>::value>::type;

            using Zipped = meta::Zip<ValueTypes, Indices>::type;

            using type = meta::QuotedInvoke<meta::Instantiate<Variant>, ValueTypes>::type;
        };

        template<ProductConcept Type>
        auto operator()(Type&& product, Size index) const
        {
            using ResultType = ReturnType<Type&&>::type;

            auto CreateFunc = [&]<Size Index>(FromIndexType<Index>)
            {
                return ResultType(FromIndex<Index>, Get<Index>(std::forward<Type>(product)));
            };

            return VisitByIndex(index, FromIndex<Count<Type&&>>, CreateFunc);
        }
    };
    inline constexpr SortFunctor Sort;
    */



    template<typename Sum>
    struct SumSource
    {
        Sum sum;

        constexpr SumSource(Sum sum) : sum(std::move(sum)) {}

        template<typename Self, typename Invocable> requires std::invocable<VisitFunctor, functional::UnwrappedLikeT<Sum, Self>, Invocable&&>
        constexpr decltype(auto) operator()(this Self&& self, Invocable&& invocable)
        {
            return algebraic::Visit(functional::UnwrapOr(std::forward<Self>(self).sum), std::forward<Invocable>(invocable));
        }
    };
    template<typename Sum>
    SumSource(Sum&&) -> SumSource<functional::WrappedT<Sum&&>>;

    template<typename Product>
    struct ProductSource
    {
        Product product;

        constexpr ProductSource(Product product) : product(std::move(product)) {}

        template<typename Self, typename Invocable> requires std::invocable<ApplyFunctor, functional::UnwrappedLikeT<Product, Self>, Invocable&&>
        constexpr decltype(auto) operator()(this Self&& self, Invocable&& invocable)
        {
            return algebraic::Apply(functional::UnwrapOr(std::forward<Self>(self).product), std::forward<Invocable>(invocable));
        }
    };
    template<typename Product>
    ProductSource(Product&&) -> ProductSource<functional::WrappedT<Product&&>>;

    struct IntoSourceFunctor : functional::ExtensionMethod
    {
        template<typename Arg>
        constexpr auto operator()(Arg&& arg) const
        {
            return functional::ConstantSource(std::forward<Arg>(arg));
        }

        template<SumConcept Arg>
        constexpr auto operator()(Arg&& arg) const
        {
            return SumSource(std::forward<Arg>(arg));
        }

        template<ProductConcept Arg>
        constexpr auto operator()(Arg&& arg) const
        {
            return ProductSource(std::forward<Arg>(arg));
        }
    };
    inline constexpr IntoSourceFunctor IntoSource;


    struct JoinFunctor : functional::ExtensionMethod
    {
        template<typename Type>
        struct UnderlyingProjection : meta::ReturnType<meta::TypeList<Type>> {};
        template<AlgebraicConcept AlgebraicType>
        struct UnderlyingProjection<AlgebraicType> : ValueTypes<AlgebraicType> {};


        template<ProductConcept Type>
        constexpr auto operator()(Type&& product) const
        {
            return Apply(std::forward<Type>(product) | Map[IntoSource], functional::AccumulateSource[ForwardAsTuple, functional::Args]);
        }


        template<SumConcept Type>
        struct SumReturnType
        {
            using Values = ValueTypes<Type>::type;

            using ValuesOfValues = meta::Map<UnderlyingProjection, Values>::type;

            using Sizes = meta::QuotedMap<meta::LiftToType<meta::Length>, ValuesOfValues>::type;

            using Sum = meta::QuotedLiftToType<meta::IntoMetafunction<functional::Plus>>;

            using Indices = meta::QuotedExclusiveScan<Sum, meta::ValueType<0>, Sizes>::type;

            using Flattened = meta::Invoke<meta::Concat, ValuesOfValues>::type;

            using type = meta::QuotedInvoke<meta::Instantiate<Variant>, Flattened>::type;
        };

        template<SumConcept Type, typename ResultType = SumReturnType<Type&&>::type>
        constexpr ResultType operator()(Type&& sum) const
        {
            auto VisitTarget = [&]<Size Index>(FromIndexType<Index>) -> ResultType
            {
                using Underlying = ValueType<Type&&, Index>::type;

                constexpr auto BaseIndex = meta::At<Index, typename SumReturnType<Type&&>::Indices>::type::value;

                if constexpr (SumConcept<Underlying>)
                {
                    Underlying&& underlying = Get<Index>(std::forward<Type>(sum));

                    auto InnerVisitTarget = [&]<Size InnerIndex>(FromIndexType<InnerIndex>) -> ResultType
                    {
                        return ResultType(std::in_place_index<BaseIndex + InnerIndex>, Get<InnerIndex>(std::forward<Underlying>(underlying)));
                    };

                    return VisitByIndex(std::forward<Underlying>(underlying), InnerVisitTarget);
                }
                else return ResultType(std::in_place_index<BaseIndex>, Get<Index>(std::forward<Type>(sum)));
            };

            return VisitByIndex(std::forward<Type>(sum), VisitTarget);
        }
    };
    inline constexpr JoinFunctor Join;

    inline constexpr auto Bind = Map >> Join >> Unique;

    
    struct UnpackLambdaFunctor : functional::ExtensionMethod, functional::DisableLambdaOperatorsTag
    {
        template<typename Expression>
        constexpr auto operator()(Expression&& expression) const
        {
            return functional::BindLambda(std::forward<Expression>(expression), IntoSource);
        }
    };
    inline constexpr UnpackLambdaFunctor UnpackLambda;

    template<myakish::Size ArgIndex>
    inline constexpr auto Arg = functional::Arg<ArgIndex> | UnpackLambda;
    
    template<myakish::Size ValueIndex, myakish::Size ArgIndex = 0>
    inline constexpr auto AtArg = Get<ValueIndex>[functional::Arg<ArgIndex>];
}

namespace myakish::functional
{
    template<typename Arg, PipelinableTo Extension>
    constexpr decltype(auto) operator||(Arg&& arg, Extension&& ext)
    {
        return std::forward<Extension>(ext)(std::forward<Arg>(arg));
    }

    template<algebraic::SumConcept SumType, PipelinableTo Extension>
    constexpr decltype(auto) operator||(SumType&& sum, Extension&& ext)
    {
        return algebraic::Visit(std::forward<SumType>(sum), std::forward<Extension>(ext));
    }
}

namespace myakish::functional::shorthands
{
    inline constexpr auto $0a = algebraic::Arg<0>;
    inline constexpr auto $1a = algebraic::Arg<1>;
    inline constexpr auto $2a = algebraic::Arg<2>;
    inline constexpr auto $3a = algebraic::Arg<3>;
    inline constexpr auto $4a = algebraic::Arg<4>;
    inline constexpr auto $5a = algebraic::Arg<5>;
    inline constexpr auto $6a = algebraic::Arg<6>;
    inline constexpr auto $7a = algebraic::Arg<7>;

    inline constexpr auto $0i0 = algebraic::AtArg<0, 0>;
    inline constexpr auto $0i1 = algebraic::AtArg<1, 0>;
    inline constexpr auto $0i2 = algebraic::AtArg<2, 0>;
    inline constexpr auto $0i3 = algebraic::AtArg<3, 0>;

    inline constexpr auto $1i0 = algebraic::AtArg<0, 1>;
    inline constexpr auto $1i1 = algebraic::AtArg<1, 1>;
    inline constexpr auto $1i2 = algebraic::AtArg<2, 1>;
    inline constexpr auto $1i3 = algebraic::AtArg<3, 1>;
}