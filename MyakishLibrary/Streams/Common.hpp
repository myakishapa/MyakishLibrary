#pragma once

#include <filesystem>
#include <fstream>
#include <ranges>

#include <MyakishLibrary/Meta.hpp>

#include <MyakishLibrary/Streams/Streams.hpp>

#include <MyakishLibrary/Utility.hpp>

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

namespace fs = std::filesystem;

namespace myakish::streams
{
    template<bool Const>
    struct ContiguousStream
    {
        using DataType = std::conditional_t<Const, const std::byte, std::byte>;

        DataType* data;
        const std::byte* const sentinel;

        ContiguousStream() = default;

        ContiguousStream(DataType* data, const std::byte* const sentinel) : data(data), sentinel(sentinel) {}
        ContiguousStream(DataType* data, Size size) : data(data), sentinel(data + size) {}

        std::byte* Write(Size size) requires !Const
        {
            return std::exchange(data, data + size);
        }

        const std::byte* Read(Size size)
        {
            return std::exchange(data, data + size);
        }

        void Seek(Size seek)
        {
            data += seek;
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
    static_assert(PointerOutputStream<ContiguousStream<false>>, "non-const ContiguousStream must be OutputStream");
    static_assert(PointerInputStream<ContiguousStream<true>>, "const ContiguousStream must be InputStream");
    static_assert(SizedStream<ContiguousStream<true>>, "const ContiguousStream must be SizedStream");
    static_assert(PersistentDataStream<ContiguousStream<true>>, "const ContiguousStream must be PersistentDataStream");
    static_assert(!OutputStream<ContiguousStream<true>>, "const ContiguousStream must not be OutputStream");

    using ConstContiguosStream = ContiguousStream<true>;

    ContiguousStream(std::byte*, const std::byte*) -> ContiguousStream<false>;
    ContiguousStream(const std::byte*, const std::byte*) -> ContiguousStream<true>;

    struct VectorOutputStream
    {
        std::vector<std::byte>& data;

        VectorOutputStream(std::vector<std::byte>& data) : data(data) {}

        void Seek(myakish::Size size)
        {
            data.resize(data.size() + size);
        }

        void Write(const std::byte* src, myakish::Size size)
        {
            auto current = data.size();
            data.resize(data.size() + size);
            std::memcpy(data.data() + current, src, size);
        }

        void Reserve(myakish::Size reserve)
        {
            data.reserve(data.size() + reserve);
        }
    };
    static_assert(ReservableStream<VectorOutputStream>, "VectorOutputStream must be ReservableStream");


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
    static_assert(AlignableStream<AlignableWrapper<StreamArchetype>>, "AlignableWrapper must be AlignableStream");
    static_assert(SizedStream<AlignableWrapper<SizedStreamArchetype>>, "AlignableWrapper must be SizedStream");
    static_assert(InputStream<AlignableWrapper<InputStreamArchetype>>, "AlignableWrapper must be InputStream");
    static_assert(ReservableStream<AlignableWrapper<ReservableStreamArchetype>>, "AlignableWrapper must be ReservableStream");

    struct AlignedFunctor : functional::ExtensionMethod
    {
        auto operator()(Stream auto stream) const
        {
            return AlignableWrapper(std::move(stream));
        }

        auto operator()(AlignableStream auto alignable) const
        {
            return std::move(alignable);
        }
    };
    inline constexpr AlignedFunctor Aligned;


    template<OutputStream Underlying>
    struct WriteOnlyWrapper
    {
        Underlying stream;

        WriteOnlyWrapper(Underlying stream) : stream(std::move(stream)) {}

        void Seek(Size size)
        {
            streams::Seek(stream, size);
        }

        Size Length() const requires SizedStream<Underlying>
        {
            return streams::Length(stream);
        }

        Size Offset() const requires AlignableStream<Underlying>
        {
            return streams::Offset(stream);
        }

        void Write(const std::byte* src, Size size)
        {
            streams::Write(stream, src, size);
        }

        void Reserve(Size reserve) requires ReservableStream<Underlying>
        {
            streams::Reserve(stream, reserve);
        }

        auto Data() const requires PersistentDataStream<Underlying>
        {
            return streams::Data(stream);
        }
    };
    static_assert(AlignableStream<WriteOnlyWrapper<CombinedArchetype<AlignableStreamArchetype, OutputStreamArchetype>>>, "WriteOnlyWrapper must be AlignableStream");
    static_assert(SizedStream<WriteOnlyWrapper<CombinedArchetype<SizedStreamArchetype, OutputStreamArchetype>>>, "WriteOnlyWrapper must be RandomAccessStream");
    static_assert(OutputStream<WriteOnlyWrapper<OutputStreamArchetype>>, "WriteOnlyWrapper must be OutputStream");
    static_assert(ReservableStream<WriteOnlyWrapper<ReservableStreamArchetype>>, "WriteOnlyWrapper must be ReservableStream");

    inline constexpr auto WriteOnly = functional::DeduceConstruct<WriteOnlyWrapper>;

    
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
        constexpr void operator()(InputStream auto&& in, OutputStream auto&& out, Size bytes) const
        {
            std::vector<std::byte> buffer(bytes);
            in.Read(buffer.data(), bytes);
            out.Write(buffer.data(), bytes);
        }
    };
    inline constexpr CopyFunctor Copy;

    struct WriteTrivialFunctor : functional::ExtensionMethod
    {
        constexpr void operator()(OutputStream auto&& out, myakish::meta::TriviallyCopyableConcept auto value) const
        {
            Write(out, reinterpret_cast<const std::byte*>(&value), sizeof(value));
        }

        constexpr void operator()(OutputStream auto&& out, std::string_view str) const
        {
            Write(out, reinterpret_cast<const std::byte*>(str.data()), str.size());
        }
    };
    inline constexpr WriteTrivialFunctor WriteTrivial;

    template<myakish::meta::TriviallyCopyableConcept Type>
    struct WriteAsFunctor : functional::ExtensionMethod
    {
        constexpr void operator()(OutputStream auto&& out, Type value) const
        {
            Write(out, reinterpret_cast<const std::byte*>(&value), sizeof(value));
        }

        constexpr void operator()(OutputStream auto&& out, std::string_view str) const requires std::same_as<Type, std::string_view>
        {
            Write(out, reinterpret_cast<const std::byte*>(str.data()), str.size());
        }
    };
    template<myakish::meta::TriviallyCopyableConcept Type>
    inline constexpr WriteAsFunctor<Type> WriteAs;

    template<myakish::meta::TriviallyCopyableConcept Type>
    struct ReadTrivialFunctor : functional::ExtensionMethod
    {
        constexpr Type operator()(InputStream auto&& in) const
        {
            Type result;
            Read(in, reinterpret_cast<std::byte*>(&result), sizeof(result));
            return result;
        }
    };
    template<myakish::meta::TriviallyCopyableConcept Type>
    inline constexpr ReadTrivialFunctor<Type> ReadTrivial;

    struct AlignFunctor : functional::ExtensionMethod
    {
        constexpr void operator()(AlignableStream auto&& stream, Size alignment) const
        {
            stream | Seek(Padding(stream.Offset(), alignment));
        }
    };
    inline constexpr AlignFunctor Align;

    struct PolymorphicStreamBase
    {
        virtual void Seek(Size size) const = 0; // Stream
        
        virtual Size Length() const = 0; // SizedStream

        virtual Size Offset() const = 0; // AlignableStream

        virtual void Read(std::byte* dst, Size size) const = 0; // InputStream

        virtual const std::byte* Read(Size size) const = 0; // PointerInputStream

        virtual void Write(const std::byte* src, Size size) const = 0; // OutputStream

        virtual std::byte* Write(Size size) const = 0; // PointerOutputStream

        virtual void Reserve(Size reserve) const = 0; // ReservableStream

        virtual std::byte* Data() const = 0; // PersistentDataStream
    };
    static_assert(AlignableStream<PolymorphicStreamBase>, "PolymorphicStreamBase must be AlignableStream");
    static_assert(SizedStream<PolymorphicStreamBase>, "PolymorphicStreamBase must be SizedStream");
    static_assert(PointerInputStream<PolymorphicStreamBase>, "PolymorphicStreamBase must be PointerInputStream");
    static_assert(PointerOutputStream<PolymorphicStreamBase>, "PolymorphicStreamBase must be PointerOutputStream");
    static_assert(ReservableStream<PolymorphicStreamBase>, "PolymorphicStreamBase must be ReservableStream");
    static_assert(PersistentDataStream<PolymorphicStreamBase>, "PolymorphicStreamBase must be PersistentDataStream");

    template<Stream Underlying>
    struct PolymorphicStream : PolymorphicStreamBase
    {
        Underlying &&stream;

        PolymorphicStream(Underlying &&stream) : stream(std::forward<Underlying>(stream)) {}

        virtual void Seek(Size size) const override
        {
            streams::Seek(stream, size);
        }


        virtual Size Length() const override
        {
            if constexpr (SizedStream<Underlying>) return streams::Length(stream);
            else std::terminate();
        }

        virtual Size Offset() const override
        {
            if constexpr (AlignableStream<Underlying>) return streams::Offset(stream);
            else std::terminate();
        }

        virtual void Read(std::byte* dst, Size size) const override
        {
            if constexpr (InputStream<Underlying>) streams::Read(stream, dst, size);
            else std::terminate();
        }

        virtual const std::byte* Read(Size size) const override
        {
            if constexpr (PointerInputStream<Underlying>) return streams::Read(stream, size);
            else std::terminate();
        }

        virtual void Write(const std::byte* src, Size size) const override
        {
            if constexpr (OutputStream<Underlying>) streams::Write(stream, src, size);
            else std::terminate();
        }

        virtual std::byte* Write(Size size) const override
        {
            if constexpr (PointerOutputStream<Underlying>) return streams::Write(stream, size);
            else std::terminate();
        }

        virtual void Reserve(Size reserve) const override
        {
            if constexpr (ReservableStream<Underlying>) streams::Reserve(stream, reserve);
            else std::terminate();
        }

        virtual std::byte* Data() const override
        {
            if constexpr (PersistentDataStream<Underlying>) return const_cast<std::byte*>(streams::Data(stream));
            else std::terminate();
        }
    };

    template<Stream Underlying>
    PolymorphicStream(Underlying&&) -> PolymorphicStream<Underlying&&>;

    inline constexpr auto Polymorphize = functional::DeduceConstruct<PolymorphicStream>;


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
        auto operator()(Range&& r) const
        {
            auto data = AsBytePtr(std::ranges::data(r));
            auto size = std::ranges::size(r) * sizeof(std::ranges::range_value_t<Range>);

            return ContiguousStream<true>(data, data + size);
        }

        template<std::ranges::input_range Range>
        auto operator()(Range&& r) const
        {
            return RangeInputStream(std::forward<Range>(r));
        }
    };
    inline constexpr ReadFromRangeFunctor ReadFromRange;

    struct WriteToRangeFunctor : functional::ExtensionMethod
    {
        template<std::ranges::contiguous_range Range>
        auto operator()(Range&& r) const
        {
            auto data = AsBytePtr(std::ranges::data(r));
            auto size = std::ranges::size(r) * sizeof(std::ranges::range_value_t<Range>);

            return ContiguousStream<false>(data, data + size);
        }

        template<std::ranges::range Range>
        auto operator()(Range&& r) const
        {
            return RangeOutputStream(std::forward<Range>(r));
        }
    };
    inline constexpr WriteToRangeFunctor WriteToRange;

    template<myakish::meta::TriviallyCopyableConcept Type, Stream Underlying>
    struct StreamIterator
    {
        Underlying &&stream;

        StreamIterator(Underlying&& stream) : stream(std::forward<Underlying>(stream)) {}

        using iterator_concept = std::input_iterator_tag;
    };
    //static_assert(std::input_or_output_iterator<StreamIterator<std::byte, ExpositionOnlyStream>>, "StreamIterator must be std::input_or_output_iterator");
}