#pragma once

#include <MyakishLibrary/Utility.hpp>

#include <MyakishLibrary/Ranges/Utility.hpp>

#include <MyakishLibrary/Functional/ExtensionMethod.hpp>

#include <MyakishLibrary/Streams/Common.hpp>

#include <MyakishLibrary/BinarySerializationSuite/BinarySerializationSuite.hpp>

#include <memory>
#include <map>
#include <string_view>
#include <ranges>

namespace myakish::tree
{
    template<typename Type>
    concept HandleConcept = std::totally_ordered<Type>;

    template<typename ChildrenType, typename TreeType>
    concept TreeChildrenConcept =
        std::ranges::random_access_range<ChildrenType> &&
        std::ranges::sized_range<ChildrenType> &&
        std::same_as<TreeType, std::ranges::range_value_t<ChildrenType>>;

    template<typename Type>
    concept TreeConcept = requires(Type tree)
    {
        { tree.Children() } -> TreeChildrenConcept<Type>;
        { tree.Read() } -> streams::InputStream;
        { tree.Handle() } -> HandleConcept;
    };


    struct HandleFunctor : functional::ExtensionMethod
    {
        template<TreeConcept TreeType>
        decltype(auto) operator()(const TreeType& tree) const
        {
            return tree.Handle();
        }
    };
    inline constexpr HandleFunctor Handle;

    struct ChildrenFunctor : functional::ExtensionMethod
    {
        template<TreeConcept TreeType>
        decltype(auto) operator()(const TreeType& tree) const
        {
            return tree.Children();
        }
    };
    inline constexpr ChildrenFunctor Children;

    struct ReadFunctor : functional::ExtensionMethod
    {
        template<TreeConcept TreeType>
        decltype(auto) operator()(const TreeType& tree) const
        {
            return tree.Read();
        }
    };
    inline constexpr ReadFunctor Read;


    struct AtFunctor : functional::ExtensionMethod
    {
        template<TreeConcept TreeType, typename HandleType>
        auto operator()(const TreeType& tree, const HandleType& handle) const
        {
            return *std::ranges::lower_bound(Children(tree) | ranges::Borrow, handle, {}, Handle);
        }
    };
    inline constexpr AtFunctor At;


    template<typename Type>
    struct AcquireTraits
    {

    };

    template<typename Type>
    concept AcquireViaParser = requires(streams::InputStreamArchetype in, Type value)
    {
        AcquireTraits<Type>::Parser(in, value);
    };

    template<typename Type>
    concept AcquireViaFunction = requires(streams::InputStreamArchetype in)
    {
        { AcquireTraits<Type>::Parse(in) } -> std::same_as<Type>;
    };

    template<typename Type>
    concept AcquireableConcept = AcquireViaParser<Type> || AcquireViaFunction<Type>;

    template<AcquireableConcept Type>
    struct AcquireFunctor : functional::ExtensionMethod
    {
        template<TreeConcept TreeType>
        Type operator()(const TreeType& tree) const
        {
            if constexpr (AcquireViaParser<Type>)
            {
                Type into{};
                AcquireTraits<Type>::Parser(Read(tree), into);
                return into;
            }
            else if constexpr (AcquireViaFunction<Type>)
            {
                return AcquireTraits<Type>::Parse(Read(tree));
            }
            else static_assert(false);
        }
    };
    template<AcquireableConcept Type>
    inline constexpr AcquireFunctor<Type> Acquire;


    template<meta::TriviallyCopyableConcept Trivial>
    struct AcquireTraits<Trivial>
    {
        inline constexpr static auto Parser = binary_serialization_suite::template Trivial<Trivial>;
    };


    template<HandleConcept StorageHandle>
    struct Storage
    {
        struct Entry
        {
            StorageHandle handle;
            std::vector<std::byte> data;
            std::vector<myakish::Size> childrenOffsets;
        };

        struct EntryHandle
        {
            const Entry* tree;

            EntryHandle() = default;
            EntryHandle(const Entry* tree) : tree(tree) {}
            EntryHandle(const Entry& tree) : tree(&tree) {}

            const StorageHandle& Handle() const
            {
                return tree->handle;
            }

            auto Children() const
            {
                return tree->childrenOffsets | std::views::transform([&](myakish::Size offset) -> EntryHandle
                    {
                        return EntryHandle(tree + offset);
                    });
            }

            auto Read() const
            {
                return tree->data | streams::ReadFromRange;
            }
        };

        std::vector<Entry> entries;


        EntryHandle Root() const
        {
            return entries[0];
        }

        const StorageHandle& Handle() const
        {
            return Root().Handle();
        }

        void BuildInto(Storage& into) const
        {
            into.entries.append_range(entries);
        }
    };

    template<HandleConcept Handle>
    using TreeHandle = Storage<Handle>::EntryHandle;
}