#pragma once
#include <functional>
#include <concepts>
#include <array>
#include <utility>
#include <tuple>
#include <variant>

#include <MyakishLibrary/Streams/Common.hpp>

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

#include <MyakishLibrary/Meta.hpp>

#include <MyakishLibrary/Algebraic/Algebraic.hpp>
#include <MyakishLibrary/Algebraic/Standard.hpp>


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


    struct NoOpParser : ParserBase
    {
        void IO(streams::Stream auto&&, auto&&... _) const
        {

        }
    };
    inline constexpr NoOpParser NoOp;

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
            value = in | streams::Read<Type>;
        }

        void IO(streams::OutputStream auto&& out, AttributeLike<Type> auto&& value) const
        {
            out | streams::WriteAs<Type>[value];
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

        template<streams::Stream Stream, typename ...Attributes>
        void IO(Stream&& stream, Attributes&&... attributes) const
        {
            f.IO(stream, attributes...);
            s.IO(stream, attributes...);
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


        template<streams::Stream Stream, typename ArgAttribute>
        void IO(Stream&& stream, ArgAttribute&& attribute) const
        {
            parser.IO(stream, attribute);
        }

        template<streams::InputStream Stream>
        Type Parse(Stream&& stream) const
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

    template<typename Type, ParserConcept ...Parsers>
    struct SequenceRuleParser : ParserBase
    {
        using Attribute = Type;

        std::tuple<Parsers...> parsers;

        constexpr SequenceRuleParser(Parsers... parsers) : parsers(std::move(parsers)...) {}


        template<streams::Stream Stream, typename ArgAttribute>
        void IO(Stream&& stream, ArgAttribute&& attribute) const
        {
            auto Func = [&](auto& parser)
                {
                    parser.IO(stream, attribute);
                };

            using namespace functional::operators;
            
            parsers | algebraic::Iterate(Func);
        }

        template<streams::InputStream Stream>
        Type Parse(Stream&& stream) const
        {
            Type synthesized{};
            IO(stream, synthesized);
            return synthesized;
        }
    };


    template<typename Type, ParserConcept ...Parsers>
    constexpr auto SequenceRule(Parsers... parsers)
    {
        return SequenceRuleParser<Type, Parsers...>(std::move(parsers)...);
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

    struct EngageParser : ParserBase
    {
        template<streams::InputStream Stream, typename Variant>
        void IO(Stream&&, myakish::Size index, Variant&& variant) const
        {       
            variant = algebraic::Synthesize<std::remove_cvref_t<Variant>>(index);
        }

        template<streams::OutputStream Stream, typename Variant>
        void IO(Stream&&, myakish::Size index, Variant&& variant) const
        {
            //no-op - variant should be already engaged
        } 
    };
    inline constexpr EngageParser Engage;

    template<MonomorphicParserConcept... Parsers>
    struct VariantParser : ParserBase
    { 
        using Attribute = algebraic::Variant<ParserAttribute<Parsers>...>;

        using Test = meta::TypeList<Parsers...>;

        std::tuple<const Parsers&...> parsers;

        constexpr VariantParser(const Parsers&... parsers) : parsers(parsers...) {}
        constexpr VariantParser(std::tuple<Parsers...> parsers) : parsers(std::move(parsers)) {}

        template<streams::Stream Stream, typename ArgAttribute>
        void IO(Stream&& out, ArgAttribute&& attribute) const
        {
            auto ParseWith = [&](auto&& parser)
                {
                    return [&](auto&& attribute)
                        {
                            parser.IO(out, attribute);
                            return attribute;
                        };
                };

            attribute = std::forward<ArgAttribute>(attribute) | algebraic::Cast<Attribute> | algebraic::Apply(parsers | algebraic::Transform[ParseWith], *algebraic::Multitransform) | algebraic::Cast<std::remove_cvref_t<ArgAttribute>>;
        }
    };

    template<MonomorphicParserConcept... Parsers>
    VariantParser(std::tuple<Parsers...> parsers) -> VariantParser<Parsers...>;

    /*template<MonomorphicParserConcept First, MonomorphicParserConcept Second>
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
    */

    struct FillRangeParser : ParserBase
    {
        template<streams::InputStream Stream, std::ranges::range AttributeRange>
        void IO(Stream&&, myakish::Size count, AttributeRange& attribute) const
        {
            auto CreateDefault = [](auto)
                {
                    return std::ranges::range_value_t<std::remove_cvref_t<AttributeRange>>{};
                };

            //attribute = std::views::iota(0, count) | std::views::transform(CreateDefault) | std::ranges::to<std::remove_cvref_t<AttributeRange>>();
            attribute.resize(count);
        }

        template<streams::OutputStream Stream, std::ranges::range AttributeRange>
        void IO(Stream&&, myakish::Size, AttributeRange&& attribute) const
        {
            //no-op - range exists
        }
    };
    inline constexpr FillRangeParser FillRange;


    template<ParserConcept Parser>
    struct RepeatParser : ParserBase
    {
        Parser parser;

        constexpr RepeatParser(Parser parser) : parser(std::move(parser)) {}


        template<streams::Stream Stream, std::ranges::range AttributeRange>
        void IO(Stream&& stream, AttributeRange&& attribute) const
        {
            using namespace myakish::literals;

            auto Parse = [&](auto&& attribute)
                {
                    parser.IO(stream, attribute);
                };

            std::ranges::for_each(attribute, Parse);
        }
    };

}