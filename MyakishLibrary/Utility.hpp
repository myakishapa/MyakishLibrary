#pragma once
#include <memory>
#include <filesystem>
#include <fstream>
#include <vector>
#include <tuple>
#include <generator>
#include <filesystem>
#include <ranges>

#include <MyakishLibrary/Meta.hpp>
#include <MyakishLibrary/Core.hpp>

namespace myakish
{
    template<typename Type>
    auto AsBytePtr(Type* ptr)
    {
        if constexpr (std::is_const_v<Type>) return reinterpret_cast<const std::byte*>(ptr);
        else return reinterpret_cast<std::byte*>(ptr);

        //return reinterpret_cast<meta::CopyConst<std::byte, Type>*>(ptr);
    }

    auto ReadFile(std::filesystem::path path)
    {
        std::vector<std::byte> result;
        result.resize(std::filesystem::file_size(path));

        std::ifstream in(path, std::ios::binary);
        in.read(reinterpret_cast<char*>(result.data()), result.size());

        return result;
    }

    auto ReadTextFile(std::filesystem::path path)
    {
        std::vector<char> result;
        result.resize(std::filesystem::file_size(path));

        std::ifstream in(path, std::ios::binary);
        in.read(result.data(), result.size());

        return result;
    }


    auto FormatByteSize(Size size)
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

    namespace detail
    {
        template<typename Tuple, std::size_t... IndicesFirst, std::size_t... IndicesSecond>
        auto SliceTuple(Tuple&& tuple, std::index_sequence<IndicesFirst...>, std::index_sequence<IndicesSecond...>)
        {
            using std::get;
            return std::tuple(std::tuple(get<IndicesFirst>(std::forward<Tuple>(tuple))...), std::tuple(get<IndicesSecond + sizeof...(IndicesFirst)>(std::forward<Tuple>(tuple))...));
        }
    }

    template<std::size_t First, typename Tuple>
    auto SliceTuple(Tuple&& tuple)
    {
        return detail::SliceTuple(std::forward<Tuple>(tuple), std::make_index_sequence<First>{}, std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<Tuple>> - First>{});
    }

    template<std::integral Number>
    std::generator<bool> CollatzWeylPRNG(Number x, Number weyl, Number s)
    {
        while (true)
        {
            if (x % 2 == 1) x = (3 * x + 1) / 2;
            else x = x / 2;
            x ^= (weyl += s);

            co_yield x & 1;
        }
    }


    template<typename Type, typename ...Args>
    constexpr Type BitCast(Args... args)
    {
        std::common_type_t<Args...> array[] = { args... };
        return std::bit_cast<Type>(array);
    }

    constexpr std::uint64_t Hash(std::string_view str)
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

    constexpr std::uint64_t HashCombine(std::uint64_t f, std::uint64_t s)
    {
        return f ^ s;
    }


    template<typename Type>
    Type FromChars(std::string_view view)
    {
        Type result;
        std::from_chars(view.data(), view.data() + view.size(), result);
        return result;
    }


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


    template<typename Only>
    constexpr Only&& RightFold(auto&&, Only&& only)
    {
        return std::forward<Only>(only);
    }
    
    template<typename First, typename... Rest, typename Invocable>
    constexpr decltype(auto) RightFold(Invocable&& invocable, First&& first, Rest&&... rest)
    {
        return std::invoke(std::forward<Invocable>(invocable), first, RightFold(std::forward<Invocable>(invocable), std::forward<Rest>(rest)...));
    }

    template<std::integral Type>
    constexpr auto Unsign(Type num)
    {
        return static_cast<std::make_unsigned_t<Type>>(num);
    }

    template<std::integral Type>
    constexpr Type Padding(Type value, Type alignment)
    {
        return (alignment - (value % alignment)) % alignment;
    }
}