#pragma once
#include <MyakishLibrary/HvTree/HvTree.h>

namespace myakish::tree::handle
{
    struct CombinedHash
    {
        std::uint64_t handle;

        constexpr static CombinedHash Combine(CombinedHash lhs, CombinedHash rhs)
        {
            return CombinedHash(HashCombine(lhs.handle, rhs.handle));
        }
        constexpr static CombinedHash ArrayIndex(myakish::Size index)
        {
            return CombinedHash(index);
        }
        constexpr static CombinedHash ArraySize()
        {
            return CombinedHash(Hash("ArraySize"));
        }

        constexpr friend auto operator<=>(CombinedHash, CombinedHash) = default;
    };

    //static_assert(ArrayDataHandle<CombinedHash>, "hv::CombinedHash should satisfy hv::ArrayDataHandle");
    static_assert(myakish::meta::TriviallyCopyable<CombinedHash>, "hv::CombinedHash should satisfy TriviallyCopyable");
}