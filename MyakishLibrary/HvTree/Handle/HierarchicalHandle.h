#pragma once
#include <MyakishLibrary/HvTree/HvTree.h>
#include <MyakishLibrary/HvTree/Array/Array.h>

#include <array>
#include <compare>

namespace myakish::tree::handle
{
    namespace hierarchical
    {
        template<meta::TriviallyCopyable Word>
        struct Family {};
        
        template<meta::TriviallyCopyable Word, Size Size>
        struct Static
        {
            std::array<Word, Size> data;

            Static() = default;

            template<std::convertible_to<Word> ...Words> requires(sizeof...(Words) == Size)
            Static(Words ...words) : data{ static_cast<Word>(words)... } {}

            template<myakish::Size RhsSize>
            auto operator/(Static<Word, RhsSize> rhs) const -> Static<Word, Size + RhsSize>
            {
                Static<Word, Size + RhsSize> result{};
                std::memcpy(result.data.data(), data.data(), sizeof(Word) * Size);
                std::memcpy(result.data.data() + Size, rhs.data.data(), sizeof(Word) * RhsSize);
                return result;
            }

            std::strong_ordering operator<=>(Static rhs) const
            {
                return data <=> rhs.data;
            }

            template<myakish::Size RhsSize>
            std::strong_ordering operator<=>(Static<Word, RhsSize> rhs) const
            {
                return Size <=> RhsSize;
            }

            operator Word() const requires(Size == 1)
            {
                return data[0];
            }
        };

        template<myakish::meta::TriviallyCopyable Word, myakish::Size Capacity>
        struct FixedCapacity
        {
            std::array<Word, Capacity> data;
            myakish::Size size;

            FixedCapacity() : size(0), data{} {}

            template<std::convertible_to<Word> ...Words>requires(sizeof...(Words) <= Capacity)
            FixedCapacity(Words ...words) : size(sizeof...(Words)), data{ static_cast<Word>(words)... } {}

            template<myakish::Size SourceSize> requires(SourceSize <= Capacity)
            FixedCapacity(Static<Word, SourceSize> staticHandle) : size(SourceSize)
            {
                std::memcpy(data.data(), staticHandle.data.data(), sizeof(Word) * SourceSize);
            }

            template<myakish::Size SourceCapacity>
            FixedCapacity(FixedCapacity<Word, SourceCapacity> rhs) : size(rhs.size)
            {
                std::memcpy(data.data(), rhs.data.data(), sizeof(Word) * size);
            }

            FixedCapacity operator/(FixedCapacity rhs) const
            {
                FixedCapacity result;
                std::memcpy(result.data.data(), data.data(), sizeof(Word) * size);
                std::memcpy(result.data.data() + size, rhs.data.data(), sizeof(Word) * rhs.size);
                result.size = size + rhs.size;
                return result;
            }


            std::strong_ordering operator<=>(FixedCapacity rhs) const
            {
                if (size != rhs.size) return size <=> rhs.size;
                return data <=> rhs.data;
            }

            bool operator==(FixedCapacity rhs) const
            {
                return size == rhs.size && data == rhs.data;
            }

            /*template<myakish::Size RhsSize>
            std::strong_ordering operator<=>(Static<Word, RhsSize> rhs)
            {
                return Size <=> RhsSize;
            }*/
        }; 

        /*template<myakish::meta::TriviallyCopyable Word>
        auto ElementHandle(hv::handle::FamilyTag<Family<Word>> tag, hv::array::IndexType index)
        {
            return Static<Word, 1>{ index };
        }

        template<myakish::meta::TriviallyCopyable Word>
        auto SizeHandle(hv::handle::FamilyTag<Family<Word>> tag)
        {
            return Static<Word, 1>{ hc::Hash("size") };
        }*/
    }

    template<myakish::meta::TriviallyCopyable Word, myakish::Size Size>
    struct FamilyTraits<hierarchical::Static<Word, Size>>
    {
        using Family = hierarchical::Family<Word>;
    };

    template<myakish::meta::TriviallyCopyable Word, myakish::Size Capacity>
    struct FamilyTraits<hierarchical::FixedCapacity<Word, Capacity>>
    {
        using Family = hierarchical::Family<Word>;
    };
}