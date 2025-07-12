#pragma once

#include <concepts>

#include <MyakishLibrary/Core.hpp>

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
        };
    }

}