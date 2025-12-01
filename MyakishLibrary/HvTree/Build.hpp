#pragma once

#include <MyakishLibrary/HvTree/HvTree.hpp>

namespace myakish::tree
{
    struct ChildrenBuilder
    {
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;

        struct promise_type
        {
            std::string currentHandle;

            ChildrenBuilder get_return_object()
            {
                return ChildrenBuilder(handle_type::from_promise(*this));
            }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            template<std::convertible_to<std::string> Handle>
            std::suspend_always yield_value(Handle&& handle)
            {
                currentHandle = std::forward<Handle>(handle);
                return {};
            }
            void return_void() {}

            void unhandled_exception()
            {
                std::terminate();
            }
        };

        handle_type handle;

        ChildrenBuilder(handle_type h) : handle(h) {}
        ChildrenBuilder(const ChildrenBuilder&) = delete;
        ~ChildrenBuilder() { handle.destroy(); }

        std::string CurrentHandle() const
        {
            return handle.promise().currentHandle;
        }
        void operator()() const
        {
            handle();
        }

        operator bool() const
        {
            return !handle.done();
        }
    };



    template<typename Type>
    concept HasHandle = requires(Type source)
    {
        { source.Handle() } -> HandleConcept;
    };

    template<typename Type>
    concept HasHandleType = HasHandle<Type> || requires
    {
        typename std::remove_cvref_t<Type>::HandleType;
    };


    template<typename Type>
    struct HandleType : meta::Undefined {};

    template<HasHandleType Type>
    struct HandleType<Type> : meta::ReturnType<typename std::remove_cvref_t<Type>::HandleType> {};

    template<HasHandle Type>
    struct HandleType<Type> : std::remove_cvref<decltype(std::declval<Type>().Handle())> {};


    template<typename Type>
    struct CopyHandleType {};

    template<HasHandleType Type>
    struct CopyHandleType<Type>
    {
        using HandleType = std::remove_cvref_t<Type>::HandleType;
    };

    template<HasHandle Type>
    struct CopyHandleType<Type> {};



    template<typename Type>
    concept CustomBuild = HasHandleType<Type> && requires(Type source, Storage<typename HandleType<Type>::type> storage)
    {
        source.BuildInto(storage);
    };

    template<typename Type>
    concept HasChildren = HasHandleType<Type> && requires(Type source, Storage<typename HandleType<Type>::type> storage)
    {
        { source.BuildChildren(storage) } -> std::same_as<ChildrenBuilder>;
    };

    template<typename Type>
    concept HasData = requires(Type source, streams::OutputStreamArchetype stream)
    {
        source.WriteData(stream);
    };


    template<typename Type>
    concept BuildSource = HasChildren<Type> || HasHandle<Type> || HasData<Type> || CustomBuild<Type>;


    struct BuildFunctor : functional::ExtensionMethod
    {
        template<BuildSource Source, HandleConcept Handle>
        void operator()(Storage<Handle>& storage, Source&& source) const
        {
            auto treeIndex = storage.entries.size();

            if constexpr (CustomBuild<Source>)
            {
                source.BuildInto(storage);
            }
            else
            {
                storage.entries.emplace_back();
            }

            auto tree = [&]() -> decltype(auto)
                {
                    return storage.entries[treeIndex];
                };

            if constexpr (HasHandle<Source>) tree().handle = source.Handle();

            if constexpr (HasData<Source>)
            {
                streams::VectorOutputStream stream(tree().data);
                source.WriteData(stream);
            }

            if constexpr (HasChildren<Source>)
            {
                auto builder = source.BuildChildren(storage);

                while (builder)
                {
                    tree().childrenOffsets.push_back(storage.entries.size() - treeIndex);

                    builder();
                }
            }
        }

        template<HasHandleType Source>
        auto operator()(Source&& source) const
        {
            Storage<typename HandleType<std::remove_cvref_t<Source>>::type> storage{};
            operator()(storage, source);
            return storage;
        };
    };
    inline constexpr BuildFunctor Build;


    template<BuildSource Target, HasHandle HandleSource>
    struct WithHandle
    {
        Target target;
        HandleSource handle;

        WithHandle(Target target, HandleSource handle) : target(std::move(target)), handle(std::move(handle)) {}

        template<HandleConcept Handle>
        void BuildInto(Storage<Handle>& into) const requires CustomBuild<Target>
        {
            target.BuildInto(into);
        }

        void WriteData(streams::OutputStream auto&& out) const requires HasData<Target>
        {
            target.WriteData(out);
        }

        decltype(auto) Handle() const
        {
            return handle.Handle();
        }

        template<HandleConcept Handle>
        decltype(auto) BuildChildren(Storage<Handle>& storage) const requires HasChildren<Target>
        {
            return target.BuildChildren(storage);
        }
    };

    template<BuildSource Target, HasData DataSource>
    struct WithData : CopyHandleType<Target>
    {
        Target target;
        DataSource data;

        WithData(Target target, DataSource data) : target(std::move(target)), data(std::move(data)) {}

        template<HandleConcept Handle>
        void BuildInto(Storage<Handle>& into) const requires CustomBuild<Target>
        {
            target.BuildInto(into);
        }

        void WriteData(streams::OutputStream auto&& out) const
        {
            data.WriteData(out);
        }

        decltype(auto) Handle() const requires HasHandle<Target>
        {
            return target.Handle();
        }

        template<HandleConcept Handle>
        decltype(auto) BuildChildren(Storage<Handle>& storage) const requires HasChildren<Target>
        {
            return target.BuildChildren(storage);
        }
    };

    template<BuildSource Target, HasHandle Child> requires(!CustomBuild<Target>)
    struct WithChild : CopyHandleType<Target>
    {
        Target target;
        Child child;

        WithChild(Target target, Child child) : target(std::move(target)), child(std::move(child)) {}


        void WriteData(streams::OutputStream auto&& out) const requires HasData<Target>
        {
            target.WriteData(out);
        }

        decltype(auto) Handle() const requires HasHandle<Target>
        {
            return target.Handle();
        }

        template<HandleConcept Handle>
        ChildrenBuilder BuildChildren(Storage<Handle>& storage) const requires !HasChildren<Target>
        {
            co_yield child.Handle();

            Build(storage, child);
        }

        template<HandleConcept Handle>
        ChildrenBuilder BuildChildren(Storage<Handle>& storage) const requires HasChildren<Target>
        {
            auto builder = target.BuildChildren(storage);

            while (builder && builder.CurrentHandle() < child.Handle())
            {
                co_yield builder.CurrentHandle();
                builder();
            }

            co_yield child.Handle();

            Build(storage, child);

            while (builder)
            {
                co_yield builder.CurrentHandle();
                builder();
            }
        }
    };


    template<BuildSource Target, HasHandle HandleSource>
    auto operator%(Target target, HandleSource handle) -> WithHandle<Target, HandleSource>
    {
        return WithHandle<Target, HandleSource>(std::move(target), std::move(handle));
    }

    template<BuildSource Target, HasData DataSource>
    auto operator*(Target target, DataSource data) -> WithData<Target, DataSource>
    {
        return WithData<Target, DataSource>(std::move(target), std::move(data));
    }

    template<BuildSource Target, HasHandle Child>requires(!CustomBuild<Target>)
    auto operator/(Target target, Child child) -> WithChild<Target, Child>
    {
        return WithChild<Target, Child>(std::move(target), std::move(child));
    }


    template<HandleConcept Handle>
    struct HandleSource
    {
        Handle handle;

        const Handle& Handle() const
        {
            return handle;
        }
    };

    struct DataSource
    {
        std::vector<std::byte> data;

        void WriteData(streams::OutputStream auto&& out) const
        {
            streams::Write(out, data.data(), data.size());
        }
    };


    struct RawDataFunctor : functional::ExtensionMethod
    {
        template<meta::TriviallyCopyableConcept Trivial>
        DataSource operator()(Trivial trivial) const
        {
            DataSource source{};

            source.data.resize(sizeof(Trivial));
            std::memcpy(source.data.data(), &trivial, sizeof(Trivial));

            return source;
        };
    };
    inline constexpr RawDataFunctor RawData;


    template<typename Value, binary_serialization_suite::ParserConcept Parser>
    struct ParserValueSource
    {
        Value* value;
        const Parser* parser;

        ParserValueSource(Value&& value, const Parser& parser) : value(&value), parser(&parser) {}

        void WriteData(streams::OutputStream auto&& out) const
        {
            (*parser)(out, std::forward<Value>(*value));
        }
    };
    template<typename Value, binary_serialization_suite::ParserConcept Parser>
    ParserValueSource(Value&&, const Parser&) -> ParserValueSource<Value, Parser>;

    struct ViaFunctor : functional::ExtensionMethod
    {
        template<typename Value, binary_serialization_suite::ParserConcept Parser>
        auto operator()(Value&& value, const Parser& parser) const
        {
            return ParserValueSource(std::forward<Value>(value), parser);
        };
    };
    inline constexpr ViaFunctor Via;
}

namespace myakish::literals
{
    tree::HandleSource<std::string> operator""_tree(const char* str, std::size_t)
    {
        return { str };
    }
}