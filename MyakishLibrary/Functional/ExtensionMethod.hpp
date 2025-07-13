#pragma once
#include <concepts>
#include <tuple>

#include <MyakishLibrary/Functional/Pipeline.hpp>

namespace myakish::functional
{
    struct DisallowDirectInvoke {};
    struct RightCurry {};

    namespace detail
    {
        template<typename Type>
        concept RightCurried = std::derived_from<std::remove_cvref_t<Type>, RightCurry>;

        template<typename Callable, typename ...Args>
        concept ExtensionInvocable = requires(Callable && object, Args&&... args)
        {
            std::forward<Callable>(object).ExtensionInvoke(std::forward<Args>(args)...);
        };

        template<typename Callable, typename ...Args>
        concept DirectExtensionInvocable = !std::derived_from<std::remove_cvref_t<Callable>, DisallowDirectInvoke> && ExtensionInvocable<Callable, Args...>;

    }


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


            template<typename ...Args, std::size_t ...Indices>
            decltype(auto) Dispatch(std::index_sequence<Indices...> seq, Args&&... args) const
            {
                if constexpr (detail::RightCurried<Callable>) return RightInvoke(seq, std::forward<Args>(args)...);
                else return                                           LeftInvoke(seq, std::forward<Args>(args)...);
            }

            template<typename ...Args, std::size_t ...Indices> requires detail::ExtensionInvocable<Callable, Args..., CurryArgs...>
            decltype(auto) LeftInvoke(std::index_sequence<Indices...>, Args&&... args) const
            {
                return std::forward<Callable>(baseCallable).ExtensionInvoke(std::forward<Args>(args)..., std::forward<CurryArgs>(std::get<Indices>(curryArgs))...);
            }

            template<typename ...Args, std::size_t ...Indices> requires detail::ExtensionInvocable<Callable, CurryArgs..., Args...>
            decltype(auto) RightInvoke(std::index_sequence<Indices...>, Args&&... args) const
            {
                return std::forward<Callable>(baseCallable).ExtensionInvoke(std::forward<CurryArgs>(std::get<Indices>(curryArgs))..., std::forward<Args>(args)...);
            }
        };

        template<typename Self, typename ...Args>
        decltype(auto) operator()(this Self&& self, Args&&... args)
        {
            return ExtensionClosure<Self, Args...>(std::forward<Self>(self), std::forward<Args>(args)...);
        }

        template<typename Self, typename ...Args> requires detail::DirectExtensionInvocable<Self&&, Args&&...>
        decltype(auto) operator()(this Self&& self, Args&&... args)
        {
            return std::forward<Self>(self).ExtensionInvoke(std::forward<Args>(args)...);
        }
    };

    template<std::derived_from<ExtensionMethod> ExtensionType>
    inline constexpr bool EnablePipelineFor<ExtensionType> = true;

    template<typename Callable, typename ...Args>
    inline constexpr bool EnablePipelineFor<ExtensionMethod::ExtensionClosure<Callable, Args...>> = true;

}