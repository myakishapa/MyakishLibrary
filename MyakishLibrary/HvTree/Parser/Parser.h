#pragma once

#include <MyakishLibrary/HvTree/HvTree.h>

#include <MyakishLibrary/HvTree/Parser/Spirit.h>
#include <MyakishLibrary/HvTree/Handle/CombinedHashHandle.h>

#include <map>
#include <ranges>
#include <set>
#include <string>
#include <variant>
#include <vector>


namespace myakish::tree::parse
{
    /*using SingleData = std::string_view;
    using ArrayData = std::span<const std::string>;

    template<DataStorage DataType>
    struct Converter
    {
        virtual void Convert(Descriptor<DataType> dst, SingleData data) const = 0;
        virtual void ConvertArray(Descriptor<DataType> dst, ArrayData data) const = 0;
    };

    template<DataStorage DataType, typename HandleConverter>
    class Parser
    {
    public:

        using Handle = DataType::Handle;
        using Type = spirit::ObjectParser::Type;

        using ConverterRef = const Converter<DataType>&;

    private:

        std::map<Type, ConverterRef> converters;

    public:

        template<std::ranges::input_range Range>
        void Parse(Descriptor<DataType> dst, Range&& src)
        {
            auto obj = spirit::ObjectParser::Parse(std::forward<Range>(src));
            Parse(dst, obj);
        }

        void Parse(Descriptor<DataType> dst, const spirit::ObjectParser::Object& obj)
        {
            for (const auto& [id, entry] : obj.singleEntries)
            {
                auto& conv = converters.at(entry.type);
                conv.Convert(dst[HandleConverter::Convert(id)], entry.data);
            }

            for (const auto& [id, entry] : obj.arrayEntries)
            {
                auto& conv = converters.at(entry.type);
                conv.ConvertArray(dst[HandleConverter::Convert(id)], std::span(entry.data));
            }

            for (const auto& [id, entry] : obj.children)
            {
                Parse(dst[HandleConverter::Convert(id)], entry);
            }

            for (const auto& [id, entry] : obj.objectArrayEntries)
            {
                auto subobject = dst[HandleConverter::Convert(id)];
                myakish::Size size = entry.data.size();
                for (myakish::Size i = 0; i < size; i++)
                {
                    Parse(subobject[ArrayAccessor{ i }], entry.data[i]);
                }
                subobject[ArraySize] = size;
            }
        }

        void AddConverter(Type type, ConverterRef ref)
        {
            converters.emplace(type, ref);
        }
    };

    struct HashHandleConverter
    {
        static handle::CombinedHash Convert(std::string_view str)
        {
            return handle::CombinedHash{ hc::Hash(str) };
        }
    };*/
}
