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

    namespace detail
    {
        template<typename Type>
        concept HasMemberAttribute = !std::same_as<typename std::remove_cvref_t<Type>::Attribute, meta::UndefinedType>;

        template<typename Type>
        struct MemberAttribute : meta::Undefined {};

        template<HasMemberAttribute Type>
        struct MemberAttribute<Type> : meta::ReturnType<typename std::remove_cvref_t<Type>::Attribute>{};
    }

    struct ParserBase
    {
        template<typename Self, typename ...Projections>
        constexpr auto operator[](this Self&& self, Projections... projections)
        {
            return ProjectedParser(std::move(self), std::move(projections)...);
        }

        template<detail::HasMemberAttribute Self, streams::InputStream Stream>
        auto Parse(this Self&& self, Stream&& stream)
        {
            typename detail::MemberAttribute<Self>::type synthesized{};
            self(stream, synthesized);
            return synthesized;
        }
    };

    template<typename Type>
    concept ParserConcept = std::derived_from<Type, ParserBase>;

    namespace detail
    {
        template<typename UnknownProjection>
        struct ProjectionSource : meta::Undefined {};

        template<typename Class, typename Field>
        struct ProjectionSource<Field Class::*>
        {
            using type = Class;
        };

        template<typename First, typename Second>
        struct SameOrUndefined : std::conditional<std::same_as<First, Second>, First, meta::UndefinedType> {};
    
        template<typename ...Projections>
        struct ProjectedParserAttribute
        {
            using Sources = meta::TypeList<typename ProjectionSource<Projections>::type...>;

            using type = meta::RightFold<SameOrUndefined, Sources>::type;
        };
    }

    template<typename Underlying, typename ...Projections>
    struct ProjectedParser : ParserBase
    {
        using Attribute = detail::ProjectedParserAttribute<Projections...>::type;

        Underlying parser;
        std::tuple<Projections...> projections;

        constexpr ProjectedParser(Underlying parser, Projections... projections) : parser(std::move(parser)), projections(std::move(projections)...) {}

        template<streams::Stream Stream, typename Attribute> requires (std::invocable<Projections, Attribute> && ...)
        void operator()(Stream&& stream, Attribute&& attribute) const
        {
            IO(stream, attribute, std::make_index_sequence<sizeof...(Projections)>{});
        }

    private:

        template<streams::Stream Stream, typename Attribute, std::size_t ...Indices>
        void IO(Stream&& stream, Attribute&& attribute, std::index_sequence<Indices...>) const
        {
            parser(stream, std::invoke(std::get<Indices>(projections), attribute)...);
        }
    };


    template<typename Type>
    concept MonomorphicParserConcept = ParserConcept<Type> && detail::HasMemberAttribute<Type>;

    template<typename Type>
    struct ParserAttribute : detail::MemberAttribute<Type> {};



    struct NoOpParser : ParserBase
    {
        void operator()(streams::Stream auto&&, auto&&... _) const
        {

        }
    };
    inline constexpr NoOpParser NoOp;



    template<typename Type>
    struct TrivialParser : ParserBase
    {
        using Attribute = Type;

        void operator()(streams::InputStream auto&& in, Type& value) const
        {
            value = in | streams::ReadTrivial<Type>;
        }


        void operator()(streams::OutputStream auto&& out, meta::SameBaseConcept<Type> auto const& value) const
        {
            out | streams::WriteTrivial[value];
        }
    };

    template<typename Type>
    inline constexpr TrivialParser<Type> Trivial;

    inline constexpr TrivialParser<int> Int;
    inline constexpr TrivialParser<myakish::Size> Size;
    inline constexpr TrivialParser<std::uint64_t> U64;

    struct DeducedTrivialParser : ParserBase
    {
        template<typename Value>
        void operator()(streams::InputStream auto&& in, Value&& value) const
        {
            value = in | streams::ReadTrivial<std::remove_cvref_t<Value>>;
        }

        void operator()(streams::OutputStream auto&& out, auto&& value) const
        {
            out | streams::WriteTrivial[value];
        }
    };
    inline constexpr DeducedTrivialParser DeducedTrivial;

    template<auto UnknownProjection>
    inline constexpr DeducedTrivialParser ProjectedTrivial;

    template<typename Class, typename Field, Field Class::* Projection>
    inline constexpr auto ProjectedTrivial<Projection> = Trivial<Field>[Projection];


    struct RawParser : ParserBase
    {       
        void operator()(streams::InputStream auto&& in, void* ptr, myakish::Size count) const
        {
            in.Read(myakish::AsBytePtr(ptr), count);
        }

        void operator()(streams::OutputStream auto&& out, const void* ptr, myakish::Size count) const
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


    namespace detail
    {
        template<ParserConcept First, ParserConcept Second>
        struct SequenceParserAttribute : meta::Undefined {};

        template<MonomorphicParserConcept First, MonomorphicParserConcept Second>
        struct SequenceParserAttribute<First, Second> : std::conditional<std::same_as<typename ParserAttribute<First>::type, typename ParserAttribute<Second>::type>, typename ParserAttribute<First>::type, meta::UndefinedType> {};
    }

    template<ParserConcept First, ParserConcept Second>
    struct SequenceParser : ParserBase
    {
        First f;
        Second s;

        using Attribute = detail::SequenceParserAttribute<First, Second>::type;

        constexpr SequenceParser(First f, Second s) : f(std::move(f)), s(std::move(s)) {}

        template<streams::Stream Stream, typename ...Attributes>
        void operator()(Stream&& stream, Attributes&&... attributes) const
        {
            f(stream, attributes...);
            s(stream, attributes...);
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

        void operator()(streams::AlignableStream auto&& stream, auto&&) const
        {
            stream | streams::Align[alignment];
        }
    };



    template<typename Type, ParserConcept Parser>
    struct RuleParser : ParserBase
    {
        using Attribute = Type;

        Parser parser;

        constexpr RuleParser(Parser parser) : parser(std::move(parser)) {}


        template<streams::Stream Stream, typename ArgAttribute>
        void operator()(Stream&& stream, ArgAttribute&& attribute) const
        {
            parser.IO(stream, attribute);
        }

    };

    template<typename Type>
    struct RuleFunctor : functional::ExtensionMethod
    {
        template<ParserConcept Parser>
        constexpr auto operator()(Parser parser) const
        {
            return RuleParser<Type, Parser>(std::move(parser));
        }
    };
    template<typename Type>
    inline constexpr RuleFunctor<Type> Rule;


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
            
            parsers | algebraic::Iterate[Func];
        }
    };

    template<typename Type>
    struct SequenceRuleFunctor : functional::ExtensionMethod
    {
        template<ParserConcept ...Parsers>
        constexpr auto operator()(Parsers... parsers) const
        {
            return SequenceRuleParser<Type, Parsers...>(std::move(parsers)...);
        }
    };
    template<typename Type>
    inline constexpr SequenceRuleFunctor<Type> SequenceRule;



    template<myakish::Size Bytes>
    struct Blob
    {
        std::array<std::byte, Bytes> storage;
    }; 


    template<ParserConcept... Parsers>
    struct VariantParser : ParserBase
    { 
        //using Attribute = algebraic::Variant<ParserAttribute<Parsers>...>;

        std::tuple<const Parsers&...> parsers;

        constexpr VariantParser(const Parsers&... parsers) : parsers(parsers...) {}
        constexpr VariantParser(std::tuple<Parsers...> parsers) : parsers(std::move(parsers)) {}

        template<streams::OutputStream Stream, typename ArgAttribute>
        void operator()(Stream&& out, ArgAttribute&& attribute) const
        {
            auto ParseWith = [&](auto&& parser)
                {
                    return [&](auto&& underlying) requires requires { parser(out, underlying); }
                        {
                            parser(out, underlying);
                        };
                };

            out | streams::WriteAs<myakish::Size>[algebraic::Index(attribute)];
            attribute | algebraic::FitVisit[parsers | algebraic::Map[ParseWith]];
        }

        template<streams::InputStream Stream, typename ArgAttribute>
        void operator()(Stream&& in, ArgAttribute&& attribute) const
        {
            auto ParseWith = [&](auto&& parser)
                {
                    return[&](auto&& underlying) requires requires { parser(in, underlying); }
                    {
                        parser(in, underlying);
                    };
                };

            auto index = in | streams::ReadTrivial<myakish::Size>;

            attribute = algebraic::Synthesize<ArgAttribute>(index);
            algebraic::FitVisit(attribute, parsers | algebraic::Map[ParseWith]);
            //attribute | algebraic::FitVisit[parsers | algebraic::Map[ParseWith]];
        }
    };

    template<MonomorphicParserConcept... Parsers>
    VariantParser(std::tuple<Parsers...> parsers) -> VariantParser<Parsers...>;

    
    
    template<ParserConcept Parser>
    struct RepeatParser : ParserBase
    {
        Parser parser;

        constexpr RepeatParser(Parser parser) : parser(std::move(parser)) {}

      
        template<streams::InputStream Stream, std::ranges::range AttributeRange>
        void operator()(Stream&& in, AttributeRange&& attribute) const
        {
            auto size = Size.Parse(in);

            auto Parse = [&](auto _)
                {
                    std::ranges::range_value_t<AttributeRange> synthesized{};
                    parser(in, synthesized);
                    return synthesized;
                };

            attribute = std::views::iota(0, size) | std::views::transform(Parse) | std::ranges::to<std::remove_cvref_t<AttributeRange>>();
        }

        template<streams::OutputStream Stream, std::ranges::range AttributeRange>
        void operator()(Stream&& out, AttributeRange&& attribute) const
        {
            out | streams::WriteAs<myakish::Size>[std::ranges::size(attribute)];

            std::ranges::for_each(attribute, functional::Invoke[parser, out, functional::Arg<0>]);
        }
    };

}