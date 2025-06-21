#pragma once
#include <concepts>
#include <tuple>
#include <ranges>

#include <MyakishLibrary/Functional/Pipeline.hpp>

namespace myakish::functional::higher_order
{
    auto Compose()
    {
        return std::identity{};
    }

    template<typename First, typename ...Rest>
    auto Compose(First&& first, Rest&&... rest)
    {
        return [&]<typename ...Args>(Args&&... args) -> decltype(auto)
        {
            return Compose(std::forward<Rest>(rest)...)(std::invoke(std::forward<First>(first), std::forward<Args>(args)...));
        };
    }
}