#pragma once

#include <concepts>
#include <fstream>
#include <filesystem>

#include <MyakishLibrary/Meta.hpp>
#include <MyakishLibrary/Core.hpp>

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

namespace myakish::streams
{
    namespace detail
    {
        enum class Strategy
        {
            None,
            Member,
            ADL
        };
    }

    namespace detail::Seek
    {
        template<typename Stream>
        concept HasMember = requires(Stream && stream, Size direction)
        {
            { stream.Seek(direction) };
        };

        template<typename Stream>
        concept HasADL = requires(Stream && stream, Size direction)
        {
            { SeekADL<Stream>(stream, direction) };
        };

        template<typename Stream>
        consteval static Strategy ChooseStrategy()
        {
            using enum Strategy;

            if constexpr (HasMember<Stream>) return Member;
            else if constexpr (HasADL<Stream>) return ADL;
            else return None;
        }
    }
    struct SeekFunctor : functional::ExtensionMethod
    {
        template<typename Stream> requires(detail::Seek::ChooseStrategy<Stream>() != detail::Strategy::None)
        void operator()(Stream&& stream, Size direction) const
        {
            constexpr auto Strategy = detail::Seek::ChooseStrategy<Stream>();

            using enum detail::Strategy;

            if constexpr (Strategy == Member) stream.Seek(direction);
            else if constexpr (Strategy == ADL) SeekADL<Stream>(stream, direction);
            else static_assert(false);
        }
    };
    inline constexpr SeekFunctor Seek;

    template<typename Type>
    concept Stream = requires(Type&& stream, Size size)
    {
        Seek(stream, size); // size > 0 only
    };

    struct StreamArchetype
    {
        void Seek(Size size);
    };
    static_assert(Stream<StreamArchetype>);




    namespace detail::Offset
    {
        template<typename Stream>
        concept HasMember = requires(Stream && stream)
        {
            { stream.Offset() } -> std::convertible_to<Size>;
        };

        template<typename Stream>
        concept HasADL = requires(Stream && stream)
        {
            { OffsetADL<Stream>(stream) } -> std::convertible_to<Size>;
        };

        template<typename Stream>
        consteval static Strategy ChooseStrategy()
        {
            using enum Strategy;

            if constexpr (HasMember<Stream>) return Member;
            else if constexpr (HasADL<Stream>) return ADL;
            else return None;
        }
    }
    struct OffsetFunctor : functional::ExtensionMethod
    {
        template<typename Stream> requires(detail::Offset::ChooseStrategy<Stream>() != detail::Strategy::None)
        Size operator()(Stream&& stream) const
        {
            constexpr auto Strategy = detail::Offset::ChooseStrategy<Stream>();

            using enum detail::Strategy;

            if constexpr (Strategy == Member) return stream.Offset();
            else if constexpr (Strategy == ADL) return OffsetADL<Stream>(stream);
            else static_assert(false);
        }
    };
    inline constexpr OffsetFunctor Offset;

    template<typename Type>
    concept AlignableStream = Stream<Type> && requires(Type&& stream)
    {
        Offset(stream);
    };

    struct AlignableStreamArchetype : virtual StreamArchetype
    {
        Size Offset() const;
    };
    static_assert(AlignableStream<AlignableStreamArchetype>);




    namespace detail::Length
    {
        template<typename Stream>
        concept HasMember = requires(Stream && stream)
        {
            { stream.Length() } -> std::convertible_to<Size>;
        };

        template<typename Stream>
        concept HasADL = requires(Stream && stream)
        {
            { LengthADL<Stream>(stream) } -> std::convertible_to<Size>;
        };

        template<typename Stream>
        consteval static Strategy ChooseStrategy()
        {
            using enum Strategy;

            if constexpr (HasMember<Stream>) return Member;
            else if constexpr (HasADL<Stream>) return ADL;
            else return None;
        }
    }
    struct LengthFunctor : functional::ExtensionMethod
    {
        template<typename Stream> requires(detail::Length::ChooseStrategy<Stream>() != detail::Strategy::None)
        Size operator()(Stream&& stream) const
        {
            constexpr auto Strategy = detail::Length::ChooseStrategy<Stream>();

            using enum detail::Strategy;

            if constexpr (Strategy == Member) return stream.Length();
            else if constexpr (Strategy == ADL) return LengthADL<Stream>(stream);
            else static_assert(false);
        }
    };
    inline constexpr LengthFunctor Length;

    template<typename Type>
    concept SizedStream = Stream<Type> && requires(Type&& stream)
    {
        Length(stream);
    };

    struct SizedStreamArchetype : virtual StreamArchetype
    {
        Size Length() const;
    };
    static_assert(SizedStream<SizedStreamArchetype>);




    
    namespace detail::WriteInto
    {
        template<typename Stream>
        concept HasMember = requires(Stream && stream, const std::byte * data, Size size)
        {
            { stream.Write(data, size) };
        };

        template<typename Stream>
        concept HasADL = requires(Stream && stream, const std::byte * data, Size size)
        {
            { WriteADL<Stream>(stream, data, size) };
        };

        template<typename Stream>
        consteval static Strategy ChooseStrategy()
        {
            using enum Strategy;

            if constexpr (HasMember<Stream>) return Member;
            else if constexpr (HasADL<Stream>) return ADL;
            else return None;
        }
    }
    namespace detail::Write
    {
        template<typename Stream>
        concept HasMember = requires(Stream && stream, Size size)
        {
            { stream.Write(size) } -> std::convertible_to<std::byte*>;
        };

        template<typename Stream>
        concept HasADL = requires(Stream && stream, Size size)
        {
            { WriteADL<Stream>(stream, size) } -> std::convertible_to<std::byte*>;
        };

        template<typename Stream>
        consteval static Strategy ChooseStrategy()
        {
            using enum Strategy;

            if constexpr (HasMember<Stream>) return Member;
            else if constexpr (HasADL<Stream>) return ADL;
            else return None;
        }
    }
    struct WriteFunctor : functional::ExtensionMethod
    {
        template<typename Stream> requires(detail::Write::ChooseStrategy<Stream>() != detail::Strategy::None)
        std::byte* operator()(Stream&& stream, Size size) const
        {
            constexpr auto Strategy = detail::Write::ChooseStrategy<Stream>();

            using enum detail::Strategy;

            if constexpr (Strategy == Member) return stream.Write(size);
            else if constexpr (Strategy == ADL) return WriteADL<Stream>(stream, size);
            else static_assert(false);
        }

        template<typename Stream> requires((detail::WriteInto::ChooseStrategy<Stream>() != detail::Strategy::None) || (detail::Write::ChooseStrategy<Stream>() != detail::Strategy::None))
        void operator()(Stream && stream, const std::byte * data, Size size) const
        {
            constexpr auto Strategy = detail::WriteInto::ChooseStrategy<Stream>();

            using enum detail::Strategy;

            if constexpr (Strategy == Member) stream.Write(data, size);
            else if constexpr (Strategy == ADL) WriteADL<Stream>(stream, data, size);
            else if constexpr (detail::Write::ChooseStrategy<Stream>() != detail::Strategy::None)
            {
                std::memcpy(operator()(stream, size), data, size);
            }
            else static_assert(false);
        }
    };
    inline constexpr WriteFunctor Write;

    template<typename Type>
    concept OutputStream = Stream<Type> && requires(Type&& out, const std::byte * data, Size size)
    {
        Write(out, data, size);
    };

    struct OutputStreamArchetype : virtual StreamArchetype
    {
        void Write(const std::byte* src, Size size);
    };
    static_assert(OutputStream<OutputStreamArchetype>);


    template<typename Type>
    concept PointerOutputStream = OutputStream<Type> && requires(Type && out, Size size)
    {
        Write(out, size);
    };

    struct PointerOutputStreamArchetype : virtual OutputStreamArchetype
    {
        using OutputStreamArchetype::Write;

        std::byte* Write(Size size);
    };
    static_assert(PointerOutputStream<PointerOutputStreamArchetype>);





    namespace detail::ReadInto
    {
        template<typename Stream>
        concept HasMember = requires(Stream && stream, std::byte * data, Size size)
        {
            { stream.Read(data, size) };
        };

        template<typename Stream>
        concept HasADL = requires(Stream && stream, std::byte * data, Size size)
        {
            { ReadADL<Stream>(stream, data, size) };
        };

        template<typename Stream>
        consteval static Strategy ChooseStrategy()
        {
            using enum Strategy;

            if constexpr (HasMember<Stream>) return Member;
            else if constexpr (HasADL<Stream>) return ADL;
            else return None;
        }
    }
    namespace detail::Read
    {
        template<typename Stream>
        concept HasMember = requires(Stream && stream, Size size)
        {
            { stream.Read(size) } -> std::convertible_to<const std::byte*>;
        };

        template<typename Stream>
        concept HasADL = requires(Stream && stream, Size size)
        {
            { ReadADL<Stream>(stream, size) } -> std::convertible_to<const std::byte*>;
        };

        template<typename Stream>
        consteval static Strategy ChooseStrategy()
        {
            using enum Strategy;

            if constexpr (HasMember<Stream>) return Member;
            else if constexpr (HasADL<Stream>) return ADL;
            else return None;
        }
    }
    struct ReadFunctor : functional::ExtensionMethod
    {
        template<typename Stream> requires((detail::Read::ChooseStrategy<Stream>() != detail::Strategy::None) || PointerOutputStream<Stream>)
        const std::byte* operator()(Stream&& stream, Size size) const
        {
            constexpr auto Strategy = detail::Read::ChooseStrategy<Stream>();

            using enum detail::Strategy;

            if constexpr (Strategy == Member) return stream.Read(size);
            else if constexpr (Strategy == ADL) return ReadADL<Stream>(stream, size);
            else if constexpr (PointerOutputStream<Stream>) return Write(stream, size);
            else static_assert(false);
        }

        template<typename Stream> requires((detail::ReadInto::ChooseStrategy<Stream>() != detail::Strategy::None) || (detail::Read::ChooseStrategy<Stream>() != detail::Strategy::None))
        void operator()(Stream&& stream, std::byte* data, Size size) const
        {
            constexpr auto Strategy = detail::ReadInto::ChooseStrategy<Stream>();

            using enum detail::Strategy;

            if constexpr (Strategy == Member) stream.Read(data, size);
            else if constexpr (Strategy == ADL) ReadADL<Stream>(stream, data, size);
            else if constexpr (detail::Read::ChooseStrategy<Stream>() != detail::Strategy::None)
            {
                std::memcpy(data, operator()(stream, size), size);
            }
            else static_assert(false);
        }
    };
    inline constexpr ReadFunctor Read;

    template<typename Type>
    concept InputStream = Stream<Type> && requires(Type&& in, std::byte* data, Size size)
    {
        Read(in, data, size);
    };

    struct InputStreamArchetype : virtual StreamArchetype
    {
        void Read(std::byte* dst, Size size);
    };
    static_assert(InputStream<InputStreamArchetype>);


    template<typename Type>
    concept PointerInputStream = InputStream<Type> && requires(Type && in, Size size)
    {
        Read(in, size);
    };

    struct PointerInputStreamArchetype : virtual InputStreamArchetype
    {
        using InputStreamArchetype::Read;

        const std::byte* Read(Size size);
    };
    static_assert(PointerInputStream<PointerInputStreamArchetype>);









    namespace detail::Reserve
    {
        template<typename Stream>
        concept HasMember = requires(Stream && stream, Size reserve)
        {
            { stream.Reserve(reserve) };
        };

        template<typename Stream>
        concept HasADL = requires(Stream && stream, Size reserve)
        {
            { ReserveADL<Stream>(stream, reserve) };
        };

        template<typename Stream>
        consteval static Strategy ChooseStrategy()
        {
            using enum Strategy;

            if constexpr (HasMember<Stream>) return Member;
            else if constexpr (HasADL<Stream>) return ADL;
            else return None;
        }
    }
    struct ReserveFunctor : functional::ExtensionMethod
    {
        template<typename Stream> requires(detail::Reserve::ChooseStrategy<Stream>() != detail::Strategy::None)
        void operator()(Stream&& stream, Size reserve) const
        {
            constexpr auto Strategy = detail::Reserve::ChooseStrategy<Stream>();

            using enum detail::Strategy;

            if constexpr (Strategy == Member) stream.Reserve(reserve);
            else if constexpr (Strategy == ADL) ReserveADL<Stream>(stream, reserve);
            else static_assert(false);
        }
    };
    inline constexpr ReserveFunctor Reserve;

    template<typename Type>
    concept ReservableStream = OutputStream<Type> && requires(Type&& out, Size reserve)
    {
        Reserve(out, reserve);
    };

    struct ReservableStreamArchetype : virtual OutputStreamArchetype
    {
        void Reserve(Size reserve);
    };
    static_assert(ReservableStream<ReservableStreamArchetype>);




    namespace detail::Data
    {
        template<typename Stream>
        concept HasMember = requires(Stream && stream)
        {
            { stream.Data() } -> std::convertible_to<std::conditional_t<OutputStream<Stream>, std::byte, const std::byte>*>;
        };

        template<typename Stream>
        concept HasADL = requires(Stream && stream)
        {
            { DataADL<Stream>(stream) } -> std::convertible_to<std::conditional_t<OutputStream<Stream>, std::byte, const std::byte>*>;
        };

        template<typename Stream>
        consteval static Strategy ChooseStrategy()
        {
            using enum Strategy;

            if constexpr (HasMember<Stream>) return Member;
            else if constexpr (HasADL<Stream>) return ADL;
            else return None;
        }
    }
    struct DataFunctor : functional::ExtensionMethod
    {
        template<typename Stream> requires(detail::Data::ChooseStrategy<Stream>() != detail::Strategy::None)
            decltype(auto) operator()(Stream&& stream) const
        {
            constexpr auto Strategy = detail::Data::ChooseStrategy<Stream>();

            using enum detail::Strategy;

            if constexpr (Strategy == Member) return stream.Data();
            else if constexpr (Strategy == ADL) return DataADL<Stream>(stream);
            else static_assert(false);
        }
    };
    inline constexpr DataFunctor Data;

    template<typename Type>
    concept PersistentDataStream = PointerInputStream<Type> && SizedStream<Type> && requires(Type&& stream)
    {
        Data(stream);
    };

    struct PersistentDataStreamArchetype : virtual PointerInputStreamArchetype, virtual SizedStreamArchetype
    {
        const std::byte* Data() const;
    };
    static_assert(PersistentDataStream<PersistentDataStreamArchetype>);

    struct MutablePersistentDataStreamArchetype : virtual PointerInputStreamArchetype, virtual SizedStreamArchetype, virtual OutputStreamArchetype
    {
        std::byte* Data() const;
    };
    static_assert(PersistentDataStream<MutablePersistentDataStreamArchetype>);


    template<typename... Archetypes>
    struct CombinedArchetype : virtual Archetypes... {};
}