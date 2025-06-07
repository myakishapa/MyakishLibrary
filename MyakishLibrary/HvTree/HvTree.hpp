#pragma once

#include <MyakishLibrary/Utility.hpp>

#include <MyakishLibrary/Streams/Common.hpp>
#include <MyakishLibrary/Streams/Concepts.hpp>

#include <MyakishLibrary/Functional/Pipeline.hpp>

#include <memory>
#include <map>
#include <string_view>

namespace myakish::tree
{
    namespace conversion
    {
        struct Undefined {};

        template<typename Type>
        struct HvToType
        {
            static inline constexpr bool UseBinary = true;

            template<myakish::streams::InputStream Stream, typename ...Args>
            static Undefined Convert(Stream&& in, Args&&... args)
            {
                return {};
            }

            template<typename DescriptorType, typename ...Args>
            static Undefined Convert(DescriptorType desc, Args&&... args)
            {
                return {};
            }
        };
        template<typename Type>
        struct TypeToHv
        {
            static inline constexpr bool UseBinary = true;

            template<myakish::streams::OutputStream Stream, typename ...Args>
            static void Convert(Stream&& out, Undefined, Args&&... args)
            {
                return;
            }

            template<typename DescriptorType, typename ...Args>
            static void Convert(DescriptorType desc, Undefined, Args&&... args)
            {
                return;
            }
        };

    }

    namespace data
    {
        template<typename EntryType>
        concept Entry = requires(EntryType entry)
        {
            { entry.Read() } -> myakish::streams::InputStream;
            { entry.Write() } -> myakish::streams::OutputStream;
        };

        template<typename StorageType>
        concept Storage = requires(StorageType data)
        {
            typename StorageType::Entry;
            typename StorageType::HandleFamily;

            //{ handle } -> DataHandle => { data.Acquire(handle) } -> std::convertible_to<typename StorageType::Entry&>;
            //{ handle } -> DataHandle => { data.Exists(handle) } -> std::convertible_to<bool>;
        };
    }
    
    namespace handle
    {
        struct NoFamily {};

        template<typename Handle>
        struct FamilyTraits
        {
            using Family = NoFamily;
        };

        template<typename Handle>
        using HandleFamily = typename FamilyTraits<std::remove_cvref_t<Handle>>::Family;

        template<typename HandleType>
        concept Handle = std::copyable<std::remove_cvref_t<HandleType>> && !std::same_as<NoFamily, HandleFamily<HandleType>> && std::totally_ordered<std::remove_cvref_t<HandleType>>;

        template<typename HandleType, typename Family>
        concept HandleOf = Handle<HandleType> && std::same_as<Family, HandleFamily<HandleType>>;


        template<typename Family, HandleOf<Family> HandleType>
        HandleType&& ResolveADL(Family, HandleType&& handle)
        {
            return std::forward<HandleType>(handle);
        }

        namespace detail
        {
            struct ResolveFunction
            {
                template<typename Family, typename Type>
                decltype(auto) operator()(Family, Type&& wrapper) const
                {
                    return ResolveADL(Family{}, std::forward<Type>(wrapper));
                }
            };
        }

        inline constexpr detail::ResolveFunction Resolve;


        struct NullHandle {};

        inline constexpr NullHandle Null;

        template<typename Arg>
        decltype(auto) operator/(NullHandle, Arg&& arg)
        {
            return std::forward<Arg>(arg);
        }
        template<typename Arg>
        decltype(auto) operator/(Arg&& arg, NullHandle)
        {
            return std::forward<Arg>(arg);
        }

        template<typename WrapperType, typename Family>
        concept Wrapper = std::same_as<WrapperType, NullHandle> || requires(WrapperType wrapper, Family familyTag)
        {
            { Resolve(familyTag, wrapper) } -> HandleOf<Family>;
        };
    }

    
    template<data::Storage StorageType, handle::Wrapper<typename StorageType::HandleFamily> Handle>
    class Descriptor
    {
    public:

        using HandleFamily = typename StorageType::HandleFamily;

    private:

        StorageType* data;
        Handle base;

    public:

        Descriptor(StorageType& data, Handle base) : data(&data), base(base)
        {

        }
        Descriptor(StorageType& data) : data(&data), base{}
        {

        }

        Descriptor(const Descriptor&) = default;

       
        template<typename Arg>
        auto Subtree(Arg&& arg) const
        {
            //trigger CTAD
            return myakish::tree::Descriptor(*data, handle::Resolve(HandleFamily{}, base / std::forward<Arg>(arg) ));
        }

        template<typename Arg>
        auto operator[](Arg&& arg) const
        {
            return Subtree(std::forward<Arg>(arg));
        }

        template<typename Type, typename ...Args>
        void Store(Type&& value, Args&&... args) const
        {
            if constexpr (conversion::TypeToHv<std::remove_cvref_t<Type>>::UseBinary)
            {
                data::Entry auto&& entry = data->Acquire(base);
                conversion::TypeToHv<std::remove_cvref_t<Type>>::Convert(entry.Write(), std::forward<Type>(value), std::forward<Args>(args)...);
            }
            else
            {
                conversion::TypeToHv<std::remove_cvref_t<Type>>::Convert(*this, std::forward<Type>(value), std::forward<Args>(args)...);
            }
        }
        template<typename Type> requires (!myakish::meta::SameBase<Type, Descriptor>)
        void operator=(Type&& value) const
        {
            return Store(std::forward<Type>(value));
        }


        template<typename Type, typename ...Args>
        auto Acquire(Args&&... args) const
        {
            if constexpr (conversion::HvToType<Type>::UseBinary)
            {
                data::Entry auto&& entry = data->Acquire(base);
                return conversion::HvToType<Type>::Convert(entry.Read(), std::forward<Args>(args)...);
            }
            else
            {
                return conversion::HvToType<Type>::Convert(*this, std::forward<Args>(args)...);
            }
        }
        template<typename Type>
        operator Type() const
        {
            return Acquire<Type>();
        }

        bool Exists() const
        {
            return data->Exists(base);
        }

        friend bool operator==(Descriptor, Descriptor) = default;
    };

    template<data::Storage StorageType>
    Descriptor(StorageType) -> Descriptor<StorageType, handle::NullHandle>;

    namespace detail
    {
        template<typename Type>
        struct AcquireFunction
        {
            template<data::Storage StorageType, handle::Wrapper<typename StorageType::HandleFamily> Handle, typename ...Args>
            decltype(auto) operator()(const Descriptor<StorageType, Handle>& desc, Args&&... args) const
            {
                return desc.Acquire<Type>(std::forward<Args>(args)...);
            }
        };
    }
    template<typename Type>
    inline constexpr detail::AcquireFunction<Type> Acquire;
}

template<typename Type>
inline constexpr bool myakish::functional::EnablePipelineFor<myakish::tree::detail::AcquireFunction<Type>> = true;
