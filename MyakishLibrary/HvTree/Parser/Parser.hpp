#pragma once

#include <MyakishLibrary/HvTree/HvTree.hpp>

#include <MyakishLibrary/HvTree/Parser/Spirit.hpp>

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

#include <map>
#include <ranges>
#include <set>
#include <string>
#include <variant>
#include <vector>


namespace myakish::tree::parse
{
    template<typename Type>
    concept Parser = requires(Type parser, streams::OutputStreamArchetype out, std::string_view value, std::optional<std::string_view> explicitType)
    {
        { parser(out, value, explicitType) } -> std::same_as<bool>;
    };

    template<Parser First, Parser Second>
    struct ChainParser
    {
        const First& first;
        const Second& second;

        ChainParser(const First& first, const Second& second) : first(first), second(second) {}

        bool operator()(streams::OutputStream auto&& out, std::string_view value, std::optional<std::string_view> explicitType) const
        {
            return first(out, value, explicitType) || second(out, value, explicitType);
        }
    };

    inline constexpr auto Chain = functional::DeduceConstruct<ChainParser>;

    template<typename Parser>
    struct EntriesSource;

    template<typename Parser>
    struct ASTSource
    {
        using HandleType = std::string;

        const ast::AST& ast;
        const Parser& parser;

        ASTSource(const ast::AST& ast, const Parser& parser) : ast(ast), parser(parser) {}

        template<HandleConcept Handle>
        ChildrenBuilder BuildChildren(Storage<Handle>& storage) const
        {
            return EntriesSource(ast.entries, parser).BuildChildren(storage);
        }

        void WriteData(streams::OutputStream auto&& out) const
        {
            if (ast.value) parser(out, *ast.value, ast.explicitType.transform(functional::Construct<std::string_view>));
        }
    };

    template<typename Parser>
    struct EntriesSource
    {
        using HandleType = std::string;

        const ast::Entries& entries;
        const Parser& parser;

        EntriesSource(const ast::Entries& entries, const Parser& parser) : entries(entries), parser(parser) {}

        template<HandleConcept Handle>
        ChildrenBuilder BuildChildren(Storage<Handle>& storage) const
        {
            for (auto&& [handle, ast] : entries)
            {
                co_yield handle;

                Build(storage, ASTSource(ast, parser) % HandleSource(handle));
            }
        }
    };


    struct IntParserType : functional::ExtensionMethod
    {
        void operator()(streams::OutputStream auto&& out, std::string_view value, std::optional<std::string_view> type) const
        {
            if (type && *type != "int") return;

            if (auto result = boost::parser::parse(value, boost::parser::int_)) out | streams::WriteTrivial[*result];
        }
    };
    inline constexpr IntParserType IntParser;

}
