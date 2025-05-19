#pragma once
#include <concepts>

namespace myakish::functional
{
    template<typename Type>
    inline constexpr bool EnablePipelineFor = false;

    template<typename Type>
    concept Pipelinable = EnablePipelineFor<std::remove_cvref_t<Type>>;

    namespace operators
    {
        template<typename Arg, Pipelinable Func>
        decltype(auto) operator|(Arg&& arg, Func&& f)
        {
            return std::forward<Func>(f)(std::forward<Arg>(arg));
        }
    }
}

