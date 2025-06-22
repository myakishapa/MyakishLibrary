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
    auto Compose(First first, Rest... rest)
    {
        return [=]<typename ...Args>(Args&&... args) -> decltype(auto)
        {
            return Compose(std::move(rest)...)(std::invoke(first, std::forward<Args>(args)...));
        };
    }

    template<typename Type>
    auto Constant(Type value)
    {
        return [=](auto&& _)
            {
                return value;
            };
    }
}