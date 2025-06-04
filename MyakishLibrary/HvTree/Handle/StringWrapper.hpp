#pragma once
#include <MyakishLibrary/HvTree/HvTree.hpp>
#include <MyakishLibrary/HvTree/Handle/CombinedHashHandle.hpp>
#include <MyakishLibrary/HvTree/Handle/HierarchicalHandle.hpp>
#include <MyakishLibrary/HvTree/Handle/WrapperComposition.hpp>

namespace myakish::tree::handle
{
    struct StringWrapper
    {
        std::string_view str;

        constexpr operator CombinedHash() const
        {
            return CombinedHash{ Hash(str) };
        }

        operator hierarchical::Static<std::uint64_t, 1>() const
        {
            return Hash(str);
        }

        /*template<myakish::Size Capacity>
        operator hierarchical::FixedCapacity<std::uint64_t, Capacity>() const
        {
            return { hc::Hash(str) };
        }*/
    };

    template<>
    struct EnableWrapper<StringWrapper> : std::true_type {};

    hierarchical::Static<std::uint64_t, 1> Resolve(FamilyTag<hierarchical::Family<std::uint64_t>>, StringWrapper wrapper)
    {
        return wrapper;
    }

    hierarchical::Static<std::string_view, 1> Resolve(FamilyTag<hierarchical::Family<std::string_view>>, StringWrapper wrapper)
    {
        return wrapper.str;
    }

    /*WrapperComposition<StringWrapper, StringWrapper> operator/(StringWrapper lhs, StringWrapper rhs)
    {
        return { { lhs, rhs } };
    }*/
}

namespace myakish::tree::literals
{
    constexpr handle::StringWrapper operator ""_sk(const char* str, std::size_t count)
    {
        return handle::StringWrapper{ std::string_view(str, count) };
    }
}