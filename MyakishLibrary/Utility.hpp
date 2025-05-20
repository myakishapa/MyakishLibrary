#pragma once
#include <memory>
#include <filesystem>
#include <fstream>
#include <vector>
#include <tuple>
#include <generator>

#include <MyakishLibrary/Meta/Functions.hpp>
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
}