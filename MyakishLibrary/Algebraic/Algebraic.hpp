#pragma once

#include <MyakishLibrary/Algebraic/Accessors.hpp>

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

#include <MyakishLibrary/Meta.hpp>

#include <MyakishLibrary/Utility.hpp>

namespace myakish::algebraic
{

    template<Size Index>
    struct FromIndexType : meta::ReturnValue<Index> {};
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
            decltype(auto) ExtensionInvoke(Algebraic&& algebraic) const
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
        Get<Index>.ExtensionInvoke(std::forward<Type>(algebraic));
    };

    template<AlgebraicConcept Type, Size Index> requires HasUnderlying<Type, Index>
    struct ValueType
    {
        using type = decltype(Get<Index>.ExtensionInvoke(std::declval<Type>()));
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
            auto ExtensionInvoke(Sum&& sum) const
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
        { Index.ExtensionInvoke(std::forward<Type>(type)) } -> std::integral;

        // algebraic.Emplace<Index>(args...)
    };

    template<SumConcept Type>
    struct IndexType : meta::ReturnType<decltype(Index(std::declval<Type>()))> {};


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
        template<SumConcept Type, typename Function>
        decltype(auto) ExtensionInvoke(Type&& sum, Function&& function) const
        {
            return ExtensionInvoke(Index(std::forward<Type>(sum)), FromIndex<Count<Type&&>>, std::forward<Function>(function));
        }

        template<Size LastIndex, typename Function>
        decltype(auto) ExtensionInvoke(Size index, FromIndexType<LastIndex>, Function&& function) const
        {
            return detail::Visit<0, LastIndex>(index, std::forward<Function>(function));

        }
    };
    inline constexpr VisitByIndexFunctor VisitByIndex;

    struct VisitFunctor : functional::ExtensionMethod
    {
        template<SumConcept Type, typename Function>
        decltype(auto) ExtensionInvoke(Type&& sum, Function&& function) const
        {
            auto Func = [&]<Size Index>(FromIndexType<Index>) -> decltype(auto)
            {
                return std::invoke(std::forward<Function>(function), Get<Index>(std::forward<Type>(sum)));
            };

            return VisitByIndex(std::forward<Type>(sum), Func);
        }

    };
    inline constexpr VisitFunctor Visit;


#pragma warning(disable: 26495) // storage is deliberatly left inititialized

    template<AccessorConcept... Accessors>
    struct Sum : AlgebraicTag
    {
        using IndexType = int;
        inline constexpr static Size Count = sizeof...(Accessors);

    private:

        using AccessorList = meta::TypeList<Accessors...>;

        inline constexpr static auto Max = [](auto first, auto second) { return std::max(first, second); };
        inline constexpr static auto Size = RightFold(Max, SizeRequirements<Accessors>...);
        inline constexpr static auto Alignment = RightFold(Max, AlignmentRequirements<Accessors>...);

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
            auto CreateFunc = [&]<myakish::Size Index>(FromIndexType<Index>)
            {
                Create(std::forward<OtherSum>(rhs).Get<Index>());
            };

            VisitByIndex(rhs, CreateFunc);
        }

        template<typename ...Args>
        Sum(Args&&... args) requires ConvertionDeducible<AccessorList, meta::TypeList<Args&&...>>
        {
            Create(std::forward<Args>(args)...);
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



    template<typename Type>
    concept ProductConcept = AlgebraicConcept<Type> && !SumConcept<Type> && requires(Type && type)
    {
        true;
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
        Function&& ExtensionInvoke(Type&&, Function&& function) const
        {
            return ExtensionInvoke(FromIndex<Count<Type&&>>, std::forward<Function>(function));
        }

        template<Size LastIndex, typename Function>
        Function&& ExtensionInvoke(FromIndexType<LastIndex>, Function&& function) const
        {
            detail::Iterate<0, LastIndex>(std::forward<Function>(function));
            return std::forward<Function>(function);
        }
    };
    inline constexpr IterateByIndexFunctor IterateByIndex;


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
        inline constexpr static auto Alignment = myakish::RightFold(Max, myakish::Size(1), AlignmentRequirements<Accessors>...);

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
            auto CreateFunc = [&]<myakish::Size Index>(FromIndexType<Index>)
            {
                Create<Index>(std::forward<OtherProduct>(rhs).Get<Index>());
            };

            IterateByIndex(rhs, CreateFunc);
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

    struct MultitransformFunctor : functional::ExtensionMethod
    {
        template<AlgebraicConcept Type, ProductConcept Functions>
        struct ReturnType
        {
            using Values = ValueTypes<Type>::type;
            using FunctionTypes = ValueTypes<Functions>::type;

            using Zipped = meta::Zip<FunctionTypes, Values>::type;

            using ResultMetafunction = meta::LeftCurry<meta::QuotedInvoke, meta::Quote<std::invoke_result>>;
            using Results = meta::QuotedApply<ResultMetafunction, Zipped>::type;

            using InstantiateMetafunction = std::conditional_t<SumConcept<Type>, meta::Instantiate<Variant>, meta::Instantiate<Tuple>>;

            using type = meta::QuotedInvoke<InstantiateMetafunction, Results>::type;
        };


        template<SumConcept Type, ProductConcept Functions> requires (Count<Type&&> == Count<Functions&&>)
            auto ExtensionInvoke(Type&& sum, Functions&& functions) const
        {
            using ResultType = ReturnType<Type&&, Functions&&>::type;

            auto Func = [&]<Size Index>(FromIndexType<Index>)
            {
                return ResultType(FromIndex<Index>, std::invoke(Get<Index>(std::forward<Functions>(functions)), Get<Index>(std::forward<Type>(sum))));
            };

            return VisitByIndex(sum, Func);
        }

        template<ProductConcept Type, ProductConcept Functions> requires (Count<Type&&> == Count<Functions&&>)
            auto ExtensionInvoke(Type&& product, Functions&& functions) const
        {
            using ResultType = ReturnType<Type&&, Functions&&>::type;

            return detail::Multitransform<ResultType>(std::forward<Type>(product), std::forward<Functions>(functions), std::make_integer_sequence<Size, Count<Type&&>>{});
        }


        template<AlgebraicConcept Type, typename ...Functions> requires (Count<Type&&> == sizeof...(Functions))
            auto ExtensionInvoke(Type&& algebraic, Functions&&... functions) const
        {
            return ExtensionInvoke(std::forward<Type>(algebraic), ForwardAsTuple(std::forward<Functions>(functions)...));
        }
    };
    inline constexpr MultitransformFunctor Multitransform;



    template<typename Type, typename CountType>
    struct RepeatType : AlgebraicTag
    {
        inline constexpr static auto Count = CountType::value;
        using ValueType = Type&&;

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
        auto ExtensionInvoke(Type&& value) const
        {
            return RepeatType<Type, FromIndexType<Count>>(std::forward<Type>(value));
        }
    };
    template<Size Count>
    inline constexpr RepeatFunctor<Count> Repeat;





    struct TransformFunctor : functional::ExtensionMethod
    {
        template<AlgebraicConcept Type, typename Function>
        auto ExtensionInvoke(Type&& algebraic, Function&& function) const
        {
            return Multitransform(std::forward<Type>(algebraic), Repeat<Count<Type&&>>(std::forward<Function>(function)));
        }
    };
    inline constexpr TransformFunctor Transform;


    struct SelectFunctor : functional::ExtensionMethod
    {
        template<ProductConcept Type>
        struct ReturnType
        {
            using ValueTypes = ValueTypes<Type>::type;

            using type = meta::QuotedInvoke<meta::Instantiate<Variant>, ValueTypes>::type;
        };

        template<ProductConcept Type>
        auto ExtensionInvoke(Type&& product, Size index) const
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
        To ExtensionInvoke(From&& from) const
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
        template<typename ...Args>
        auto ExtensionInvoke(Size index, Args&&... args) const
        {
            auto CreateFunc = [&]<Size Index>(FromIndexType<Index>)
            {
                return Type(FromIndex<Index>, std::forward<Args>(args)...);
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
        decltype(auto) ExtensionInvoke(Type&& product, Function&& function) const
        {
            return detail::Apply(std::forward<Type>(product), std::forward<Function>(function), std::make_integer_sequence<myakish::Size, Count<Type&&>>{});
        }
    };
    inline constexpr ApplyFunctor Apply;

}