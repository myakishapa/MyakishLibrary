#pragma once
#include <memory>
#include <filesystem>
#include <fstream>
#include <vector>
#include <tuple>
#include <generator>
#include <ranges>
#include <concepts>
#include <cmath>
#include <bit>

#include <MyakishLibrary/Meta.hpp>
#include <MyakishLibrary/Core.hpp>
#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

namespace myakish
{
    struct FactorialFunctor : functional::ExtensionMethod
    {
        template<std::integral Number>
        constexpr Number operator()(Number n) const
        {
            Number result = 1;
            for (Number i = 2; i <= n; i++) result *= n;
            return result;
        }
    };
    inline constexpr FactorialFunctor Factorial;


    struct ChooseFunctor : functional::ExtensionMethod
    {
        template<std::integral Number>
        constexpr Number operator()(Number n, Number k) const
        {
            return Factorial(n) / (Factorial(k) * Factorial(n - k));
        }
    };
    inline constexpr ChooseFunctor Choose;

    struct Log2CeilFunctor : functional::ExtensionMethod
    {
        template<std::unsigned_integral Number>
        constexpr Number operator()(Number n) const
        {
            return std::countr_zero(std::bit_ceil(n));
        }
    };
    inline constexpr Log2CeilFunctor Log2Ceil;


    struct AsBytePtrFunctor : functional::ExtensionMethod
    {
        template<typename Type>
        auto operator()(Type* ptr) const
        {
            if constexpr (std::is_const_v<Type>) return reinterpret_cast<const std::byte*>(ptr);
            else return reinterpret_cast<std::byte*>(ptr);
        }
    };
    inline constexpr AsBytePtrFunctor AsBytePtr;


    struct ReadFileFunctor : functional::ExtensionMethod
    {
        auto operator()(const std::filesystem::path& path) const
        {
            std::vector<std::byte> result;
            result.resize(std::filesystem::file_size(path));

            std::ifstream in(path, std::ios::binary);
            in.read(reinterpret_cast<char*>(result.data()), result.size());

            return result;
        }
    };
    inline constexpr ReadFileFunctor ReadFile;


    struct ReadTextFileFunctor : functional::ExtensionMethod
    {
        auto operator()(const std::filesystem::path& path) const
        {
            std::vector<char> result;
            result.resize(std::filesystem::file_size(path));

            std::ifstream in(path, std::ios::binary);
            in.read(result.data(), result.size());

            return result;
        }
    };
    inline constexpr ReadTextFileFunctor ReadTextFile;

    struct FormatByteSizeFunctor : functional::ExtensionMethod
    {
        auto operator()(Size size) const
        {
            using namespace literals;

            if (size < 1024_ssize) return std::format("{} bytes", size);
            size /= 1024_ssize;

            if (size < 1024_ssize) return std::format("{} KiB", size);
            size /= 1024_ssize;

            if (size < 1024_ssize) return std::format("{} MiB", size);
            size /= 1024_ssize;

            if (size < 1024_ssize) return std::format("{} GiB", size);
            size /= 1024_ssize;

            return std::format("{} TiB", size);
        }
    };
    inline constexpr FormatByteSizeFunctor FormatByteSize;


    struct CollatzWeylPRNGFunctor : functional::ExtensionMethod
    {
        template<std::integral Number>
        std::generator<bool> operator()(Number x, Number weyl, Number s) const
        {
            while (true)
            {
                if (x % 2 == 1) x = (3 * x + 1) / 2;
                else x = x / 2;
                x ^= (weyl += s);

                co_yield x & 1;
            }
        }
    };
    inline constexpr CollatzWeylPRNGFunctor CollatzWeylPRNG;


    template<typename To>
    struct BitCastFunctor : functional::ExtensionMethod
    {
        template<typename ...Args>
        constexpr To operator()(Args... args) const
        {
            std::common_type_t<Args...> array[] = { args... };
            return std::bit_cast<To>(array);
        }
    };
    template<typename To>
    inline constexpr BitCastFunctor<To> BitCast;


    struct HashFunctor : functional::ExtensionMethod
    {
        constexpr std::uint64_t operator()(std::string_view str) const
        {
            constexpr std::uint64_t multiply = 0xc6a4a7935bd1e995ULL;
            constexpr std::uint64_t shift = 47ULL;
            constexpr std::uint64_t seed = 700924169573080812ULL;

            const std::size_t len = str.size();
            const std::size_t calclen = len ? len + 1 : 0;
            std::uint64_t hash = seed ^ (calclen * multiply);

            if (len > 0)
            {
                const auto* data = str.data();
                const auto first_loop_iterations = calclen / 8;

                for (size_t i = 0; i < first_loop_iterations; ++i)
                {
                    std::uint64_t k = BitCast<std::uint64_t>(
                        *data, *(data + 1), *(data + 2), *(data + 3),
                        *(data + 4), *(data + 5), *(data + 6), *(data + 7));

                    k *= multiply;
                    k ^= k >> shift;
                    k *= multiply;

                    hash ^= k;
                    hash *= multiply;
                    data += 8;
                }

                const auto* data2 = str.data() + 8 * first_loop_iterations;

                switch (calclen & 7)
                {
                case 7: hash ^= static_cast<uint64_t>(data2[6]) << 48ULL; [[fallthrough]];
                case 6: hash ^= static_cast<uint64_t>(data2[5]) << 40ULL; [[fallthrough]];
                case 5: hash ^= static_cast<uint64_t>(data2[4]) << 32ULL; [[fallthrough]];
                case 4: hash ^= static_cast<uint64_t>(data2[3]) << 24ULL; [[fallthrough]];
                case 3: hash ^= static_cast<uint64_t>(data2[2]) << 16ULL; [[fallthrough]];
                case 2: hash ^= static_cast<uint64_t>(data2[1]) << 8ULL; [[fallthrough]];
                case 1: hash ^= static_cast<uint64_t>(data2[0]);
                    hash *= multiply;
                };
            }

            hash ^= hash >> shift;
            hash *= multiply;
            hash ^= hash >> shift;

            return hash;
        }
    };
    inline constexpr HashFunctor Hash;


    struct HashCombineFunctor : functional::ExtensionMethod
    {
        constexpr std::uint64_t operator()(std::uint64_t f, std::uint64_t s) const
        {
            return f ^ s;
        }
    };
    inline constexpr HashCombineFunctor HashCombine;


    
    template<typename Type>
    struct FromCharsFunctor : functional::ExtensionMethod
    {
        template<typename ...Args>
        constexpr Type operator()(std::string_view view) const
        {
            Type result;
            std::from_chars(view.data(), view.data() + view.size(), result);
            return result;
        }
    };
    template<typename Type>
    inline constexpr FromCharsFunctor<Type> FromChars;


    struct DirectoryRange : std::ranges::view_interface<DirectoryRange>
    {
        std::filesystem::path directory;

        DirectoryRange(std::filesystem::path directory) : directory(directory) {}

        auto begin() const
        {
            return std::filesystem::directory_iterator(directory);
        }
        auto end() const
        {
            return std::filesystem::directory_iterator();
        }
    };
    struct RecursiveDirectoryRange : std::ranges::view_interface<RecursiveDirectoryRange>
    {
        std::filesystem::path directory;

        RecursiveDirectoryRange(std::filesystem::path directory) : directory(directory) {}

        auto begin() const
        {
            return std::filesystem::recursive_directory_iterator(directory);
        }
        auto end() const
        {
            return std::filesystem::recursive_directory_iterator();
        }
    };


    struct UnsignFunctor : functional::ExtensionMethod
    {
        template<std::integral Type>
        constexpr auto operator()(Type num) const
        {
            return static_cast<std::make_unsigned_t<Type>>(num);
        }
    };
    inline constexpr UnsignFunctor Unsign;

    struct PaddingFunctor : functional::ExtensionMethod
    {
        template<std::integral Type>
        constexpr Type operator()(Type value, Type alignment) const
        {
            return (alignment - (value % alignment)) % alignment;
        }
    };
    inline constexpr PaddingFunctor Padding;
    
    struct AsSizeFunctor : functional::ExtensionMethod
    {
        template<std::convertible_to<Size> Type>
        constexpr auto operator()(Type&& value) const
        {
            return static_cast<Size>(std::forward<Type>(value));
        }
    };
    inline constexpr AsSizeFunctor AsSize;   

    struct AsBytesFunctor : functional::ExtensionMethod
    {
        template<std::ranges::contiguous_range Range,
            typename SpanType = std::span<std::conditional_t<std::ranges::constant_range<Range>, const std::byte, std::byte>>> 
            requires meta::TriviallyCopyableConcept<std::ranges::range_value_t<Range>>
        constexpr SpanType operator()(Range&& range) const
        {
            return SpanType(std::ranges::data(range) | AsBytePtr, std::ranges::size(range) * sizeof(std::ranges::range_value_t<Range>));
        }
    };
    inline constexpr AsBytesFunctor AsBytes;

}