#pragma once

#include <MyakishLibrary/HvTree/Parser/Parser.hpp>
#include <MyakishLibrary/HvTree/Conversion/Conversion.hpp>

namespace myakish::tree::parse
{
    struct IntParserType
    {
        bool MatchType(std::string_view type) const
        {
            return type == "int";
        }
        bool MatchValue(std::string_view value) const
        {
            return boost::parser::parse(value, boost::parser::int_).has_value();
        }

        void ParseInto(const auto& desc, std::string_view value) const
        {
            desc = *boost::parser::parse(value, boost::parser::int_);
        }
        void ParseInto(const auto& desc, std::string_view type, std::string_view value) const
        {
            desc = *boost::parser::parse(value, boost::parser::int_);
        }
    };
    inline constexpr IntParserType IntParser;
}