#pragma once
#include <ranges>
#include <cstdint>
#include <bit>
#include <cmath>

#include <MyakishLibrary/Math/Common.hpp>
#include <MyakishLibrary/Core.hpp>
#include <MyakishLibrary/Functional/Pipeline.hpp>

namespace myakish::ranges
{
    struct BitDescriptor
    {
        using Underlying = std::uintptr_t;
        using ByteType = std::byte;
        using BitType = std::uint_fast8_t;

        constexpr static Size BitWidth = CHAR_BIT * sizeof(Underlying);

        constexpr static Size IndexBits = math::Log2Ceil((unsigned)CHAR_BIT);
        constexpr static Size AddressBits = BitWidth - IndexBits;

        constexpr static Underlying IndexMask = ~Underlying{ 0 } >> AddressBits;
        constexpr static Underlying AddressMask = ~Underlying{ 0 } >> IndexBits;

    private:

        Underlying value;

    public:

        BitDescriptor() = default;
        BitDescriptor(const ByteType* byte, BitType bit)
        {
            Reference(byte, bit);
        }
        BitDescriptor(const ByteType* byte) : BitDescriptor(byte, 0)
        {

        }
        explicit BitDescriptor(Size index)
        {
            AbsoluteIndex(index);
        }

        ByteType* Byte() const
        {
            void* ptr = std::bit_cast<void*>((value >> IndexBits) & AddressMask);
            return reinterpret_cast<ByteType*>(ptr);
        }

        BitType Bit() const
        {
            return value & IndexMask;
        }

        void Reference(const ByteType* byte, BitType bit)
        {
            auto ptr = std::bit_cast<Underlying>(byte);
            if (std::bit_width(ptr) > AddressBits) std::terminate();

            value = ((ptr & AddressMask) << IndexBits) | (bit & IndexMask);
        }

        void Byte(const ByteType* byte)
        {
            Reference(byte, Bit());
        }

        void Bit(BitType bit)
        {
            Reference(Byte(), bit);
        }

        bool Value() const
        {
            return static_cast<bool>((*Byte() >> Bit()) & ByteType{ 1 });
        }

        void Value(bool value) const
        {
            auto oldByte = *Byte();

            if (value) oldByte |= ByteType{ 1 } << Bit();
            else oldByte &= ~(ByteType{ 1 } << Bit());

            *Byte() = oldByte;
        }

        operator bool() const
        {
            return Value();
        }

        void operator=(bool value) const
        {
            Value(value);
        }

        Size AbsoluteIndex() const
        {
            return static_cast<Size>(value);
        }

        void AbsoluteIndex(Size index)
        {
            value = index;
        }

        BitDescriptor operator+(Size rhs) const
        {
            return BitDescriptor(value + rhs);
        }
        BitDescriptor operator-(Size rhs) const
        {
            return BitDescriptor(value - rhs);
        }

        friend BitDescriptor operator+(Size lhs, BitDescriptor rhs)
        {
            return rhs + lhs;
        }

        BitDescriptor& operator+=(Size rhs)
        {
            value += rhs;
            return *this;
        }
        BitDescriptor& operator-=(Size rhs)
        {
            value -= rhs;
            return *this;
        }

        Size operator-(BitDescriptor rhs) const
        {
            return AbsoluteIndex() - rhs.AbsoluteIndex();
        }

        friend std::strong_ordering operator<=>(BitDescriptor, BitDescriptor) = default;
    };

    struct ContiguousBitIterator
    {
        BitDescriptor descriptor;

        using difference_type = Size;
        using value_type = BitDescriptor;
        using iterator_concept = std::random_access_iterator_tag;

        BitDescriptor operator*() const
        {
            return descriptor;
        }

        ContiguousBitIterator& operator++()
        {
            descriptor += 1;
            return *this;
        }
        ContiguousBitIterator operator++(int)
        {
            auto copy = *this;
            ++*this;
            return copy;
        }

        ContiguousBitIterator& operator--()
        {
            descriptor -= 1;
            return *this;
        }
        ContiguousBitIterator operator--(int)
        {
            auto copy = *this;
            --*this;
            return copy;
        }

        ContiguousBitIterator operator+(Size rhs) const
        {
            return ContiguousBitIterator(descriptor + rhs);
        }
        ContiguousBitIterator operator-(Size rhs) const
        {
            return ContiguousBitIterator(descriptor - rhs);
        }

        friend ContiguousBitIterator operator+(Size lhs, ContiguousBitIterator rhs)
        {
            return rhs + lhs;
        }

        ContiguousBitIterator& operator+=(Size rhs)
        {
            descriptor += rhs;
            return *this;
        }
        ContiguousBitIterator& operator-=(Size rhs)
        {
            descriptor -= rhs;
            return *this;
        }

        Size operator-(ContiguousBitIterator rhs) const
        {
            return descriptor - rhs.descriptor;
        }

        BitDescriptor operator[](Size index) const
        {
            return descriptor + index;
        }

        friend std::strong_ordering operator<=>(ContiguousBitIterator, ContiguousBitIterator) = default;
    };
    static_assert(std::random_access_iterator<ContiguousBitIterator>, "ContiguousBitIterator should be std::random_access_iterator");
    static_assert(std::output_iterator<ContiguousBitIterator, bool>, "ContiguousBitIterator should be std::output_iterator<bool>");

    struct ContiguousBitRange
    {
        const std::byte* start;
        Size size;

        auto begin() const
        {
            return ContiguousBitIterator(start);
        }
        auto end() const
        {
            return ContiguousBitIterator(start + size);
        }
    };
    static_assert(std::ranges::random_access_range<ContiguousBitRange>, "ContiguousBitRange should be std::ranges::random_access_range");

    struct BitsFunctor : std::ranges::range_adaptor_closure<BitsFunctor>
    {
        ContiguousBitRange operator()(std::ranges::contiguous_range auto&& range) const
        {
            auto data = std::ranges::data(range);
            auto size = std::ranges::size(range) * sizeof(std::ranges::range_value_t<decltype(range)>);
            return ContiguousBitRange(AsBytePtr(data), size);
        }
    };
    constexpr inline BitsFunctor Bits;

    struct ValuesFunctor : std::ranges::range_adaptor_closure<ValuesFunctor>
    {
        auto operator()(std::ranges::range auto&& range) const
        {
            return range | std::views::transform([](auto desc) -> bool { return desc.Value(); });
        }
    };
    constexpr inline ValuesFunctor Values;

}

