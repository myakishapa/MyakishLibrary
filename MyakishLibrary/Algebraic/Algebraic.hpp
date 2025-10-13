#pragma once

#include <MyakishLibrary/Algebraic/Accessors.hpp>

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

#include <MyakishLibrary/Meta.hpp>

#include <MyakishLibrary/Utility.hpp>

#include <tuple>

namespace myakish::algebraic
{
    template<Size Index>
    using FromIndexType = meta::ValueType<Index>;
    template<Size Index>
    inline constexpr FromIndexType<Index> FromIndex;

    template<typename Type>
    inline constexpr bool EnableAlgebraicFor = false;

    template<typename Type>
    concept AlgebraicConcept = EnableAlgebraicFor<Type>;


    struct AlgebraicTag {};

    template<typename Type> requires(std::derived_from<std::remove_cvref_t<Type>, AlgebraicTag>)
        inline constexpr bool EnableAlgebraicFor<Type> = true;


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
            decltype(auto) operator()(Algebraic&& algebraic) const
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
            { std::forward<Sum>(sum).Index() } -> std::integral;
        };

        template<typename Sum>
        concept HasADL = requires(Sum && sum)
        {
            { IndexADL<Sum>(std::forward<Sum>(sum)) } -> std::integral;
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
            auto operator()(Sum&& sum) const
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
        { Index(std::forward<Type>(type)) } -> std::integral;

        // algebraic.Emplace<Index>(args...)
    };

    template<SumConcept Type>
    struct IndexType : meta::ReturnType<decltype(Index(std::declval<Type>()))> {};

    template<typename Type>
    concept ProductConcept = AlgebraicConcept<Type> && !SumConcept<Type> && requires(Type && type)
    {
        true;
    };



    template<typename Type, typename CountType>
    struct RepeatType : AlgebraicTag
    {
        inline constexpr static auto Count = CountType::value;

        Type&& value;

        RepeatType(Type&& value) : value(std::forward<Type>(value)) {}

        template<Size Index> requires(Index < Count)
            Type&& Get() const
        {
            return std::forward<Type>(value);
        }
    };

    template<Size Count>
    struct RepeatFunctor : functional::ExtensionMethod
    {
        template<typename Type>
        auto operator()(Type&& value) const
        {
            return RepeatType<Type, FromIndexType<Count>>(std::forward<Type>(value));
        }
    };
    template<Size Count>
    inline constexpr RepeatFunctor<Count> Repeat;

    namespace detail
    {
        template<typename... References>
        struct ReferenceTuple : AlgebraicTag
        {
            std::tuple<References&&...> references;

            ReferenceTuple(References&&... references) : references(std::forward<References>(references)...) {}

            template<Size Index> requires(Index < Size(sizeof...(References)))
            decltype(auto) Get() const
            {
                return functional::detail::ForwardFromTuple<Index>(references);
            }
        };
        template<typename... References>
        ReferenceTuple(References&&...) -> ReferenceTuple<References...>;


    }

    struct VariadicOverloads
    {
        template<typename Self, AlgebraicConcept Type, typename ...Functions> requires (Count<Type&&> == sizeof...(Functions))
        decltype(auto) operator()(this Self&& self, Type&& algebraic, Functions&&... functions) requires requires { std::forward<Self>(self)(std::forward<Type>(algebraic), detail::ReferenceTuple(std::forward<Functions>(functions)...)); }
        {
            return std::forward<Self>(self)(std::forward<Type>(algebraic), detail::ReferenceTuple(std::forward<Functions>(functions)...));
        }

        template<typename Self, AlgebraicConcept Type, typename Function> requires ((Count<Type&&> != 1) && !ProductConcept<Function>)
        decltype(auto) operator()(this Self&& self, Type&& algebraic, Function&& function) requires requires { std::forward<Self>(self)(std::forward<Type>(algebraic), Repeat<Count<Type&&>>(std::forward<Function>(function))); }
        {
            return std::forward<Self>(self)(std::forward<Type>(algebraic), Repeat<Count<Type&&>>(std::forward<Function>(function)));
        }
    };


    namespace detail
    {
        template<Size CurrentIndex, Size TypesCount, typename Function>
        decltype(auto) Visit(Size Index, Function&& function) requires (CurrentIndex < TypesCount)
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
        decltype(auto) operator()(Type&& sum, Function&& function) const requires requires { operator()(Index(std::forward<Type>(sum)), FromIndex<Count<Type&&>>, std::forward<Function>(function)); }
        {
            return operator()(Index(std::forward<Type>(sum)), FromIndex<Count<Type&&>>, std::forward<Function>(function));
        }

        template<Size LastIndex, typename Function> requires(meta::TypeDefinedConcept<ReturnType<LastIndex, Function>>)
        ReturnType<LastIndex, Function>::type operator()(Size index, FromIndexType<LastIndex>, Function&& function) const
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
        decltype(auto) operator()(Type&& sum, Functions&& functions) const requires std::invocable<VisitByIndexFunctor, Type, VisitTarget<Type, Functions>>
        {
            return VisitByIndex(std::forward<Type>(sum), VisitTarget(std::forward<Type>(sum), std::forward<Functions>(functions)));
        }
    };
    inline constexpr VisitFunctor Visit;

    template<template<typename> typename Predicate>
    struct IsPredicateFunctor : functional::ExtensionMethod
    {
        template<SumConcept Type>
        bool operator()(Type&& sum) const
        {
            auto Func = [&]<typename Arg>(Arg&&) -> bool
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
        Alternative operator()(Type&& sum) const
        {
            auto Func = [&]<typename Arg>(Arg&& arg) -> Alternative
            {
                if constexpr (Comparator<Arg&&, Alternative>::value) return std::forward<Arg>(arg);
                else std::unreachable();
            };

            return Visit(std::forward<Type>(sum), Func);
        }
    };
    template<typename Alternative, template<typename, typename> typename Comparator = meta::SameBase>
    inline constexpr GetByTypeFunctor<Alternative, Comparator> GetByType;


#pragma warning(disable: 26495) // storage is deliberatly left inititialized

    template<AccessorConcept... Accessors>
    struct Sum : AlgebraicTag
    {
        using IndexType = int;
        inline constexpr static Size Count = sizeof...(Accessors);

    private:

        using AccessorList = meta::TypeList<Accessors...>;

        inline constexpr static auto Max = [](auto first, auto second) { return std::max(first, second); };
        inline constexpr static auto Size = functional::RightFold(Max, SizeRequirements<Accessors>...);
        inline constexpr static auto Alignment = functional::RightFold(Max, AlignmentRequirements<Accessors>...);

        alignas(Alignment) std::byte storage[Size];
        IndexType index;

        void Destroy()
        {
            auto Func = [&]<myakish::Size Index>(FromIndexType<Index>)
            {
                using Accessor = meta::At<Index, AccessorList>::type;

                return Accessor::Destroy(const_cast<std::byte*>(storage));
            };

            VisitByIndex(*this, Func);
        }

        template<myakish::Size Index, typename ...Args>
        void Create(Args&&... args)
        {
            using Accessor = meta::At<Index, AccessorList>::type;

            Accessor::Create(storage, std::forward<Args>(args)...);
            index = Index;
        }

        template<typename ...Args>
        void Create(Args&&... args) requires ConvertionDeducible<AccessorList, meta::TypeList<Args&&...>>
        {
            using ArgsList = meta::TypeList<Args&&...>;

            constexpr auto Index = DeduceConvertion<AccessorList, ArgsList>::value;

            //using Predicate = meta::RightCurry<IndirectlyConstructibleFrom, Args&&...>;
            //constexpr auto Index = meta::QuotedFirst<Predicate, AccessorList>::value;

            Create<Index>(std::forward<Args>(args)...);
        }

        template<SumConcept OtherSum>
        void Create(OtherSum&& rhs)
        {
            auto CreateFunc = [&]<myakish::Size Index>(FromIndexType<Index>)
            {
                Create(std::forward<OtherSum>(rhs).Get<Index>());
            };

            VisitByIndex(rhs, CreateFunc);
        }

    public:


        template<typename Qualifiers>
        struct ValueTypes : meta::ReturnType<meta::TypeList<typename AccessorValueType<Accessors, Qualifiers>::type...>> {};

        template<myakish::Size Index, typename ...Args>
        Sum(FromIndexType<Index>, Args&&... args)
        {
            Create<Index>(std::forward<Args>(args)...);
        }

        template<SumConcept OtherSum>
        Sum(OtherSum&& rhs)
        {
            Create(std::forward<OtherSum>(rhs));
        }

        Sum(const Sum& rhs)
        {
            Create(rhs);
        }
        Sum(Sum&& rhs)
        {
            Create(std::move(rhs));
        }


        template<typename ...Args>
        Sum(Args&&... args) requires ConvertionDeducible<AccessorList, meta::TypeList<Args&&...>>
        {
            Create(std::forward<Args>(args)...);
        }

        Sum() requires(IndirectlyConstructibleFrom<Accessors>::value || ...)
        {
            using Predicate = meta::RightCurry<IndirectlyConstructibleFrom>;
            constexpr auto Index = meta::QuotedFirst<Predicate, AccessorList>::value;

            Create<Index>();
        }

        IndexType Index() const
        {
            return index;
        }



        template<myakish::Size Index, typename ...Args>
        void Emplace(Args&&... args)
        {
            Destroy();
            Create<Index>(std::forward<Args>(args)...);
        }

        template<typename ...Args>
        void Emplace(Args&&... args)
        {
            Destroy();
            Create(std::forward<Args>(args)...);
        }


        template<typename Arg>
        Sum& operator=(Arg&& arg)
        {
            Emplace(std::forward<Arg>(arg));
            return *this;
        }

        Sum& operator=(const Sum& rhs)
        {
            Emplace(rhs);
            return *this;
        }
        Sum& operator=(Sum&& rhs)
        {
            Emplace(std::move(rhs));
            return *this;
        }


        template<myakish::Size Index, typename Self> requires (0 <= Index && Index < Count)
            decltype(auto) Get(this Self&& self)
        {
            using Accessor = meta::At<Index, AccessorList>::type;

            if (Index != self.index) std::unreachable();

            return Accessor::template Acquire<Self>(const_cast<std::byte*>(self.storage));
        }


        ~Sum()
        {
            Destroy();
        }
    };






    namespace detail
    {
        template<Size CurrentIndex, Size TypesCount, typename Function>
        void Iterate(Function&& function) requires(CurrentIndex == TypesCount)
        {
            //noop
        }

        template<Size CurrentIndex, Size TypesCount, typename Function>
        void Iterate(Function&& function)
        {
            std::invoke(std::forward<Function>(function), FromIndex<CurrentIndex>);
            Iterate<CurrentIndex + 1, TypesCount>(std::forward<Function>(function));
        }
    }

    struct IterateByIndexFunctor : functional::ExtensionMethod
    {
        template<ProductConcept Type, typename Function>
        Function&& operator()(Type&&, Function&& function) const
        {
            return operator()(FromIndex<Count<Type&&>>, std::forward<Function>(function));
        }

        template<Size LastIndex, typename Function>
        Function&& operator()(FromIndexType<LastIndex>, Function&& function) const
        {
            detail::Iterate<0, LastIndex>(std::forward<Function>(function));
            return std::forward<Function>(function);
        }
    };
    inline constexpr IterateByIndexFunctor IterateByIndex;

    struct IterateFunctor : functional::ExtensionMethod
    {
        template<ProductConcept Type, typename Function>
        decltype(auto) operator()(Type&& product, Function&& function) const
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


    template<AccessorConcept... Accessors>
    struct Product : AlgebraicTag
    {
        inline constexpr static myakish::Size Count = sizeof...(Accessors);

    private:

        using OffsetsType = std::array<myakish::Size, Count>;

        struct OffsetsResultType
        {
            OffsetsType Offsets;
            myakish::Size Size;
        };

        static consteval OffsetsResultType CalculateOffsets()
        {
            constexpr std::array Sizes = { SizeRequirements<Accessors>... };
            constexpr std::array Alignments = { AlignmentRequirements<Accessors>... };

            OffsetsType result;

            myakish::Size currentOffset = 0;

            for (auto&& [offset, size, alignment] : std::views::zip(result, Sizes, Alignments))
            {
                currentOffset += myakish::Padding(currentOffset, alignment);
                offset = currentOffset;
                currentOffset += size;
            }

            return { result , currentOffset };
        }

        using AccessorList = meta::TypeList<Accessors...>;

        inline constexpr static auto Offsets = CalculateOffsets().Offsets;
        inline constexpr static auto Size = CalculateOffsets().Size;
        inline constexpr static auto Max = [](auto first, auto second) { return std::max(first, second); };
        inline constexpr static auto Alignment = functional::RightFold(Max, myakish::Size(1), AlignmentRequirements<Accessors>...);

        template<myakish::Size Index>
        inline constexpr static auto Offset = Offsets[Index];

        alignas(Alignment) std::byte storage[Size];

        template<myakish::Size Index>
        void Destroy()
        {
            using Accessor = meta::At<Index, AccessorList>::type;

            return Accessor::Destroy(const_cast<std::byte*>(storage + Offset<Index>));
        }

        template<myakish::Size CurrentIndex = 0>
        void DestroyAll()
        {
            if constexpr (CurrentIndex < Count)
            {
                Destroy<CurrentIndex>();
                DestroyAll<CurrentIndex + 1>();
            }
        }


        template<myakish::Size Index, typename ...Args>
        void Create(Args&&... args) requires (0 <= Index && Index < Count)
        {
            using Accessor = meta::At<Index, AccessorList>::type;

            Accessor::Create(storage + Offset<Index>, std::forward<Args>(args)...);
        }

        template<ProductConcept OtherProduct>
        void Create(OtherProduct&& rhs)
        {
            auto CreateFunc = [&]<myakish::Size Index>(FromIndexType<Index>)
            {
                Create<Index>(std::forward<OtherProduct>(rhs).Get<Index>());
            };

            IterateByIndex(rhs, CreateFunc);
        }


        template<myakish::Size CurrentIndex>
        void MatchCreate()
        {
            if constexpr (CurrentIndex < Count)
            {
                Create<CurrentIndex>();
                MatchCreate<CurrentIndex + 1>();
            }
        }

        template<myakish::Size CurrentIndex, typename First, typename ...Rest>
        void MatchCreate(First&& first, Rest&&... rest)
        {
            Create<CurrentIndex>(std::forward<First>(first));
            MatchCreate<CurrentIndex + 1>(std::forward<Rest>(rest)...);
        }

    public:

        template<typename ...Args>
        Product(Args&&... args) : storage{}
        {
            MatchCreate<0>(std::forward<Args>(args)...);
        }

        template<ProductConcept OtherProduct>
        Product(OtherProduct&& rhs)
        {
            Create(std::forward<OtherProduct>(rhs));
        }

        
        Product(const Product& rhs)
        {
            Create(rhs);
        }
        Product(Product&& rhs)
        {
            Create(std::move(rhs));
        }


        template<myakish::Size Index, typename ...Args>
        void Emplace(Args&&... args)
        {
            Destroy<Index>();
            Create<Index>(std::forward<Args>(args)...);
        }


        template<typename ...Args>
        void Emplace(Args&&... args)
        {
            DestroyAll();
            MatchCreate<0>(std::forward<Args>(args)...);
        }

        template<ProductConcept OtherProduct>
        void Emplace(OtherProduct&& rhs)
        {
            DestroyAll();
            Create(std::forward<OtherProduct>(rhs));
        }


        template<ProductConcept OtherProduct>
        Product& operator=(OtherProduct&& rhs)
        {
            Emplace(std::forward<OtherProduct>(rhs));
            return *this;
        }

        Product& operator=(const Product& rhs)
        {
            Emplace(rhs);
            return *this;
        }
        Product& operator=(Product&& rhs)
        {
            Emplace(std::move(rhs));
            return *this;
        }



        template<myakish::Size Index, typename Self> requires (0 <= Index && Index < Count)
            decltype(auto) Get(this Self&& self)
        {
            using Accessor = meta::At<Index, AccessorList>::type;

            return Accessor::template Acquire<Self>(const_cast<std::byte*>(self.storage + Offset<Index>));
        }

        ~Product()
        {
            DestroyAll();
        }
    };

    template<>
    struct Product<> : AlgebraicTag
    {
        inline constexpr static myakish::Size Count = 0;

        Product() {}
    };



    template<typename ...Types>
    using Variant = Sum<std::conditional_t<std::is_reference_v<Types>, accessors::Reference<Types>, accessors::Value<Types>>...>;

    template<typename ...Types>
    using Tuple = Product<std::conditional_t<std::is_reference_v<Types>, accessors::Reference<Types>, accessors::Value<Types>>...>;





    struct ForwardAsTupleFunctor
    {
        template<typename ...Args>
        auto operator()(Args&&... args) const
        {
            return Tuple<Args&&...>(std::forward<Args>(args)...);
        }
    };
    inline constexpr ForwardAsTupleFunctor ForwardAsTuple;






    namespace detail
    {
        template<typename ReturnType, ProductConcept Type, ProductConcept Functions, Size... Indices> requires (Count<Type&&> == Count<Functions&&>)
            ReturnType Multitransform(Type&& product, Functions&& functions, std::integer_sequence<Size, Indices...>)
        {
            return ReturnType(std::invoke(Get<Indices>(std::forward<Functions>(functions)), Get<Indices>(std::forward<Type>(product)))...);
        }
    }

    struct MapFunctor : functional::ExtensionMethod, VariadicOverloads
    {
        using VariadicOverloads::operator();

        template<AlgebraicConcept Type, ProductConcept Functions>
        struct ReturnType
        {
            using Values = ValueTypes<Type>::type;
            using FunctionTypes = ValueTypes<Functions>::type;

            using Zipped = meta::Zip<FunctionTypes, Values>::type;

            using ResultMetafunction = meta::LeftCurry<meta::QuotedInvoke, meta::Quote<std::invoke_result>>;
            using Results = meta::QuotedMap<ResultMetafunction, Zipped>::type;

            using InstantiateMetafunction = std::conditional_t<SumConcept<Type>, meta::Instantiate<Variant>, meta::Instantiate<Tuple>>;

            using type = meta::QuotedInvoke<InstantiateMetafunction, Results>::type;
        };

        template<AlgebraicConcept Type, ProductConcept Functions>
        struct ConstraintType
        {
            using Values = ValueTypes<Type>::type;
            using FunctionTypes = ValueTypes<Functions>::type;

            using Zipped = meta::Zip<FunctionTypes, Values>::type;

            using InvocableMetafunction = meta::LeftCurry<meta::QuotedInvoke, meta::Quote<std::is_invocable>>;

            inline constexpr static auto value = meta::QuotedAllOf<InvocableMetafunction, Zipped>::value;
        };

        template<SumConcept Type, ProductConcept Functions> requires ((Count<Type&&> == Count<Functions&&>) && ConstraintType<Type, Functions>::value)
            auto operator()(Type&& sum, Functions&& functions) const
        {
            using ResultType = ReturnType<Type&&, Functions&&>::type;

            auto Func = [&]<Size Index>(FromIndexType<Index>)
            {
                return ResultType(FromIndex<Index>, std::invoke(Get<Index>(std::forward<Functions>(functions)), Get<Index>(std::forward<Type>(sum))));
            };

            return VisitByIndex(sum, Func);
        }

        template<ProductConcept Type, ProductConcept Functions> requires ((Count<Type&&> == Count<Functions&&>) && ConstraintType<Type, Functions>::value)
            auto operator()(Type&& product, Functions&& functions) const
        {
            using ResultType = ReturnType<Type&&, Functions&&>::type;

            return detail::Multitransform<ResultType>(std::forward<Type>(product), std::forward<Functions>(functions), std::make_integer_sequence<Size, Count<Type&&>>{});
        }
    };
    inline constexpr MapFunctor Map;

    template<auto Underlying>
    struct FitFunctor : functional::ExtensionMethod, VariadicOverloads
    {
        using VariadicOverloads::operator();

        template<typename Arg, ProductConcept Functions>
        struct FunctionIndex
        {
            using FunctionTypes = ValueTypes<Functions>::type;

            using Predicate = meta::RightCurry<std::is_invocable, Arg>;

            inline constexpr static auto value = meta::QuotedFirst<Predicate, FunctionTypes>::value;
        };

        template<AlgebraicConcept Type, ProductConcept Functions>
        struct FunctionsProxy : AlgebraicTag
        {
            Functions&& functions;

            FunctionsProxy(Type&&, Functions&& functions) : functions(std::forward<Functions>(functions)) {}

            template<Size Index> requires(Index < Count<Type&&>)
            decltype(auto) Get() const
            {
                using FoundFunctionIndex = FunctionIndex<typename ValueType<Type&&, Index>::type, Functions>;
                if constexpr (meta::ValueDefinedConcept<FoundFunctionIndex>) return algebraic::Get<FoundFunctionIndex::value>(std::forward<Functions>(functions));
                else return functional::Identity;
            }
        };
        template<AlgebraicConcept Type, ProductConcept Functions>
        FunctionsProxy(Type&&, Functions&&) -> FunctionsProxy<Type, Functions>;

        template<AlgebraicConcept Type, ProductConcept Functions>
        decltype(auto) operator()(Type&& algebraic, Functions&& functions) const
        {
            return Underlying(std::forward<Type>(algebraic), FunctionsProxy(std::forward<Type>(algebraic), std::forward<Functions>(functions)));
        }
    };
    inline constexpr FitFunctor<Map> FitMap;
    inline constexpr FitFunctor<Visit> FitVisit;




    struct SelectFunctor : functional::ExtensionMethod
    {
        template<ProductConcept Type>
        struct ReturnType
        {
            using ValueTypes = ValueTypes<Type>::type;

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
    inline constexpr SelectFunctor Select;


    template<SumConcept To>
    struct CastFunctor : functional::ExtensionMethod
    {
        template<SumConcept From>
        To operator()(From&& from) const
        {
            auto CastFunc = []<typename Arg>(Arg && arg) -> To
            {
                if constexpr (std::constructible_from<To, Arg&&>) return To(std::forward<Arg>(arg));
                else std::unreachable();
            };

            return Visit(std::forward<From>(from), CastFunc);
        }
    };
    template<SumConcept To>
    inline constexpr CastFunctor<To> Cast;


    template<SumConcept Type>
    struct SynthesizeFunctor : functional::ExtensionMethod
    {
        using Unqualified = std::remove_cvref_t<Type>;

        template<typename ...Args>
        auto operator()(Size index, Args&&... args) const
        {
            auto CreateFunc = [&]<Size Index>(FromIndexType<Index>)
            {
                return Unqualified(FromIndex<Index>, std::forward<Args>(args)...);
            };

            return VisitByIndex(index, FromIndex<Count<Type>>, CreateFunc);
        }
    };
    template<SumConcept Type>
    inline constexpr SynthesizeFunctor<Type> Synthesize;


    namespace detail
    {
        template<typename Function, ProductConcept Type, myakish::Size ...Indices>
        decltype(auto) Apply(Type&& product, Function&& function, std::integer_sequence<myakish::Size, Indices...>)
        {
            return std::invoke(std::forward<Function>(function), Get<Indices>(std::forward<Type>(product))...);
        }
    }

    struct ApplyFunctor : functional::ExtensionMethod
    {
        template<typename Function, ProductConcept Type>
        decltype(auto) operator()(Type&& product, Function&& function) const
        {
            return detail::Apply(std::forward<Type>(product), std::forward<Function>(function), std::make_integer_sequence<myakish::Size, Count<Type&&>>{});
        }
    };
    inline constexpr ApplyFunctor Apply;

    struct UnpackFunctor : functional::ExtensionMethod
    {
        template<typename Function>
        auto operator()(Function&& function) const
        {
            return [&]<ProductConcept Arg>(Arg && product) -> decltype(auto)
            {
                return Apply(std::forward<Arg>(product), std::forward<Function>(function));
            };
        }
    };
    inline constexpr UnpackFunctor Unpack;



    struct FlattenFunctor : functional::ExtensionMethod
    {
        template<typename Type>
        struct UnderlyingProjection : meta::ReturnType<meta::TypeList<Type>> {};
        template<AlgebraicConcept AlgebraicType>
        struct UnderlyingProjection<AlgebraicType> : ValueTypes<AlgebraicType> {};

        template<ProductConcept Type>
        struct ProductReturnType
        {
            using Values = ValueTypes<Type>::type;

            using ValuesOfValues = meta::Map<UnderlyingProjection, Values>::type;

            using Flattened = meta::Invoke<meta::Concat, ValuesOfValues>::type;

            using type = meta::QuotedInvoke<meta::Instantiate<Tuple>, Flattened>::type;
        };

        template<ProductConcept Type, typename ResultType = ProductReturnType<Type&&>::type>
        ResultType operator()(Type&& product) const
        {
            auto Transform = []<typename Underlying>(Underlying && arg)
            {
                if constexpr (ProductConcept<Underlying>) return [&]<typename Invocable>(Invocable&& invocable) -> decltype(auto) requires true
                {
                    return Apply(std::forward<Underlying>(arg), std::forward<Invocable>(invocable));
                }; 
                else return [&]<std::invocable<Underlying> Invocable>(Invocable && invocable) -> decltype(auto) 
                {
                    return std::invoke(std::forward<Invocable>(invocable), std::forward<Underlying>(arg));
                };
            };

            return Apply(std::forward<Type>(product) | Map[Transform], functional::IndirectAccumulate[functional::Construct<ResultType>, functional::Args]);
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
        ResultType operator()(Type&& sum) const
        {
            auto VisitTarget = [&]<Size Index>(FromIndexType<Index>) -> ResultType
            {
                using Underlying = ValueType<Type&&, Index>::type;

                constexpr auto BaseIndex = meta::At<Index, SumReturnType<Type&&>::Indices>::type::value;

                if constexpr (SumConcept<Underlying>)
                {
                    Underlying&& underlying = Get<Index>(std::forward<Type>(sum));

                    auto InnerVisitTarget = [&]<Size InnerIndex>(FromIndexType<InnerIndex>) -> ResultType
                    {
                        return ResultType(FromIndex<BaseIndex + InnerIndex>, Get<InnerIndex>(std::forward<Underlying>(underlying)));
                    };


                    return VisitByIndex(std::forward<Underlying>(underlying), InnerVisitTarget);
                }
                else return ResultType(FromIndex<BaseIndex>, Get<Index>(std::forward<Type>(sum)));               
            };

            return VisitByIndex(std::forward<Type>(sum), VisitTarget);
        }
    };
    inline constexpr FlattenFunctor Flatten;

    template<template<typename, typename> typename Comparator = meta::SameBase>
    struct UniqueFunctor : functional::ExtensionMethod
    {
        template<SumConcept Type>
        struct ReturnType
        {
            using Values = ValueTypes<Type>::type;

            using Unique = meta::Unique<Values, Comparator>::type;

            using type = meta::QuotedInvoke<meta::Instantiate<Variant>, Unique>::type;
        };

        template<SumConcept Type, typename ResultType = ReturnType<Type&&>::type>
        ResultType operator()(Type&& sum) const
        {           
            return ResultType(std::forward<Type>(sum));
        }
    };
    template<template<typename, typename> typename Comparator = meta::SameBase>
    inline constexpr UniqueFunctor<Comparator> Unique;

    struct ValuesFunctor : functional::ExtensionMethod
    {
        template<AlgebraicConcept Type>
        auto operator()(Type&& algebraic) const
        {
            return std::forward<Type>(algebraic) | Map[functional::MakeCopy];
        }
    };
    inline constexpr ValuesFunctor Values;


    namespace detail
    {
        struct UnpackLambdaTransformType
        {
            template<typename Arg>
            constexpr auto operator()(Arg&& arg) const
            {
                return[&]<std::invocable<Arg&&> Continuation> (Continuation && continuation) -> decltype(auto)
                {
                    return std::invoke(std::forward<Continuation>(continuation), std::forward<Arg>(arg));
                };
            }

            template<SumConcept Arg>
            constexpr auto operator()(Arg&& arg) const
            {
                return[&]<typename Continuation> (Continuation && continuation) -> decltype(auto) requires requires { algebraic::Visit(std::forward<Arg>(arg), std::forward<Continuation>(continuation)); }
                {
                    return algebraic::Visit(std::forward<Arg>(arg), std::forward<Continuation>(continuation));
                };
            }
        };
        inline constexpr UnpackLambdaTransformType UnpackLambdaTransform;
    }

    
    struct LambdaUnpackFunctor : functional::ExtensionMethod, functional::DisableLambdaOperatorsTag
    {
        template<functional::LambdaExpressionConcept Expression>
        constexpr auto operator()(Expression expression) const
        {
            return functional::LambdaTransform(std::move(expression), detail::UnpackLambdaTransform);
        }
    };
    inline constexpr LambdaUnpackFunctor LambdaUnpack;

    template<myakish::Size ArgIndex>
    inline constexpr auto Arg = functional::Arg<ArgIndex> | LambdaUnpack;

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
    inline constexpr auto $a0 = algebraic::Arg<0>;
    inline constexpr auto $a1 = algebraic::Arg<1>;
    inline constexpr auto $a2 = algebraic::Arg<2>;
    inline constexpr auto $a3 = algebraic::Arg<3>;
    inline constexpr auto $a4 = algebraic::Arg<4>;
    inline constexpr auto $a5 = algebraic::Arg<5>;
    inline constexpr auto $a6 = algebraic::Arg<6>;
    inline constexpr auto $a7 = algebraic::Arg<7>;
}