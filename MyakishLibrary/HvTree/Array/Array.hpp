#pragma once
#include <MyakishLibrary/HvTree/HvTree.hpp>

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

#include <ranges>
#include <numeric>

namespace myakish::tree::array
{
    struct IndexingUndefined
    {

    };

    template<typename Family>
    IndexingUndefined ArrayIndex(Family, Size index)
    {
        return {};
    }

    namespace detail
    {
        template<typename Family>
        struct MakeArrayIndexerFunctor : functional::ExtensionMethod
        {
            auto operator()(Size index) const
            {
                return ArrayIndex(Family{}, index);
            }
        };
    }
    template<typename Family>
    inline constexpr detail::MakeArrayIndexerFunctor<Family> MakeArrayIndex;

    template<typename Family>
    inline constexpr auto Indices = std::views::iota(Size{ 0 }, std::numeric_limits<Size>::max()) | std::views::transform(MakeArrayIndex<Family>);
    
    namespace detail
    {
        struct InfiniteFunctor : functional::ExtensionMethod
        {
            template<data::Storage StorageType, handle::HandleOf<typename StorageType::HandleFamily> Handle>
            auto operator()(const Descriptor<StorageType, Handle>& descriptor) const
            {
                return Indices<typename StorageType::HandleFamily>
                    | std::views::transform([=](auto index) { return descriptor[index]; });
            }
        };
    }
    inline constexpr detail::InfiniteFunctor Infinite;

    namespace detail
    {
        struct RangeFunctor : functional::ExtensionMethod
        {
            template<data::Storage StorageType, handle::HandleOf<typename StorageType::HandleFamily> Handle>
            auto operator()(const Descriptor<StorageType, Handle>& descriptor, Size begin, Size count = std::numeric_limits<Size>::max()) const
            {
                return Infinite(descriptor) | std::views::drop(begin) | std::views::take(count);
            }
        };
    }
    inline constexpr detail::RangeFunctor Range;

    namespace detail
    {
        struct ExistingFunctor : functional::ExtensionMethod
        {
            template<data::Storage StorageType, handle::HandleOf<typename StorageType::HandleFamily> Handle>
            auto operator()(const Descriptor<StorageType, Handle>& descriptor) const
            {
                return Infinite(descriptor)
                    | std::views::take_while([](auto desc) { return desc.Exists(); });
            }
        };
    }
    inline constexpr detail::ExistingFunctor Existing;
}