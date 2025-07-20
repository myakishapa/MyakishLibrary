#pragma once

#include <concepts>
#include <new>

#include <MyakishLibrary/Core.hpp>
#include <MyakishLibrary/Meta.hpp>

namespace myakish::algebraic
{
    template<typename Type>
    concept AccessorConcept = requires(std::byte * storage)
    {
        { Type::SizeRequirements } -> std::convertible_to<Size>;
        { Type::AlignmentRequirements } -> std::convertible_to<Size>;

        Type::Destroy(storage);

        //Type::Create(storage, args...)
        //Type::Acquire<Qualifiers>(storage) -> ValueType
    };

    template<AccessorConcept Accessor>
    inline constexpr Size SizeRequirements = Accessor::SizeRequirements;
    template<AccessorConcept Accessor>
    inline constexpr Size AlignmentRequirements = Accessor::AlignmentRequirements;

    template<AccessorConcept Accessor, typename Qualifiers>
    struct AccessorValueType
    {
        using type = decltype(Accessor::template Acquire<Qualifiers>(std::declval<std::byte*>()));
    };

    template<AccessorConcept Accessor, typename ...Args>
    struct IndirectlyConstructibleFrom
    {
        inline constexpr static auto value = requires(std::byte * storage, Args&&... args)
        {
            Accessor::Create(storage, std::forward<Args>(args)...);
        };
    };


    namespace accessors
    {
        template<typename ValueType>
        struct Value
        {
            inline constexpr static myakish::Size SizeRequirements = sizeof(ValueType);
            inline constexpr static myakish::Size AlignmentRequirements = alignof(ValueType);

            static ValueType& AcquireRaw(std::byte* storage)
            {
                return *std::launder(reinterpret_cast<ValueType*>(storage));
            }


            template<typename Qualifiers>
            static decltype(auto) Acquire(std::byte* storage)
            {
                return std::forward_like<Qualifiers>(AcquireRaw(storage));
            }

            template<typename ...Args>
            static void Create(std::byte* storage, Args&&... args)
            {
                new(storage) ValueType(std::forward<Args>(args)...);
            }

            static void Destroy(std::byte* storage)
            {
                AcquireRaw(storage).~ValueType();
            }


            template<Size Index>
            struct DeduceConvertion
            {
                static meta::ValueType<Index> Deduce(ValueType)
                {
                    return {};
                }
            };
        };

        template<typename ReferenceType>
        struct Reference
        {
            using PointerType = std::add_pointer_t<std::remove_reference_t<ReferenceType>>;

            inline constexpr static myakish::Size SizeRequirements = sizeof(PointerType);
            inline constexpr static myakish::Size AlignmentRequirements = alignof(PointerType);

            template<typename Qualifiers>
            static ReferenceType Acquire(std::byte* storage)
            {
                auto pointer = *std::launder(reinterpret_cast<PointerType*>(storage));
                return static_cast<ReferenceType>(*pointer);
            }

            static void Create(std::byte* storage, ReferenceType ref)
            {
                new(storage) PointerType(&ref);
            }

            static void Destroy(std::byte* storage)
            {
                //noop
            }

            template<Size Index>
            struct DeduceConvertion
            {
                static meta::ValueType<Index> Deduce(ReferenceType)
                {
                    return {};
                }
            };


        };
    }

    struct CantDeduceConvertionType {};
    inline constexpr CantDeduceConvertionType CantDeduceConvertion;

    namespace detail
    {
        template<typename ...Bases>
        struct DeduceOverloads : Bases...
        {
            using Bases::Deduce...;
        };

        template<typename Base, typename Index>
        struct DeducerMetafunction
        {
            using type = Base::template DeduceConvertion<Index::value>;
        };

        template<typename Overloads, typename... Args>
        concept ConvertionDeducible = requires(Args&&... args)
        {
            Overloads::Deduce(std::forward<Args>(args)...);
        };

        template<AccessorConcept ...Bases>
        struct DeduceConvertionImpl
        {
            using Indices = meta::IntegerSequence<myakish::Size, sizeof...(Bases)>::type;
            using BaseList = meta::TypeList<Bases...>;

            using Zipped = meta::Zip<BaseList, Indices>::type;

            using DeducerFunc = meta::LeftCurry<meta::QuotedInvoke, meta::Quote<DeducerMetafunction>>;

            using Deducers = meta::QuotedApply<DeducerFunc, Zipped>::type;

            using Overloads = meta::QuotedInvoke<meta::Instantiate<DeduceOverloads>, Deducers>::type;


            template<typename ...Args>
            struct Result
            {
                inline constexpr static auto value = CantDeduceConvertion;
            };

            template<typename ...Args> requires ConvertionDeducible<Overloads, Args&&...>
            struct Result<Args...>
            {
                inline constexpr static auto value = decltype(Overloads::Deduce(std::declval<Args>()...))::value;
            };
        };

    }

    template<typename AccessorsList, typename ArgsList>
    struct DeduceConvertion
    {
        using Impl = meta::QuotedInvoke<meta::Instantiate<detail::DeduceConvertionImpl>, AccessorsList>::type;

        using Result = meta::QuotedInvoke<meta::Instantiate<Impl::template Result>, ArgsList>::type;

        inline constexpr static auto value = Result::value;
    };

    template<typename AccessorsList, typename ArgsList>
    concept ConvertionDeducible = !std::same_as<std::remove_cvref_t<decltype(DeduceConvertion<AccessorsList, ArgsList>::value)>, CantDeduceConvertionType>;
}