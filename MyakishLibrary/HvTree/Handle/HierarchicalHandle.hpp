#pragma once
#include <MyakishLibrary/HvTree/HvTree.hpp>
#include <MyakishLibrary/HvTree/Array/Array.hpp>

#include <array>
#include <compare>

namespace myakish::tree::handle
{
    namespace hierarchical
    {
        template<typename Word>
        struct Family {};
        
        template<typename Word, Size Size>
        struct Static
        {
            std::array<Word, Size> data;

            Static() = default;

            template<std::convertible_to<Word> ...Words> requires(sizeof...(Words) == Size)
            Static(Words ...words) : data{ static_cast<Word>(words)... } {}

            template<myakish::Size SourceSize> requires(SourceSize <= Size)
                Static(Static<Word, SourceSize> staticHandle)
            {
                std::ranges::copy(staticHandle.data, data.begin());

                // std::memcpy(data.data(), staticHandle.data.data(), sizeof(Word) * SourceSize);
            }

            template<myakish::Size RhsSize>
            auto operator/(Static<Word, RhsSize> rhs) const -> Static<Word, Size + RhsSize>
            {
                Static<Word, Size + RhsSize> result(*this);

                std::ranges::copy(rhs.data, result.data.begin() + Size);

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

        template<typename Word, myakish::Size Capacity>
        struct FixedCapacity
        {
            std::array<Word, Capacity> data;
            myakish::Size size;

            FixedCapacity() : size(0), data{} {}

            template<std::convertible_to<Word> ...Words> requires(sizeof...(Words) <= Capacity)
            FixedCapacity(Words ...words) : size(sizeof...(Words)), data{ static_cast<Word>(words)... } {}

            template<myakish::Size SourceSize> requires(SourceSize <= Capacity)
            FixedCapacity(Static<Word, SourceSize> staticHandle) : size(SourceSize)
            {
                std::ranges::copy(staticHandle.data, data.begin());
                
                // std::memcpy(data.data(), staticHandle.data.data(), sizeof(Word) * SourceSize);
            }

            template<myakish::Size SourceCapacity>
            FixedCapacity(FixedCapacity<Word, SourceCapacity> rhs) : size(rhs.size)
            {
                std::ranges::copy(rhs.data | std::views::take(rhs.size), data.begin());

                //std::memcpy(data.data(), rhs.data.data(), sizeof(Word) * size);
            }

            FixedCapacity operator/(FixedCapacity rhs) const
            {
                FixedCapacity result(*this);
                
                std::ranges::copy(rhs.data | std::views::take(rhs.size), result.data.begin() + size);

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

        template<std::integral Word>
        Static<Word, 1> ArrayIndex(Family<Word>, Size index)
        {
            return { index };
        }

        Static<std::string, 1> ArrayIndex(Family<std::string>, Size index)
        {
            return { std::to_string(index) };
        }


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

    template<typename Word, myakish::Size Size>
    struct FamilyTraits<hierarchical::Static<Word, Size>>
    {
        using Family = hierarchical::Family<Word>;
    };

    template<typename Word, myakish::Size Capacity>
    struct FamilyTraits<hierarchical::FixedCapacity<Word, Capacity>>
    {
        using Family = hierarchical::Family<Word>;
    };
}