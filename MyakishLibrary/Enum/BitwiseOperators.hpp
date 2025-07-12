#pragma once
#include <utility>

#include <MyakishLibrary/Meta/Meta2.hpp>

namespace myakish::enums::operators
{

    template<meta2::EnumConcept EnumType>
    EnumType operator|(EnumType lhs, EnumType rhs)
    {
        return static_cast<EnumType>(std::to_underlying(lhs) | std::to_underlying(rhs));
    }

    template<meta2::EnumConcept EnumType>
    EnumType operator&(EnumType lhs, EnumType rhs)
    {
        return static_cast<EnumType>(std::to_underlying(lhs) & std::to_underlying(rhs));
    }

    template<meta2::EnumConcept EnumType>
    EnumType operator^(EnumType lhs, EnumType rhs)
    {
        return static_cast<EnumType>(std::to_underlying(lhs) ^ std::to_underlying(rhs));
    }

    template<meta2::EnumConcept EnumType>
    EnumType operator~(EnumType lhs)
    {
        return static_cast<EnumType>(~std::to_underlying(lhs));
    }

    template<meta2::EnumConcept EnumType>
    EnumType& operator|=(EnumType& lhs, EnumType rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    template<meta2::EnumConcept EnumType>
    EnumType& operator&=(EnumType& lhs, EnumType rhs)
    {
        lhs = lhs & rhs;
        return lhs;
    }

    template<meta2::EnumConcept EnumType>
    EnumType& operator^=(EnumType& lhs, EnumType rhs)
    {
        lhs = lhs ^ rhs;
        return lhs;
    }
}