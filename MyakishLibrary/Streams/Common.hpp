#pragma once

#include <filesystem>
#include <fstream>
#include <ranges>

#include <MyakishLibrary/Meta/Concepts.hpp>
#include <MyakishLibrary/Meta/Functions.hpp>
#include <MyakishLibrary/Streams/Concepts.hpp>

#include <MyakishLibrary/Utility.hpp>

#include <MyakishLibrary/Functional/Pipeline.hpp>
#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

namespace fs = std::filesystem;

namespace myakish::streams
{
    struct ExpositionOnlyStream
    {
        void Seek(Size size); // Stream

        bool Valid() const; // Stream

        Size Length() const; // SizedStream

        Size Offset() const; // AlignableStream

        void Read(std::byte* dst, Size size); // InputStream

        void Write(const std::byte* src, Size size); // OutputStream

        void Reserve(Size reserve); // ReservableStream
    };
    static_assert(AlignableStream<ExpositionOnlyStream>, "ExpositionOnlyStream must be AlignableStream");
    static_assert(SizedStream<ExpositionOnlyStream>, "ExpositionOnlyStream must be RandomAccessStream");
    static_assert(InputStream<ExpositionOnlyStream>, "ExpositionOnlyStream must be InputStream");
    static_assert(ReservableStream<ExpositionOnlyStream>, "ExpositionOnlyStream must be ReservableStream");

    template<bool Const>
    struct ContiguousStream
    {
        using DataType = meta::ConstIfT<std::byte, Const>;

        DataType* data;
        const std::byte* const sentinel;

        ContiguousStream() = default;

        ContiguousStream(DataType* data, const std::byte* const sentinel) : data(data), sentinel(sentinel) {}
        ContiguousStream(DataType* data, Size size) : data(data), sentinel(data + size) {}

        void Write(const std::byte* source, Size size) requires !Const
        {
            std::memcpy(std::exchange(data, data + size), source, size);
        }

        void Seek(Size seek)
        {
            data += seek;
        }

        void Read(std::byte* dst, Size size)
        {
            std::memcpy(dst, std::exchange(data, data + size), size);
        }

        bool Valid() const
        {
            return data < sentinel;
        }

        operator ContiguousStream<false>() const requires !Const
        {
            return { data, sentinel };
        }

        Size Length() const
        {
            return sentinel - data;
        }

        DataType* Data() const
        {
            return data;
        }
    };
    static_assert(OutputStream<ContiguousStream<false>>, "non-const ContiguousStream must be OutputStream");
    static_assert(InputStream<ContiguousStream<true>>, "const ContiguousStream must be InputStream");
    static_assert(SizedStream<ContiguousStream<true>>, "const ContiguousStream must be SizedStream");
    static_assert(PersistentDataStream<ContiguousStream<true>>, "const ContiguousStream must be PersistentDataStream");
    static_assert(!OutputStream<ContiguousStream<true>>, "const ContiguousStream must not be OutputStream");

    using ConstContiguosStream = ContiguousStream<true>;

    ContiguousStream(std::byte*, const std::byte*) -> ContiguousStream<false>;
    ContiguousStream(const std::byte*, const std::byte*) -> ContiguousStream<true>;

    template<Stream Underlying>
    struct AlignableWrapper
    {
        Underlying stream;
        Size offset;

        AlignableWrapper(Underlying stream) : stream(std::move(stream)), offset{} {}

        void Seek(Size size)
        {
            offset += size;
            stream.Seek(size);
        }

        bool Valid() const
        {
            return stream.Valid();
        }

        Size Length() const requires SizedStream<Underlying>
        {
            return stream.Length();
        }

        Size Offset() const
        {
            return offset;
        }

        void Read(std::byte* dst, Size size) requires InputStream<Underlying>
        {
            offset += size;
            stream.Read(dst, size);
        }

        void Write(const std::byte* src, Size size) requires OutputStream<Underlying>
        {
            offset += size;
            stream.Write(src, size);
        }

        void Reserve(Size reserve) requires ReservableStream<Underlying>
        {
            stream.Reserve(reserve);
        }
    };
    static_assert(AlignableStream<AlignableWrapper<ExpositionOnlyStream>>, "AlignableWrapper must be AlignableStream");
    static_assert(SizedStream<AlignableWrapper<ExpositionOnlyStream>>, "AlignableWrapper must be RandomAccessStream");
    static_assert(InputStream<AlignableWrapper<ExpositionOnlyStream>>, "AlignableWrapper must be InputStream");
    static_assert(ReservableStream<AlignableWrapper<ExpositionOnlyStream>>, "AlignableWrapper must be ReservableStream");

    struct Aligner : functional::ExtensionMethod
    {
        auto ExtensionInvoke(Stream auto stream) const
        {
            return AlignableWrapper(std::move(stream));
        }

        auto ExtensionInvoke(AlignableStream auto alignable) const
        {
            return std::move(alignable);
        }
    };
    inline constexpr Aligner Aligned;

    
    struct StandardOutputStream
    {
        std::ostream& out;

        StandardOutputStream(std::ostream& out) : out(out)
        {

        }

        void Seek(Size seek)
        {
            out.seekp(seek, std::ios::cur);
        }

        void Write(const std::byte* source, Size size)
        {
            out.write(reinterpret_cast<const char*>(source), size);
        }

        bool Valid() const
        {
            return out.good();
        }
    };
    static_assert(OutputStream<StandardOutputStream>, "StandardOutputStream must be OutputStream");
    struct StandardInputStream
    {
        std::istream& in;

        StandardInputStream(std::istream& in) : in(in)
        {

        }


        void Read(std::byte* destination, Size size)
        {
            in.read(reinterpret_cast<char*>(destination), size);
        }

        void Seek(Size seek)
        {
            in.seekg(seek, std::ios::cur);
        }

        bool Valid() const
        {
            return in.good();
        }
    };
    static_assert(InputStream<StandardInputStream>, "StandardInputStream must be InputStream");

    struct FileOutputStream : StandardOutputStream
    {
        fs::path path;
        std::ofstream out;

        FileOutputStream(fs::path path) : path(path), out(path, std::ios::binary), StandardOutputStream(out)
        {

        }

        FileOutputStream(FileOutputStream&& rhs) noexcept : path(std::move(rhs.path)), out(std::move(rhs.out)), StandardOutputStream(out) {}
        FileOutputStream(const FileOutputStream& rhs) = delete;
    };
    static_assert(OutputStream<FileOutputStream>, "FileInputStream must be OutputStream");

    struct FileInputStream : StandardInputStream
    {
        fs::path path;
        std::ifstream in;

        FileInputStream(fs::path path) : path(path), in(path, std::ios::binary), StandardInputStream(in)
        {

        }
      
        FileInputStream(FileInputStream&& rhs) noexcept : path(std::move(rhs.path)), in(std::move(rhs.in)), StandardInputStream(in) {}
        FileInputStream(const FileInputStream& rhs) = delete;
    };
    static_assert(InputStream<FileInputStream>, "FileInputStream must be InputStream");
    
    struct CopyFunctor : functional::ExtensionMethod
    {
        constexpr void ExtensionInvoke(InputStream auto&& in, OutputStream auto&& out, Size bytes) const
        {
            std::vector<std::byte> buffer(bytes);
            in.Read(buffer.data(), bytes);
            out.Write(buffer.data(), bytes);
        }
    };
    inline constexpr CopyFunctor Copy;

    struct WriteFunctor : functional::ExtensionMethod
    {
        constexpr void ExtensionInvoke(OutputStream auto&& out, myakish::meta::TriviallyCopyable auto value) const
        {
            out.Write(reinterpret_cast<const std::byte*>(&value), sizeof(value));
        }

        constexpr void ExtensionInvoke(OutputStream auto&& out, std::string_view str) const
        {
            out.Write(reinterpret_cast<const std::byte*>(str.data()), str.size());
        }
    };
    inline constexpr WriteFunctor Write;

    template<myakish::meta::TriviallyCopyable Type>
    struct WriteAsFunctor : functional::ExtensionMethod
    {
        constexpr void ExtensionInvoke(OutputStream auto&& out, Type value) const
        {
            out.Write(reinterpret_cast<const std::byte*>(&value), sizeof(value));
        }

        constexpr void ExtensionInvoke(OutputStream auto&& out, std::string_view str) const requires std::same_as<Type, std::string_view>
        {
            out.Write(reinterpret_cast<const std::byte*>(str.data()), str.size());
        }
    };
    template<myakish::meta::TriviallyCopyable Type>
    inline constexpr WriteAsFunctor<Type> WriteAs;

    template<myakish::meta::TriviallyCopyable Type>
    struct ReadFunctor : functional::ExtensionMethod
    {
        constexpr Type ExtensionInvoke(InputStream auto&& in) const
        {
            Type result;
            in.Read(reinterpret_cast<std::byte*>(&result), sizeof(result));
            return result;
        }
    };
    template<myakish::meta::TriviallyCopyable Type>
    inline constexpr ReadFunctor<Type> Read;

    struct AlignFunctor : functional::ExtensionMethod
    {
        constexpr void ExtensionInvoke(AlignableStream auto&& stream, Size alignment) const
        {
            stream.Seek((alignment - (stream.Offset() % alignment)) % alignment);
        }
    };
    inline constexpr AlignFunctor Align;

    struct PolymorphicStreamBase
    {
        virtual void Seek(Size size) const = 0; // Stream
        
        virtual bool Valid() const = 0; // Stream

        virtual Size Length() const = 0; // SizedStream

        virtual Size Offset() const = 0; // AlignableStream

        virtual void Read(std::byte* dst, Size size) const = 0; // InputStream

        virtual void Write(const std::byte* src, Size size) const = 0; // OutputStream

        virtual void Reserve(Size reserve) const = 0; // ReservableStream
    };
    static_assert(AlignableStream<PolymorphicStreamBase>, "PolymorphicStreamBase must be AlignableStream");
    static_assert(SizedStream<PolymorphicStreamBase>, "PolymorphicStreamBase must be RandomAccessStream");
    static_assert(InputStream<PolymorphicStreamBase>, "PolymorphicStreamBase must be InputStream");
    static_assert(ReservableStream<PolymorphicStreamBase>, "PolymorphicStreamBase must be ReservableStream");

    template<Stream Underlying>
    struct PolymorphicStream : PolymorphicStreamBase
    {
        Underlying &&stream;

        PolymorphicStream(Underlying &&stream) : stream(std::forward<Underlying>(stream)) {}

        virtual void Seek(Size size) const override
        {
            stream.Seek(size);
        }

        virtual bool Valid() const override
        {
            return stream.Valid();
        }

        virtual Size Length() const override
        {
            if constexpr (SizedStream<Underlying>) return stream.Length();
            else std::terminate();
        }

        virtual Size Offset() const override
        {
            if constexpr (AlignableStream<Underlying>) return stream.Offset();
            else std::terminate();
        }

        virtual void Read(std::byte* dst, Size size) const override
        {
            if constexpr (InputStream<Underlying>) stream.Read(dst, size);
            else std::terminate();
        }

        virtual void Write(const std::byte* src, Size size) const override
        {
            if constexpr (OutputStream<Underlying>) stream.Write(src, size);
            else std::terminate();
        }

        virtual void Reserve(Size reserve) const override
        {
            if constexpr (ReservableStream<Underlying>) stream.Reserve(reserve);
            else std::terminate();
        }
    };

    template<Stream Underlying>
    PolymorphicStream(Underlying&&) -> PolymorphicStream<Underlying&&>;

    struct Polymorphizer : functional::ExtensionMethod
    {
        auto ExtensionInvoke(Stream auto &&stream) const
        {
            return PolymorphicStream(std::forward<decltype(stream)>(stream));
        }
    };
    inline constexpr Polymorphizer Polymorphize;

    template<std::ranges::input_range Range>
    struct RangeInputStream
    {
        using Iterator = std::ranges::iterator_t<Range>;
        using Value = std::ranges::range_value_t<Range>;

        constexpr inline static Size ValueSize = (Size)sizeof(Value);

        Range&& range;
        Iterator iterator;
        Size offset;

        RangeInputStream(Range&& range) : range(std::forward<Range>(range)), iterator(std::ranges::begin(range)), offset{ 0 } {}

        void Read(std::byte* dst, Size size)
        {
            while (size)
            {
                auto currentLeft = ValueSize - offset;
                if (currentLeft == 0)
                {
                    iterator++;
                    offset = 0;
                    currentLeft = ValueSize;
                }

                auto toCopy = std::min(currentLeft, size);

                Value current = *iterator;
                memcpy(std::exchange(dst, dst + toCopy), AsBytePtr(&current) + offset, toCopy);

                offset += toCopy;
                size -= toCopy;
            }
        }

        bool Valid() const
        {
            return iterator == std::ranges::end(range);
        }

        void Seek(Size size)
        {
            auto currentLeft = ValueSize - offset;
            auto toCopy = std::min(currentLeft, size);
            size -= toCopy;
            offset += toCopy;

            if (size)
            {
                std::ranges::advance(iterator, size / ValueSize);
                offset = size % ValueSize;
            }
        }


    };

    template<std::ranges::range Range> 
    struct RangeOutputStream
    {
        using Iterator = std::ranges::iterator_t<Range>;
        using Value = std::ranges::range_value_t<Range>;

        constexpr inline static Size ValueSize = (Size)sizeof(Value);

        Range&& range;
        Iterator iterator;

        Value value;
        Size offset;

        RangeOutputStream(Range&& range) : range(std::forward<Range>(range)), iterator(std::ranges::begin(range)), offset{ 0 } {}

        void Write(const std::byte* src, Size size)
        {     
            while (size)
            {              
                auto currentLeft = ValueSize - offset;

                auto toCopy = std::min(currentLeft, size);
                std::memcpy(AsBytePtr(&value) + offset, std::exchange(src, src + toCopy), toCopy);

                offset += toCopy;
                size -= toCopy;
                
                currentLeft = ValueSize - offset;
                
                if (currentLeft == 0)
                {
                    *iterator++ = value;
                    offset = 0;
                }
            }
        }

        bool Valid() const
        {
            return iterator == std::ranges::end(range);
        }

        void Seek(Size size)
        {
            while (size)
            {
                auto currentLeft = ValueSize - offset;

                auto toCopy = std::min(currentLeft, size);
                std::memset(AsBytePtr(&value) + offset, 0, toCopy);

                offset += toCopy;
                size -= toCopy;

                currentLeft = ValueSize - offset;

                if (currentLeft == 0)
                {
                    *iterator++ = value;
                    offset = 0;
                }
            }
        }


    };

    struct ReadFromRangeFunctor : functional::ExtensionMethod
    {
        template<std::ranges::contiguous_range Range>
        auto ExtensionInvoke(Range&& r) const
        {
            auto data = AsBytePtr(std::ranges::data(r));
            auto size = std::ranges::size(r) * sizeof(std::ranges::range_value_t<Range>);

            return ContiguousStream<true>(data, data + size);
        }

        template<std::ranges::input_range Range>
        auto ExtensionInvoke(Range&& r) const
        {
            return RangeInputStream(std::forward<Range>(r));
        }
    };
    inline constexpr ReadFromRangeFunctor ReadFromRange;

    struct WriteToRangeFunctor : functional::ExtensionMethod
    {
        template<std::ranges::contiguous_range Range>
        auto ExtensionInvoke(Range&& r) const
        {
            auto data = AsBytePtr(std::ranges::data(r));
            auto size = std::ranges::size(r) * sizeof(std::ranges::range_value_t<Range>);

            return ContiguousStream<false>(data, data + size);
        }

        template<std::ranges::range Range>
        auto ExtensionInvoke(Range&& r) const
        {
            return RangeOutputStream(std::forward<Range>(r));
        }
    };
    inline constexpr WriteToRangeFunctor WriteToRange;

    template<meta::TriviallyCopyable Type, Stream Underlying>
    struct StreamIterator
    {
        Underlying &&stream;

        StreamIterator(Underlying&& stream) : stream(std::forward<Underlying>(stream)) {}

        using iterator_concept = std::input_iterator_tag;
    };
    //static_assert(std::input_or_output_iterator<StreamIterator<std::byte, ExpositionOnlyStream>>, "StreamIterator must be std::input_or_output_iterator");
}