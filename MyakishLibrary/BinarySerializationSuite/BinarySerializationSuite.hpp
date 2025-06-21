#pragma once
#include <functional>
#include <concepts>
#include <array>
#include <utility>
#include <tuple>
#include <variant>

#include <MyakishLibrary/Streams/Common.hpp>

#include <MyakishLibrary/Functional/Pipeline.hpp>
#include <MyakishLibrary/Functional/ExtensionMethod.hpp>
#include <MyakishLibrary/Functional/Algebraic.hpp>

#include <MyakishLibrary/Meta/Concepts.hpp>


namespace myakish::binary_serialization_suite
{
    template<typename Underlying, typename ...Projections>
    struct ProjectedParser;


    struct ParserBase
    {
        template<typename Self, typename Projection>
        constexpr auto operator[](this Self&& self, Projection projection)
        {
            return ProjectedParser(std::move(self), std::move(projection));
        }

        template<typename Self, typename ...Projections>
        constexpr auto operator()(this Self&& self, Projections... projections)
        {
            return ProjectedParser(std::move(self), std::move(projections)...);
        }
    };

    template<typename Type>
    concept ParserConcept = std::derived_from<Type, ParserBase>;

    template<typename Underlying, typename ...Projections>
    struct ProjectedParser : ParserBase
    {
        Underlying parser;
        std::tuple<Projections...> projections;

        constexpr ProjectedParser(Underlying parser, Projections... projections) : parser(std::move(parser)), projections(std::move(projections)...) {}

        template<streams::Stream Stream, typename Attribute> requires (std::invocable<Projections, Attribute> && ...)
        void IO(Stream&& stream, Attribute&& attribute) const
        {
            IO(stream, attribute, std::make_index_sequence<sizeof...(Projections)>{});
            //parser.IO(stream, std::invoke(projection, attribute));
        }

    private:

        template<streams::Stream Stream, typename Attribute, std::size_t ...Indices>
        void IO(Stream&& stream, Attribute&& attribute, std::index_sequence<Indices...>) const
        {
            parser.IO(stream, std::invoke(std::get<Indices>(projections), attribute)...);
        }
    };


    /*
    template<typename Factory>
    struct DynamicParser : ParserBase
    {
        Factory factory;


        constexpr DynamicParser(Factory factory) : factory(std::move(factory)) {}

        template<streams::Stream Stream, typename Attribute> requires std::invocable<Factory, Attribute>
        void IO(Stream&& stream, Attribute&& attribute) const
        {
            std::invoke(factory, attribute).IO(stream, attribute);
        }
    };

    template<typename Type, typename InitProjection, typename ParserProjection>
    constexpr auto Dynamic(InitProjection initProjection, ParserProjection parserProjection)
    {
        return DynamicParser([=](auto&& attrib)
            {
                return Type(std::invoke(initProjection, attrib))[parserProjection];
            });
    }
    */


    template<typename Type, typename Attribute>
    concept AttributeLike = requires(Type attributeLike, Attribute attribute)
    {
        attributeLike = attribute;
        static_cast<Attribute>(attributeLike);
    };



    template<typename Type>
    struct TrivialParser : ParserBase
    {
        using Attribute = Type;

        void IO(streams::InputStream auto&& in, AttributeLike<Type> auto&& value) const
        {
            using namespace myakish::functional::operators;

            value = in | streams::Read<Type>;
        }

        void IO(streams::OutputStream auto&& out, AttributeLike<Type> auto&& value) const
        {
            using namespace myakish::functional::operators;

            out | streams::WriteAs<Type>(value);
        }
    };

    template<typename Type>
    inline constexpr TrivialParser<Type> Trivial;

    inline constexpr TrivialParser<int> Int;
    inline constexpr TrivialParser<myakish::Size> Size;
    inline constexpr TrivialParser<std::uint64_t> U64;



    struct RawParser : ParserBase
    {       
        void IO(streams::InputStream auto&& in, void* ptr, myakish::Size count) const
        {
            in.Read(myakish::AsBytePtr(ptr), count);
        }

        void IO(streams::OutputStream auto&& out, const void* ptr, myakish::Size count) const
        {
            out.Write(myakish::AsBytePtr(ptr), count);
        }
    };
    inline constexpr RawParser Raw;



    template<typename ReadProjection, typename WriteProjection>
    struct FakeProjection
    {
        ReadProjection read;
        WriteProjection write;

        constexpr FakeProjection(ReadProjection read, WriteProjection write) : read(std::move(read)), write(std::move(write)) {}

        template<typename Attribute>
        struct Closure
        {
            const FakeProjection& projections;
            Attribute&& attribute;

            constexpr Closure(Attribute&& attribute, const FakeProjection& projections) : attribute(std::forward<Attribute>(attribute)), projections(projections) {}

            operator std::invoke_result_t<ReadProjection, Attribute&&>() const
            {
                return std::invoke(projections.read, std::forward<Attribute>(attribute));
            }

            template<typename Arg> requires std::invocable<WriteProjection, Attribute&&, Arg&&>
            void operator=(Arg&& arg) const
            {
                std::invoke(projections.write, std::forward<Attribute>(attribute), std::forward<Arg>(arg));
            }
        };

        template<typename Attribute>
        constexpr auto operator()(Attribute&& attribute) const
        {
            return Closure<Attribute>(std::forward<Attribute>(attribute), *this);
        }
    };

    template<typename ReadProjection, typename WriteProjection>
    FakeProjection(ReadProjection, WriteProjection) -> FakeProjection<ReadProjection, WriteProjection>;



    template<ParserConcept First, ParserConcept Second>
    struct SequenceParser : ParserBase
    {
        First f;
        Second s;

        constexpr SequenceParser(First f, Second s) : f(std::move(f)), s(std::move(s)) {}

        template<streams::Stream Stream, typename Attribute>
        void IO(Stream&& stream, Attribute&& attribute) const
        {
            f.IO(stream, attribute);
            s.IO(stream, attribute);
        }
    };

    template<ParserConcept First, ParserConcept Second>
    constexpr auto operator>>(First f, Second s)
    {
        return SequenceParser(std::move(f), std::move(s));
    }



    struct Align : ParserBase
    {
        myakish::Size alignment;
        
        constexpr Align(myakish::Size alignment) : alignment(alignment) {}

        void IO(streams::AlignableStream auto&& stream, auto&&) const
        {
            using namespace myakish::functional::operators;

            stream | streams::Align(alignment);
        }
    };



    template<typename Type, ParserConcept Parser>
    struct RuleParser : ParserBase
    {
        using Attribute = Type;

        Parser parser;

        constexpr RuleParser(Parser parser) : parser(std::move(parser)) {}


        template<streams::Stream Stream>
        void IO(Stream&& stream, myakish::meta::SameBase<Type> auto&& attribute) const
        {
            parser.IO(stream, attribute);
        }

        template<streams::InputStream Stream>
        Type Parse(Stream&& stream)
        {
            Type synthesized{};
            IO(stream, synthesized);
            return synthesized;
        }
    };

    template<typename Type, ParserConcept Parser>
    constexpr auto Rule(Parser parser)
    {
        return RuleParser<Type, Parser>(std::move(parser));
    }



    template<myakish::Size Bytes>
    struct Blob
    {
        std::array<std::byte, Bytes> storage;
    }; 



    template<typename Type>
    concept MonomorphicParserConcept = ParserConcept<Type> && requires()
    {
        typename Type::Attribute;
    };

    template<MonomorphicParserConcept Parser>
    using ParserAttribute = Parser::Attribute;


    template<MonomorphicParserConcept... Parsers>
    struct VariantParser : ParserBase
    { 
        using Attribute = std::variant<ParserAttribute<Parsers>...>;

        std::tuple<Parsers...> parsers;

        constexpr VariantParser(Parsers... parsers) : parsers(std::move(parsers)...) {}
        constexpr VariantParser(std::tuple<Parsers...> parsers) : parsers(std::move(parsers)) {}

        template<streams::InputStream Stream, typename ArgAttribute>
        void IO(Stream&& in, std::size_t index, ArgAttribute& attribute) const
        {
            using namespace functional::algebraic;
            using namespace functional::operators;

            auto ParseWith = [&](auto& parser)
                {
                    return [&](auto&& attribute)
                        {
                            parser.IO(in, attribute);
                            return attribute;
                        };
                };

            attribute = Synthesize<Attribute>(index) | Multitransform(parsers | Transform(ParseWith)) | Cast<std::remove_reference_t<ArgAttribute>>();
        }

        template<streams::OutputStream Stream, typename ArgAttribute>
        void IO(Stream&& out, std::size_t, ArgAttribute&& attribute) const
        {            
            using namespace functional::algebraic;
            using namespace functional::operators;

            auto ParseWith = [&](auto& parser)
                {
                    return [&](auto&& attribute)
                        {
                            parser.IO(out, attribute);
                            return attribute;
                        };
                };

            std::forward<ArgAttribute>(attribute) | Cast<Attribute>() | std::apply(Multitransform, parsers | Transform(ParseWith));
        }
    };

    template<MonomorphicParserConcept... Parsers>
    VariantParser(std::tuple<Parsers...> parsers) -> VariantParser<Parsers...>;


    template<MonomorphicParserConcept First, MonomorphicParserConcept Second>
    constexpr auto operator|(First f, Second s)
    {
        return VariantParser(std::move(f), std::move(s));
    }

    template<MonomorphicParserConcept... LeftParsers, MonomorphicParserConcept Second>
    constexpr auto operator|(VariantParser<LeftParsers...> f, Second s)
    {
        return VariantParser(std::tuple_cat( std::move(f.parsers), std::tuple(std::move(s)) ));
    }

    template<MonomorphicParserConcept First, MonomorphicParserConcept... RightParsers>
    constexpr auto operator|(First f, VariantParser<RightParsers...> s)
    {
        return VariantParser(std::tuple_cat(std::tuple(std::move(f)), std::move(s.parsers)));
    }

    template<MonomorphicParserConcept... LeftParsers, MonomorphicParserConcept... RightParsers>
    constexpr auto operator|(VariantParser<LeftParsers...> f, VariantParser<RightParsers...> s)
    {
        return VariantParser(std::tuple_cat(std::move(f.parsers), std::move(s.parsers)));
    }


}