#pragma once
#include <utility>

#include <MyakishLibrary/Meta/Concepts.hpp>

namespace myakish::enums::operators
{

    template<meta::Enum EnumType>
    EnumType operator|(EnumType lhs, EnumType rhs)
    {
        return static_cast<EnumType>(std::to_underlying(lhs) | std::to_underlying(rhs));
    }

    template<meta::Enum EnumType>
    EnumType operator&(EnumType lhs, EnumType rhs)
    {
        return static_cast<EnumType>(std::to_underlying(lhs) & std::to_underlying(rhs));
    }

    template<meta::Enum EnumType>
    EnumType operator^(EnumType lhs, EnumType rhs)
    {
        return static_cast<EnumType>(std::to_underlying(lhs) ^ std::to_underlying(rhs));
    }

    template<meta::Enum EnumType>
    EnumType operator~(EnumType lhs)
    {
        return static_cast<EnumType>(~std::to_underlying(lhs));
    }

    template<meta::Enum EnumType>
    EnumType& operator|=(EnumType& lhs, EnumType rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    template<meta::Enum EnumType>
    EnumType& operator&=(EnumType& lhs, EnumType rhs)
    {
        lhs = lhs & rhs;
        return lhs;
    }

    template<meta::Enum EnumType>
    EnumType& operator^=(EnumType& lhs, EnumType rhs)
    {
        lhs = lhs ^ rhs;
        return lhs;
    }
}