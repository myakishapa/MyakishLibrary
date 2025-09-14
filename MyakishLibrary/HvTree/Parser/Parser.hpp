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
    template<typename Type, typename Descriptor>
    concept ParserFor = requires(const Type &parser, const Descriptor &desc, std::optional<std::string_view> type, std::string_view value)
    {
        parser.TryParse(desc, type, value);
    };

    struct ParseIntoFunctor : functional::ExtensionMethod
    {
        template<data::Storage StorageType, handle::HandleOf<typename StorageType::HandleFamily> Handle, ParserFor<Descriptor<StorageType, Handle>>... Parsers>
        void operator()(const Descriptor<StorageType, Handle>& desc, const ast::AST& source, const Parsers&... parsers) const
        {
            if (source.value) ((parsers.TryParse(desc, source.explicitType, *source.value)), ...);

            operator()(desc, source.entries, parsers...);
        }

        template<data::Storage StorageType, handle::HandleOf<typename StorageType::HandleFamily> Handle, ParserFor<Descriptor<StorageType, Handle>>... Parsers>
        void operator()(const Descriptor<StorageType, Handle>& desc, const ast::Entries& source, const Parsers&... parsers) const
        {
            for (auto&& [name, entry] : source)
                operator()(desc[handle::StringWrapper{ name }], entry, parsers...);
        }
    };
    inline constexpr ParseIntoFunctor ParseInto; 
}
