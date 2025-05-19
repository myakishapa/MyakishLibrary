#pragma once

#include <concepts>
#include <fstream>
#include <filesystem>

#include <MyakishLibrary/Meta/Concepts.hpp>
#include <MyakishLibrary/Core.hpp>


namespace myakish::streams
{
    namespace literals
    {
        constexpr Size operator ""_ssize(unsigned long long i)
        {
            return i;
        }
    }

    template<typename Iterator>
    concept BinaryIterator = requires(Iterator it, Size size)
    {
        it.Seek(size); // size > 0 only
    };

    template<typename Iterator>
    concept RandomAccessBinaryIterator = BinaryIterator<Iterator> && requires(Iterator it, Size seek)
    {
        it.Seek(seek);
        { it.Offset() } -> std::convertible_to<Size>;
    };



    template<typename Iterator>
    concept BinaryInputIterator = BinaryIterator<Iterator> && requires(Iterator it, Size size, std::byte* dst)
    {
        it.Read(dst, size);
        { it.Valid() } -> std::convertible_to<bool>;
    };

    template<typename Iterator>
    concept RandomAccessBinaryInputIterator = BinaryInputIterator<Iterator> && RandomAccessBinaryIterator<Iterator>;


    template<typename Stream>
    concept BinaryInput = requires(Stream in)
    {
        { in.Begin() } -> BinaryInputIterator;
    };

    template<typename Stream>
    concept SizedBinaryInput = BinaryInput<Stream> && requires(Stream in)
    {
        { in.Length() } -> std::same_as<Size>;
    };

    template<typename Stream>
    concept RandomAccessBinaryInput = SizedBinaryInput<Stream> && requires(Stream in)
    {
        { in.Begin() } -> RandomAccessBinaryInputIterator;
    };



    template<typename Iterator>
    concept BinaryOutputIterator = BinaryIterator<Iterator> && requires(Iterator out, const std::byte* data, Size size)
    {
        out.Write(data, size);
    };

    template<typename Iterator>
    concept RandomAccessBinaryOutputIterator = BinaryOutputIterator<Iterator> && RandomAccessBinaryIterator<Iterator>;


    template<typename Stream>
    concept BinaryOutput = requires(Stream out, Size reserve)
    {
        { out.Begin() } -> BinaryOutputIterator;
        out.Reserve(reserve);
    };

    template<typename Stream>
    concept RandomAccessBinaryOutput = BinaryOutput<Stream> && requires(Stream out)
    {
        { out.Begin() } -> RandomAccessBinaryOutputIterator;
    };
}


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
