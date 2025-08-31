#pragma once

#include <MyakishLibrary/HvTree/HvTree.hpp>

#include <MyakishLibrary/HvTree/Parser/Spirit.hpp>
#include <MyakishLibrary/HvTree/Handle/StringWrapper.hpp>

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
    concept Parser = requires(Type parser, std::string_view type, std::string_view value)
    {
        { parser.MatchType(type) } -> std::convertible_to<bool>;
        { parser.MatchValue(value) } -> std::convertible_to<bool>;
    };

    struct ParseIntoFunctor : functional::ExtensionMethod
    {
        template<data::Storage StorageType, handle::HandleOf<typename StorageType::HandleFamily> Handle, Parser... Parsers>
        void operator()(const Descriptor<StorageType, Handle>& desc, const ast::AST& source, const Parsers&... parsers) const
        {
            auto TryParse = [&](Parser auto&& parser)
                {
                    if (source.explicitType)
                    {
                        if (parser.MatchType(*source.explicitType)) parser.ParseInto(desc, *source.explicitType, *source.value);
                    }
                    else
                    {
                        if (parser.MatchValue(*source.value)) parser.ParseInto(desc, *source.value);
                    }
                };

            if (source.value) (TryParse(parsers), ...);

            operator()(desc, source.entries, parsers...);
        }

        template<data::Storage StorageType, handle::HandleOf<typename StorageType::HandleFamily> Handle, Parser... Parsers>
        void operator()(const Descriptor<StorageType, Handle>& desc, const ast::Entries& source, const Parsers&... parsers) const
        {
            for (auto&& [name, entry] : source)
                operator()(desc[handle::StringWrapper{ name }], entry, parsers...);
        }
    };
    inline constexpr ParseIntoFunctor ParseInto; 
}
