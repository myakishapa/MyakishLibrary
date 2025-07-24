#pragma once
#include <concepts>
#include <tuple>

#include <MyakishLibrary/Meta.hpp>

namespace myakish::functional
{
    struct ExtensionMethod
    {
        template<typename Callable, typename ...CurryArgs>
        struct ExtensionClosure
        {
            Callable&& baseCallable;
            std::tuple<CurryArgs&&...> curryArgs;

            ExtensionClosure(Callable&& baseCallable, CurryArgs&&... curryArgs) : baseCallable(std::forward<Callable>(baseCallable)), curryArgs(std::forward<CurryArgs>(curryArgs)...) {}


            template<typename ...Args>
            decltype(auto) operator()(Args&&... args) const
            {
                return Dispatch(std::make_index_sequence<sizeof...(CurryArgs)>{}, std::forward<Args>(args)...);
            }

        private:

            template<typename ...Args, std::size_t ...Indices> requires std::invocable<Callable&&, Args&&..., CurryArgs&&...>
            decltype(auto) Dispatch(std::index_sequence<Indices...> seq, Args&&... args) const
            {
                return std::invoke(std::forward<Callable>(baseCallable), std::forward<Args>(args)..., std::forward<CurryArgs>(std::get<Indices>(curryArgs))...);
            }
        };

        template<typename Self, typename ...Args>
        auto operator[](this Self&& self, Args&&... args)
        {
            return ExtensionClosure<Self, Args...>(std::forward<Self>(self), std::forward<Args>(args)...);
        }


        template<typename Invocable>
        struct PartialApplier
        {
            Invocable&& invocable;

            PartialApplier(Invocable&& invocable) : invocable(std::forward<Invocable>(invocable)) {}

            template<typename ...Args>
            auto operator()(Args&&... args) const
            {
                return std::forward<Invocable>(invocable)[std::forward<Args>(args)...];
            }
        };

        template<typename Self>
        auto operator*(this Self&& self)
        {
            return PartialApplier<Self>(std::forward<Self>(self));
        }
    };

    template<typename Arg, typename Extension> requires std::derived_from<std::remove_cvref_t<Extension>, ExtensionMethod>
    decltype(auto) operator|(Arg&& arg, Extension&& ext)
    {
        return std::forward<Extension>(ext)(std::forward<Arg>(arg));
    }

    template<typename Arg, meta::InstanceOfConcept<ExtensionMethod::ExtensionClosure> Extension>
    decltype(auto) operator|(Arg&& arg, Extension&& ext)
    {
        return std::forward<Extension>(ext)(std::forward<Arg>(arg));
    }

    template<typename Arg, meta::InstanceOfConcept<ExtensionMethod::PartialApplier> PartialApplier>
    decltype(auto) operator|(Arg&& arg, PartialApplier&& applier)
    {
        return std::forward<PartialApplier>(applier)(std::forward<Arg>(arg));
    }
}