#pragma once

#include <concepts>
#include <fstream>
#include <filesystem>

#include <MyakishLibrary/Meta/Concepts.hpp>
#include <MyakishLibrary/Core.hpp>

namespace myakish::streams2
{
    template<typename Type>
    concept Stream = requires(Type stream, const Type cstream, Size size)
    {
        stream.Seek(size); // size > 0 only
        { cstream.Valid() } -> std::convertible_to<bool>;
    };

    template<typename Type>
    concept AlignableStream = Stream<Type> && requires(Type stream, const Type cstream, Size size)
    {
        { cstream.Offset() } -> std::convertible_to<Size>;
    };

    template<typename Type>
    concept SizedStream = Stream<Type> && requires(const Type cstream)
    {
        { cstream.Length() } -> std::convertible_to<Size>;
    };

    template<typename Type>
    concept InputStream = Stream<Type> && requires(Type in, Size size, std::byte* dst)
    {
        in.Read(dst, size);
    };

    template<typename Type>
    concept OutputStream = Stream<Type> && requires(Type out, const std::byte* data, Size size)
    {
        out.Write(data, size);
    };

    template<typename Type>
    concept ReservableStream = OutputStream<Type> && requires(Type out, Size reserve)
    {
        out.Reserve(reserve);
    };
}
