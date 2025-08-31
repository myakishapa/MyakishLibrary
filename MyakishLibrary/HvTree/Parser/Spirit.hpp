#pragma once

#define NOMINMAX

//#define BOOST_SPIRIT_X3_DEBUG
#include <boost/parser/parser.hpp>

#include <string>
#include <iostream>
#include <ranges>
#include <map>
#include <vector>
#include <print>
#include <format>

#include <MyakishLibrary/Meta.hpp>

namespace myakish::tree::parse
{
    namespace grammar
    {
        namespace bp = boost::parser;

        using namespace bp::literals;

        template<auto Proj>
        inline constexpr auto SetProjected = [](auto& ctx)
            {
                std::invoke(Proj, _val(ctx)) = _attr(ctx);
            };
        template<auto Proj>
        inline constexpr auto SetProjectedTo = [](auto val)
            {
                return [=](auto& ctx)
                    {
                        std::invoke(Proj, _val(ctx)) = val;
                    };
            };
        template<auto Proj>
        inline constexpr auto PushProjected = [](auto& ctx)
            {
                std::invoke(Proj, _val(ctx)).push_back(_attr(ctx));
            };

        inline constexpr auto Set = SetProjected < std::identity{} > ;

        inline constexpr auto StringParser = [](auto separator)
            {
                return bp::lexeme[*(bp::char_ - separator)];
            };

        inline constexpr auto NonEmptyStringParser = [](auto separator)
            {
                return bp::lexeme[+(bp::char_ - separator)];
            };


        struct Line
        {
            int nestingLevel;
            std::string name;
            std::optional<std::string> explicitType = std::nullopt;
            std::optional<std::string> value = std::nullopt;
        };

        bp::rule<struct LineParserTag, Line> LineParser = "line";

        const auto IdentationUnitParser = "\t"_l | "    "_l;

        bp::rule<struct IdentationParserTag, int> IdentationParser = "identation";

        inline constexpr auto Increment = [](auto& ctx)
            {
                _val(ctx)++;
            };



        const auto IdentationParser_def = bp::lexeme[*(IdentationUnitParser[Increment])];

        const auto TypeStartParser = ":"_l;

        const auto ValueStartParser = ">>"_l;

        const auto TypeParser = TypeStartParser >> (bp::quoted_string | NonEmptyStringParser(bp::ws | ValueStartParser))[SetProjected<&Line::explicitType>];

        const auto ValueParser = ValueStartParser >> StringParser(bp::eol)[SetProjected<&Line::value>];

        const auto LineValuesParser = (bp::quoted_string | NonEmptyStringParser(bp::ws | TypeStartParser | ValueStartParser))[SetProjected<&Line::name>] >>
            -TypeParser >>
            -ValueParser;

        const auto LineParser_def =
            IdentationParser[SetProjected<&Line::nestingLevel>] >>
            bp::skip(bp::ws - bp::eol)[LineValuesParser] >>
            -bp::eol;

        BOOST_PARSER_DEFINE_RULES(LineParser, IdentationParser);

        using File = std::vector<Line>;

        const auto FileParser = *LineParser;

        std::optional<File> Parse(bp::parsable_range auto&& range, bp::trace trace = bp::trace::off)
        {
            return bp::parse(range, FileParser, trace);
        }
    }

    namespace ast
    {
        struct AST;

        using Entries = std::map<std::string, AST>;

        struct AST
        {
            std::optional<std::string> value;
            std::optional<std::string> explicitType;

            Entries entries;
        };

        Entries Parse(auto& begin, const auto& end, int currentNestingLevel = 0)
        {
            Entries result;

            Entries::iterator last{};

            while (begin != end)
            {
                grammar::Line line = *begin;

                if (line.nestingLevel > currentNestingLevel)
                {
                    last->second.entries.insert_range(Parse(begin, end, currentNestingLevel + 1));
                }
                else if (line.nestingLevel < currentNestingLevel) return result;
                else
                {              
                    AST ast{};

                    ast.value = line.value;
                    ast.explicitType = line.explicitType;

                    auto [it, inserted] = result.emplace(std::move(line).name, std::move(ast));
                    last = it;

                    begin++;
                }
            }

            return result;
        }

        namespace detail
        {
            template<typename Type>
            std::string FormatOptional(const std::optional<Type>& opt)
            {
                return opt.has_value() ? std::format("{}", opt.value()) : std::string("nullopt");
            }
        }

        void DebugPrint(Entries entries, std::string prefix = "")
        {
            for (auto&& [name, entry] : entries)
            {
                std::print("{}{}", prefix, name);
                if (entry.explicitType) std::print(": {}", *entry.explicitType);
                if (entry.value) std::print(" >> {}", *entry.value);
                
                std::println();

                DebugPrint(entry.entries, prefix + "    ");
            }
        }

        Entries Parse(meta::RangeOfConcept<grammar::Line> auto&& range)
        {
            auto begin = std::ranges::begin(range);
            return Parse(begin, std::ranges::end(range), 0);
        }

        Entries Parse(boost::parser::parsable_range auto&& range)
        {
            return Parse(grammar::Parse(range).value());
        }
    }
}