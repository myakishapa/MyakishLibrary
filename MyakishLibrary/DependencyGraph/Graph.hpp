#pragma once

#include <iostream>
#include <functional>
#include <map>
#include <set>
#include <print>
#include <format>
#include <variant>
#include <algorithm>
#include <utility>

#include <MyakishLibrary/Any.hpp>

namespace myakish::dependency_graph
{
    template<typename Type>
    struct DefaultCreateTraits
    {
        template<typename CreateArgs, typename DependencyProvider>
        static myakish::Any Create(CreateArgs, DependencyProvider&&)
        {
            return myakish::Any::Create<Type>();
        }
    };

    template<typename Handle, typename CreateArguments, template<typename> typename CreateTraits = DefaultCreateTraits>
    class Graph
    {
        using DependencySet = std::set<Handle>;
        struct Node
        {
            DependencySet children;
            DependencySet parents;

            std::uint64_t externalDependencies;

            Any value;
        };

    public:

        template<typename T>
        class ExternalReference
        {
            Node& node;

            friend class Graph;

            ExternalReference() = delete;
            ExternalReference(Node& node) : node(node)
            {
                node.externalDependencies++;
            }

        public:

            ExternalReference(const ExternalReference& rhs) : node(rhs.node)
            {
                node.externalDependencies++;
            }

            ExternalReference operator=(const ExternalReference&) = delete;

            T& Get() const
            {
                return node.value.Get<T>();
            }

            operator T& () const
            {
                return Get();
            }

            T* operator->() const
            {
                return &Get();
            }

            T& operator*() const
            {
                return Get();
            }

            ~ExternalReference()
            {
                node.externalDependencies--;
            }
        };

    private:

        std::map<Handle, CreateArguments> arguments;

        std::map<Handle, Node> nodes;

        class DependencyProvider
        {
            Graph& graph;
            const Handle& handle;

            DependencySet parents;

            friend class Graph;

        public:

            DependencyProvider(Graph& graph, const Handle& handle) : graph(graph), handle(handle), parents{} {}

            template<typename T>
            T& Acquire(const Handle& dependency)
            {
                auto& node = graph.AcquireNode<T>(dependency);

                node.children.insert(handle);
                parents.insert(dependency);

                return node.value.Get<T>();
            }
        };

        template<typename T>
        Node& CreateDependent(const Handle& handle)
        {
            DependencyProvider provider(*this, handle);
            myakish::Any&& container = CreateTraits<T>::Create(arguments.at(handle), provider);
            return CreateNode<T>(handle, std::move(container), {}, provider.parents);
        }

        template<typename T>
        Node& AcquireNode(const Handle& handle)
        {
            auto it = nodes.find(handle);
            if (it != nodes.end()) return it->second;
            else return CreateDependent<T>(handle);
        }

        template<typename T, typename ...Args>
        Node& CreateNode(const Handle& handle, Args&&... createArgs)
        {
            return CreateNode<T>(handle, myakish::Any::Create<T>(std::forward<Args>(createArgs)...));
        }
        template<typename T>
        Node& CreateNode(const Handle& handle, myakish::Any&& container, DependencySet children = {}, DependencySet parents = {})
        {
            auto node = nodes.emplace(std::piecewise_construct, std::forward_as_tuple(handle), std::forward_as_tuple(std::move(children), std::move(parents), 0, std::move(container)));
            return node.first->second;
        }

    public:

        template<typename T>
        ExternalReference<T> Acquire(const Handle& handle)
        {
            return ExternalReference<T>(AcquireNode<T>(handle));
        }

        template<typename T, typename ...Args>
        ExternalReference<T> EmplaceNode(const Handle& handle, Args&&... createArgs)
        {
            return ExternalReference<T>(CreateNode<T>(handle, std::forward<Args>(createArgs)...));
        }

        void SetDependencies(Handle handle, CreateArguments args)
        {
            arguments.emplace(handle, args);
        }

        bool Referenced(Handle handle)
        {
            auto it = nodes.find(handle);
            if (it == nodes.end()) return false;
            return (it->second.externalDependencies != 0) || (it->second.children.size() != 0);
        }

        void Clean()
        {
            std::erase_if(nodes, [&](auto&& pair) -> bool
                {
                    auto&& [handle, node] = pair;
                    if (Referenced(handle)) return false;
                    for (auto&& parentHandle : node.parents)
                    {
                        auto& parentNode = nodes.at(parentHandle);
                        parentNode.children.erase(handle);
                    }
                    return true;
                });
        }
    };
}