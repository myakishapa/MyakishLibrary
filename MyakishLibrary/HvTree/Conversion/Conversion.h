#pragma once

#include <ranges>
#include <cstdint>
#include <string>
#include <string_view>

#include <HvTree2/HvTree.h>

#include <MyakishLibrary/Meta/Concepts.hpp>

namespace hv::conversion
{
    template<typename Type>
    struct EnableTrivialConversion : std::true_type {};

    template<typename Type>
    concept TriviallyConvertible = EnableTrivialConversion<Type>::value && !std::ranges::range<Type>;

    template<TriviallyConvertible Type>
    struct HvToType<Type>
    {
        static inline constexpr bool UseBinary = true;

        template<myakish::streams::BinaryInput BinaryStream>
        static Type Convert(BinaryStream&& in)
        {
            return myakish::streams::Read<Type>(in);
        }
    };

    template<TriviallyConvertible Type>
    struct TypeToHv<Type>
    {
        static inline constexpr bool UseBinary = true;

        template<myakish::streams::BinaryOutput BinaryStream>
        static void Convert(BinaryStream&& out, Type value)
        {
            out.Reserve(sizeof(Type));
            auto outIt = out.Begin();
            myakish::streams::Write(outIt, value);
        }
    };

    struct Bounds
    {
        std::size_t begin;
        std::size_t count;
    };

    template<std::ranges::sized_range Range> requires myakish::meta::TriviallyCopyable<std::ranges::range_value_t<Range>>&& std::constructible_from<Range, std::size_t>
    struct HvToType<Range>
    {
        static inline constexpr bool UseBinary = true;

        using Type = std::ranges::range_value_t<Range>;

        template<myakish::streams::SizedBinaryInput BinaryStream>
        static Range Convert(BinaryStream&& in)
        {
            auto bytes = in.Size();
            auto count = bytes / sizeof(Type);
            Range result(count);

            auto inIt = in.Begin();
            auto outIt = result.begin();

            for (auto i = 0; i < count; i++)
            {
                auto result = myakish::streams::Read<Type>(inIt);
                *outIt++ = result;
            }

            return result;
        }

        template<myakish::streams::RandomAccessBinaryInput BinaryStream>
        static Range Convert(BinaryStream&& in, Bounds bounds)
        {
            Range result(bounds.count);

            auto inIt = in.Begin();
            inIt.Seek(bounds.begin * sizeof(Type));

            auto outIt = result.begin();

            for (auto i = 0; i < bounds.count; i++)
            {
                auto result = myakish::streams::Read<Type>(inIt);
                *outIt++ = result;
            }

            return result;
        }
    };

    template<std::ranges::sized_range Range> requires myakish::meta::TriviallyCopyable<std::ranges::range_value_t<Range>>
    struct TypeToHv<Range>
    {
        static inline constexpr bool UseBinary = true;

        using Type = std::ranges::range_value_t<Range>;

        template<myakish::streams::BinaryOutput BinaryStream, myakish::meta::ForwardingReferenceOf<Range> RangeRef>
        static void Convert(BinaryStream&& out, RangeRef&& in)
        {
            auto count = std::ranges::size(std::forward<RangeRef>(in));
            out.Reserve(sizeof(Type) * count);

            auto outIt = out.Begin();
            for (auto value : in)
            {
                myakish::streams::Write(outIt, value);
            }
        }

        template<myakish::streams::BinaryOutput BinaryStream, myakish::meta::ForwardingReferenceOf<Range> RangeRef>
        static void Convert(BinaryStream&& out, RangeRef&& in, Bounds bounds)
        {
            out.Reserve(sizeof(Type) * bounds.count);

            auto outIt = out.Begin();
            for (auto value : in | std::views::drop(bounds.begin) | std::views::take(bounds.count))
            {
                myakish::streams::Write(outIt, value);
            }
        }
    };

    template<>
    struct TypeToHv<std::string_view>
    {
        static inline constexpr bool UseBinary = true;

        template<myakish::streams::BinaryOutput BinaryStream>
        static void Convert(BinaryStream&& out, std::string_view value)
        {
            out.Reserve(value.size());
            auto outIt = out.Begin();
            outIt.Write(reinterpret_cast<const std::byte*>(value.data()), value.size());
        }
    };

    template<>
    struct HvToType<std::string>
    {
        static inline constexpr bool UseBinary = true;

        template<myakish::streams::SizedBinaryInput BinaryStream>
        static std::string Convert(BinaryStream&& in)
        {
            auto inIt = in.Begin();
            auto size = in.Length();
            std::string result(size, '\0');
            inIt.Read(reinterpret_cast<std::byte*>(result.data()), size);
            return result;
        }
    };
    template<>
    struct TypeToHv<std::string> : public hv::conversion::TypeToHv<std::string_view>
    {

    };
}