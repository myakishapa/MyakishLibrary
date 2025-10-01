#pragma once

#include <MyakishLibrary/Utility.hpp>

#include <MyakishLibrary/Streams/Common.hpp>

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

        template<typename Family>
        struct NullHandle 
        {
            constexpr bool operator==(NullHandle) const
            {
                return true;
            }
        };

        template<typename Family>
        inline constexpr NullHandle<Family> Null;

        template<typename NullFamily>
        struct FamilyTraits<NullHandle<NullFamily>>
        {
            using Family = NullFamily;
        };

        template<typename Family, typename Arg>
        decltype(auto) operator/(NullHandle<Family>, Arg&& arg)
        {
            return std::forward<Arg>(arg);
        }
        template<typename Family, typename Arg>
        decltype(auto) operator/(Arg&& arg, NullHandle<Family>)
        {
            return std::forward<Arg>(arg);
        }

        template<typename HandleType>
        concept Handle = std::copyable<std::remove_cvref_t<HandleType>> && !std::same_as<NoFamily, HandleFamily<HandleType>> && std::equality_comparable<std::remove_cvref_t<HandleType>>;

        template<typename HandleType, typename Family>
        concept HandleOf = Handle<HandleType> && std::same_as<Family, HandleFamily<HandleType>>;


        template<typename Family, HandleOf<Family> HandleType>
        HandleType&& ResolveADL(Family, HandleType&& handle)
        {
            return std::forward<HandleType>(handle);
        }

        
        struct ResolveFunctor : functional::ExtensionMethod
        {
            template<typename Family, typename Type>
            decltype(auto) operator()(Family, Type&& wrapper) const
            {
                return ResolveADL(Family{}, std::forward<Type>(wrapper));
            }
        };      
        inline constexpr ResolveFunctor Resolve;

        struct NextFunctor : functional::ExtensionMethod
        {
            template<Handle Type>
            decltype(auto) operator()(const Type& handle) const
            {
                return NextADL(handle);
            }
        };
        inline constexpr NextFunctor Next;

        struct IsChildFunctor : functional::ExtensionMethod
        {
            template<Handle Type>
            bool operator()(const Type& parent, const Type& child) const
            {
                return IsChildADL(parent, child);
            }
        };
        inline constexpr IsChildFunctor IsChild;

        template<typename Type>
        concept LayeredHandle = Handle<Type> && std::totally_ordered<std::remove_cvref_t<Type>> && requires(Type handle)
        {
            IsChild(handle, handle);
            { Next(handle) } -> HandleOf<HandleFamily<Type>>;
        };


        template<typename WrapperType, typename Family>
        concept Wrapper = requires(WrapperType wrapper, Family familyTag)
        {
            { Resolve(familyTag, wrapper) } -> HandleOf<Family>;
        };
    }

    
    template<data::Storage StorageType, handle::HandleOf<typename StorageType::HandleFamily> Handle>
    class Descriptor
    {
    public:

        using HandleFamily = typename StorageType::HandleFamily;

    private:

        StorageType* data;
        Handle base;

    public:

        Descriptor(StorageType& data, const Handle &base) : data(&data), base(base)
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
            return myakish::tree::Descriptor(*data, base / handle::Resolve(HandleFamily{}, std::forward<Arg>(arg) ) );
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
        template<typename Type> requires (!meta::InstanceOfConcept<Type, myakish::tree::Descriptor>)
        void operator=(Type&& value) const
        {
            return Store(std::forward<Type>(value));
        }


        template<typename Type, typename ...Args> requires (!meta::InstanceOfConcept<Type, myakish::tree::Descriptor>)
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

        template<handle::HandleOf<HandleFamily> NewHandle>
        auto Rebase() const
        {
            return myakish::tree::Descriptor(*data, static_cast<NewHandle>(handle::Resolve(HandleFamily{}, base)) );
        }
        template<handle::HandleOf<HandleFamily> NewHandle>
        operator myakish::tree::Descriptor<StorageType, NewHandle>() const
        {
            return Rebase<NewHandle>();
        }

        Handle Base() const
        {
            return base;
        }

        bool Exists() const
        {
            return data->Exists(base);
        }

        friend bool operator==(Descriptor, Descriptor) = default;
    };

    template<data::Storage StorageType>
    Descriptor(StorageType&) -> Descriptor<StorageType, handle::NullHandle<typename StorageType::HandleFamily>>;
    template<data::Storage StorageType, handle::Wrapper<typename StorageType::HandleFamily> Handle>
    Descriptor(StorageType&, const Handle&) -> Descriptor<StorageType, Handle>;

    
    template<typename Type>
    struct AcquireFunctor : functional::ExtensionMethod
    {
        template<data::Storage StorageType, handle::HandleOf<typename StorageType::HandleFamily> Handle, typename ...Args>
        Type operator()(const Descriptor<StorageType, Handle>& desc, Args&&... args) const
        {
            return desc.Acquire<Type>(std::forward<Args>(args)...);
        }
    };
    template<typename Type>
    inline constexpr AcquireFunctor<Type> Acquire;

    template<typename Type>
    struct AcquireOrFunctor : functional::ExtensionMethod
    {
        template<data::Storage StorageType, handle::HandleOf<typename StorageType::HandleFamily> Handle, typename ...Args>
        Type operator()(const Descriptor<StorageType, Handle>& desc, Type defaultValue = {}, Args&&... args) const
        {
            return desc.Exists() ? desc.Acquire<Type>(std::forward<Args>(args)...) : defaultValue;
        }
    };
    template<typename Type>
    inline constexpr AcquireOrFunctor<Type> AcquireOr;

}