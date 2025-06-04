#pragma once
#include <HvTree2/HvTree.h>

namespace hv::array
{
    using IndexType = myakish::Size;
    using SizeType = myakish::Size;

    struct ElementWrapper
    {
        IndexType index;
    };

    struct SizeWrapper
    {
        
    };

    template<typename Family>
    auto Resolve(hv::handle::FamilyTag<Family> tag, ElementWrapper wrapper)
    {
        return ElementHandle(tag, wrapper.index);
    }

    template<typename Family>
    auto Resolve(hv::handle::FamilyTag<Family> tag, SizeWrapper)
    {
        return SizeHandle(tag);
    }

    struct ArraySize
    {
        SizeType size;
    };

    template<handle::Handle Handle, data::Storage Storage>
    struct Array
    {
        using DescriptorType = Descriptor<Handle, Storage>;
        DescriptorType desc;

        Array(DescriptorType desc) : desc(desc)
        {

        }


        struct Iterator
        {
            IndexType index;
            DescriptorType desc;

            DescriptorType operator*() const
            {
                return desc[ElementWrapper{ index }];
            }

            Iterator& operator++()
            {
                index++;
                return *this;
            }
            Iterator& operator--()
            {
                index--;
                return *this;
            }

            Iterator operator++(int)
            {
                auto copy = *this;
                index++;
                return copy;
            }
            Iterator operator--(int)
            {
                auto copy = *this;
                index--;
                return copy;
            }

            using difference_type = myakish::Size;
            using value_type = DescriptorType;

            friend bool operator==(Iterator, Iterator) = default;
        };

        auto begin() const
        {
            return Iterator{ 0, desc };
        }

        auto end() const
        {
            return Iterator{ desc.Acquire<ArraySize>().size , desc };
        }
    };

    template<handle::Handle Handle, data::Storage Storage>
    struct SizedArray
    {
        using DescriptorType = Descriptor<Handle, Storage>;
        DescriptorType desc;
        SizeType size;

        SizedArray(DescriptorType desc, SizeType size) : desc(desc), size(size)
        {

        }

        struct Iterator
        {
            IndexType index;
            DescriptorType desc;

            DescriptorType operator*() const
            {
                return desc[ElementWrapper{ index }];
            }

            Iterator& operator++()
            {
                index++;
                return *this;
            }
            Iterator& operator--()
            {
                index--;
                return *this;
            }

            Iterator operator++(int)
            {
                auto copy = *this;
                index++;
                return copy;
            }
            Iterator operator--(int)
            {
                auto copy = *this;
                index--;
                return copy;
            }

            using difference_type = myakish::Size;
            using value_type = DescriptorType;

            friend bool operator==(Iterator, Iterator) = default;
        };

        auto begin() const
        {
            return Iterator{ 0, desc };
        }

        auto end() const
        {
            return Iterator{ size, desc };
        }
    };


    template<handle::Handle Handle, data::Storage Storage>
    Array(Descriptor<Handle, Storage>) -> Array<Handle, Storage>;

    template<handle::Handle Handle, data::Storage Storage>
    SizedArray(Descriptor<Handle, Storage>, SizeType) -> SizedArray<Handle, Storage>;

}

template<>
struct hv::conversion::EnableTrivialConversion< hv::array::ArraySize> : std::false_type {};

template<>
struct hv::conversion::HvToType<hv::array::ArraySize>
{
    static inline constexpr bool UseBinary = false;

    template<typename DescriptorType, typename ...Args>
    static hv::array::ArraySize Convert(DescriptorType desc, Args&&... args)
    {
        return { desc[hv::array::SizeWrapper{}].Acquire<hv::array::SizeType>() };
    }
};

template<>
struct hv::conversion::TypeToHv<hv::array::ArraySize>
{
    static inline constexpr bool UseBinary = false;

    template<typename DescriptorType>
    static void Convert(DescriptorType desc, hv::array::ArraySize size)
    {
        desc[hv::array::SizeWrapper{}] = size.size;
    }
};



namespace hv::literals
{
    constexpr hv::array::ElementWrapper operator ""_aa(unsigned long long index)
    {
        return { myakish::Size(index) };
    }
}

template<>
struct hv::handle::EnableWrapper<hv::array::ElementWrapper> : std::true_type {};

template<>
struct hv::handle::EnableWrapper<hv::array::SizeWrapper> : std::true_type {};
