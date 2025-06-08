#pragma once
#include <concepts>
#include <tuple>

#include <MyakishLibrary/Functional/Pipeline.hpp>

namespace myakish::functional
{
    struct ExtensionMethod
    {
        template<typename Callable, typename ...Args>
        struct ExtensionClosure
        {
            Callable&& baseCallable;
            std::tuple<Args&&> args;

            ExtensionClosure(Callable&& baseCallable, Args&&... args) : baseCallable(std::forward<Callable>(baseCallable)), args(std::forward<Args>(args)...) {}

            template<typename Extended, std::size_t ...Indices>
            decltype(auto) Invoke(Extended&& obj, std::index_sequence<Indices...>) const
            {
                return std::forward<Callable>(baseCallable)(std::forward<Extended>(obj), std::forward<Args>(std::get<Indices>(args))...);
            }

            template<typename Extended>
            decltype(auto) operator()(Extended&& obj) const
            {
                return Invoke(std::forward<Extended>(obj), sizeof...(Args));
            }
        };

        template<typename Self, typename ...Args>
        auto operator()(this Self&& self, Args&&... args)
        {
            return ExtensionClosure<Self, Args...>(std::forward<Self>(self), std::forward<Args>(args)...);
        }
    };

    template<std::derived_from<ExtensionMethod> ExtensionType>
    inline constexpr bool EnablePipelineFor<ExtensionType> = true;
}