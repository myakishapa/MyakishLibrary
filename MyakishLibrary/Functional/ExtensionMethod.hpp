#pragma once
#include <concepts>
#include <tuple>

#include <MyakishLibrary/Functional/Pipeline.hpp>

namespace myakish::functional
{
    template<typename Type, typename ...Args>
    concept ExtensionInvocable = requires(Type&& object, Args&&... args)
    {
        std::forward<Type>(object).ExtensionInvoke(std::forward<Args>(args)...);
    };

    struct ExtensionMethod
    {
        template<typename Callable, typename ...Args>
        struct ExtensionClosure
        {
            Callable&& baseCallable;
            std::tuple<Args&&...> argsTuple;

            ExtensionClosure(Callable&& baseCallable, Args&&... args) : baseCallable(std::forward<Callable>(baseCallable)), argsTuple(std::forward<Args>(args)...) {}

            template<typename Extended, std::size_t ...Indices>
            decltype(auto) Invoke(Extended&& obj, std::index_sequence<Indices...>) const
            {
                return std::forward<Callable>(baseCallable).ExtensionInvoke(std::forward<Extended>(obj), std::forward<Args>(std::get<Indices>(argsTuple))...);
            }

            template<typename Extended>
            decltype(auto) operator()(Extended&& obj) const
            {
                return Invoke(std::forward<Extended>(obj), std::make_index_sequence<sizeof...(Args)>{});
            }
        };

        template<typename Self, typename ...Args> requires (!meta::InstanceOf<ExtensionClosure>::Func<Self>::value)
        auto operator()(this Self&& self, Args&&... args)
        {
            if constexpr (ExtensionInvocable<Self&&, Args&&...>) return self.ExtensionInvoke(std::forward<Args&&>(args)...);
            else return ExtensionClosure<Self, Args...>(std::forward<Self>(self), std::forward<Args>(args)...);
        }
    };

    template<std::derived_from<ExtensionMethod> ExtensionType>
    inline constexpr bool EnablePipelineFor<ExtensionType> = true;

    template<typename Callable, typename ...Args>
    inline constexpr bool EnablePipelineFor<ExtensionMethod::ExtensionClosure<Callable, Args...>> = true;

}