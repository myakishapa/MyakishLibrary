#pragma once

#include <MyakishLibrary/HvTree/Parser/Parser.hpp>
#include <MyakishLibrary/HvTree/Conversion/Conversion.hpp>

namespace myakish::tree::parse
{
    struct IntParserType
    {
        void TryParse(const auto& desc, std::optional<std::string_view> type, std::string_view value) const
        {
            if (type && *type != "int") return;
                      
            if (auto result = boost::parser::parse(value, boost::parser::int_)) desc = *result;
        }
    };
    inline constexpr IntParserType IntParser;
}