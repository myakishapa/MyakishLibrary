#pragma once

//#define BOOST_SPIRIT_X3_DEBUG
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/include/io.hpp>

#include <string>
#include <iostream>
#include <ranges>
#include <map>
#include <vector>

namespace hv::parse::spirit
{
    namespace x3 = boost::spirit::x3;
    
    namespace pqs
    {
        x3::rule<class str, std::string> rule = "pqs";

        auto push = [](auto& ctx)
            {
                x3::_val(ctx) += x3::_attr(ctx);
            };

        auto const quoted_string =
            x3::lexeme['"' > *(x3::char_[push] - '"') > '"'];

        auto const unquoted_string =
            x3::lexeme[+(x3::alnum[push])];

        auto const rule_def =
            quoted_string | unquoted_string;

        BOOST_SPIRIT_DEFINE(rule);
    }

    namespace ObjectParser
    {
        using Type = std::string;
        using ID = std::string;
        using DataStorage = std::string;

        struct Object;

        struct SingleEntry
        {
            Type type;
            DataStorage data;
        };

        struct ArrayEntry
        {
            Type type;
            std::vector<DataStorage> data;
        };

        struct ObjectArrayEntry
        {
            std::vector<Object> data;
        };

        struct Object
        {
            ID currentId;

            std::map<ID, Object> children;
            std::map<ID, ObjectArrayEntry> objectArrayEntries;

            std::map<ID, SingleEntry> singleEntries;
            std::map<ID, ArrayEntry> arrayEntries;
        };

        std::ostream& operator<<(std::ostream& out, const Object& obj)
        {
            std::print(out, "Object ({}, {}, {})", obj.children.size(), obj.singleEntries.size(), obj.arrayEntries.size());
            return out;
        }

        std::ostream& operator<<(std::ostream& out, const SingleEntry& entry)
        {
            std::print(out, "Single Entry ({}, {})", entry.type, entry.data);
            return out;
        }

        std::ostream& operator<<(std::ostream& out, const ArrayEntry& entry)
        {
            std::print(out, "Array Entry ({}, {})", entry.type, entry.data);
            return out;
        }

        auto setType = [](auto& ctx)
            {
                x3::_val(ctx).type = x3::_attr(ctx);
            };

        auto setData = [](auto& ctx)
            {
                x3::_val(ctx).data = x3::_attr(ctx);
            };
        auto pushData = [](auto& ctx)
            {
                x3::_val(ctx).data.push_back(x3::_attr(ctx));
            };

        x3::rule<struct objectParser, Object> rule = "Object";

        x3::rule<struct objectArrayParser, ObjectArrayEntry> objectArrayEntryRule = "Object Array Entry";

        x3::rule<struct singleEntryParser, SingleEntry> singleEntryRule = "Single Entry";

        x3::rule<struct arrayEntryParser, ArrayEntry> arrayEntryRule = "Array Entry";

        auto const TypeParser = pqs::rule[setType];

        auto const singleEntryRule_def = TypeParser >> ':' >> pqs::rule[setData];
        auto const arrayEntryRule_def = TypeParser >> x3::char_(':') >> '[' >> (pqs::rule[pushData] % ',') >> ']';

        auto pushSingle = [](auto& ctx)
            {
                auto&& val = x3::_val(ctx);
                val.singleEntries[val.currentId] = x3::_attr(ctx);
            };
        auto pushArray = [](auto& ctx)
            {
                auto&& val = x3::_val(ctx);
                val.arrayEntries[val.currentId] = x3::_attr(ctx);
            };
        auto pushObject = [](auto& ctx)
            {
                auto&& val = x3::_val(ctx);
                val.children[val.currentId] = x3::_attr(ctx);
            };
        auto pushObjectArray = [](auto& ctx)
            {
                auto&& val = x3::_val(ctx);
                val.objectArrayEntries[val.currentId] = x3::_attr(ctx);
            };

        auto setCurrentId = [](auto& ctx)
            {
                x3::_val(ctx).currentId = x3::_attr(ctx);
            };

        auto const currentIdParser = pqs::rule[setCurrentId];

        auto const wrappedObject = x3::char_('{') >> rule[pushObject] >> x3::char_('}');

        auto const rule_def = 
            ((currentIdParser >> x3::char_(':') >> (wrappedObject | singleEntryRule[pushSingle] | arrayEntryRule[pushArray] | objectArrayEntryRule[pushObjectArray])) % x3::char_(','));

        auto pushObjectToArray = [](auto& ctx)
            {
                auto&& val = x3::_val(ctx);
                val.data.push_back(x3::_attr(ctx));
            };

        auto const objectArrayEntryRule_def =
            x3::char_('[') >> ((x3::char_('{') >> rule[pushObjectToArray] >> x3::char_('}')) % x3::char_(',')) >> x3::char_(']');


        BOOST_SPIRIT_DEFINE(rule, singleEntryRule, arrayEntryRule, objectArrayEntryRule);

        template<std::ranges::input_range Range>
        Object Parse(Range&& r)
        {
            Object obj;

            auto result = x3::phrase_parse(std::ranges::begin(r), std::ranges::end(r), ObjectParser::rule, x3::space, obj);

            return obj;
        }
    }
}