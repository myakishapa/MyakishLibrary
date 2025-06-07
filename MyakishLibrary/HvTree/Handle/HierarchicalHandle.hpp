#pragma once
#include <MyakishLibrary/HvTree/HvTree.hpp>
#include <MyakishLibrary/HvTree/Array/Array.hpp>

#include <array>
#include <vector>
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

            template<myakish::Size RhsSize>
            auto operator/(const Static<Word, RhsSize> &rhs) const -> Static<Word, Size + RhsSize>
            {
                Static<Word, Size + RhsSize> result;

                std::ranges::copy(data, result.data.begin());
                std::ranges::copy(rhs.data, result.data.begin() + Size);

                return result;
            }

            std::strong_ordering operator<=>(const Static &rhs) const
            {
                return data <=> rhs.data;
            }

            bool operator==(const Static& rhs) const
            {
                return data == rhs.data;
            }

            /*template<myakish::Size RhsSize>
            std::strong_ordering operator<=>(Static<Word, RhsSize> rhs) const
            {
                return Size <=> RhsSize;
            }*/

            operator Word() const requires(Size == 1)
            {
                return data[0];
            }
        };

        template<typename Word>
        struct Dynamic
        {
            std::vector<Word> data;

            Dynamic() : data{} {}

            template<std::convertible_to<Word> ...Words>
            Dynamic(Words ...words) : data{ static_cast<Word>(words)... } {}

            template<myakish::Size SourceSize>
            Dynamic(const Static<Word, SourceSize> &staticHandle) : data(std::from_range, staticHandle.data) {}

            Size Length() const
            {
                return data.size();
            }

            Dynamic operator/(const Dynamic &rhs) const
            {
                Dynamic result(*this);
                
                result.data.append_range(rhs.data);

                return result;
            }


            std::strong_ordering operator<=>(const Dynamic& rhs) const
            {
                return std::lexicographical_compare_three_way(data.begin(), data.end(), rhs.data.begin(), rhs.data.end());

                if (Length() != rhs.Length()) return Length() <=> rhs.Length();
                return data <=> rhs.data;
            }

            bool operator==(const Dynamic& rhs) const
            {
                return Length() == rhs.Length() && data == rhs.data;
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

        template<std::integral Word, Size StaticSize>
        auto NextADL(Static<Word, StaticSize> arg)
        {
            arg.data[arg.data.size() - 1]++;
            return arg;
        }

        template<std::integral Word>
        auto NextADL(Dynamic<Word> arg)
        {
            arg.data[arg.data.size() - 1]++;
            return arg;
        }

        template<Size StaticSize>
        auto NextADL(Static<std::string, StaticSize> arg)
        {
            arg.data[arg.data.size() - 1] += (char)1;
            return arg;
        }

        auto NextADL(Dynamic<std::string> arg)
        {
            arg.data[arg.data.size() - 1] += (char)1;
            return arg;
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

    template<typename Word>
    struct FamilyTraits<hierarchical::Dynamic<Word>>
    {
        using Family = hierarchical::Family<Word>;
    };
}