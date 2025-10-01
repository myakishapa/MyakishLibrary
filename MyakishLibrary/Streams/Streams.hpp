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
            decltype(auto) operator()(Stream&& stream, Size direction) const
        {
            constexpr auto Strategy = detail::Seek::ChooseStrategy<Stream>();

            using enum detail::Strategy;

            if constexpr (Strategy == Member) return stream.Seek(direction);
            else if constexpr (Strategy == ADL) return SeekADL<Stream>(stream, direction);
            else static_assert(false);
        }
    };
    inline constexpr SeekFunctor Seek;

    template<typename Type>
    concept Stream = requires(Type&& stream, Size size)
    {
        Seek(stream, size); // size > 0 only
    };


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
            decltype(auto) operator()(Stream&& stream) const
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
            decltype(auto) operator()(Stream&& stream) const
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


    namespace detail::Read
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
    struct ReadFunctor : functional::ExtensionMethod
    {
        template<typename Stream> requires(detail::Read::ChooseStrategy<Stream>() != detail::Strategy::None)
            decltype(auto) operator()(Stream&& stream, std::byte* data, Size size) const
        {
            constexpr auto Strategy = detail::Read::ChooseStrategy<Stream>();

            using enum detail::Strategy;

            if constexpr (Strategy == Member) return stream.Read(data, size);
            else if constexpr (Strategy == ADL) return ReadADL<Stream>(stream, data, size);
            else static_assert(false);
        }
    };
    inline constexpr ReadFunctor Read;

    template<typename Type>
    concept InputStream = Stream<Type> && requires(Type&& in, std::byte* data, Size size)
    {
        Read(in, data, size);
    };


    namespace detail::Write
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
    struct WriteFunctor : functional::ExtensionMethod
    {
        template<typename Stream> requires(detail::Write::ChooseStrategy<Stream>() != detail::Strategy::None)
            decltype(auto) operator()(Stream&& stream, const std::byte* data, Size size) const
        {
            constexpr auto Strategy = detail::Write::ChooseStrategy<Stream>();

            using enum detail::Strategy;

            if constexpr (Strategy == Member) return stream.Write(data, size);
            else if constexpr (Strategy == ADL) return WriteADL<Stream>(stream, data, size);
            else static_assert(false);
        }
    };
    inline constexpr WriteFunctor Write;

    template<typename Type>
    concept OutputStream = Stream<Type> && requires(Type&& out, const std::byte * data, Size size)
    {
        Write(out, data, size);
    };


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
            decltype(auto) operator()(Stream&& stream, Size reserve) const
        {
            constexpr auto Strategy = detail::Reserve::ChooseStrategy<Stream>();

            using enum detail::Strategy;

            if constexpr (Strategy == Member) return stream.Reserve(reserve);
            else if constexpr (Strategy == ADL) return ReserveADL<Stream>(stream, reserve);
            else static_assert(false);
        }
    };
    inline constexpr ReserveFunctor Reserve;

    template<typename Type>
    concept ReservableStream = OutputStream<Type> && requires(Type&& out, Size reserve)
    {
        Reserve(out, reserve);
    };


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
    concept PersistentDataStream = InputStream<Type> && SizedStream<Type> && requires(Type&& stream)
    {
        Data(stream);
    };
}