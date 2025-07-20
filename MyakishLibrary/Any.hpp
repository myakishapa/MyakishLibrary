#pragma once
#include <utility>
#include <new>

#include <MyakishLibrary/Meta.hpp>

namespace myakish
{
    class Any
    {
        using Deleter = void(*)(void*);

        void* data;
        Deleter deleter;

        template<typename T>
        constexpr static void Delete(void* object)
        {
            delete std::launder(reinterpret_cast<T*>(object));
        }

        template<typename Type>
        struct TypeTag {};

        template<typename Type, typename ...Args>
        constexpr Any(TypeTag<Type>, Args&&... args) : deleter(Delete<Type>), data(nullptr)
        {
            data = new Type(std::forward<Args>(args)...);
        }

    public:

        constexpr Any() : deleter(nullptr), data(nullptr) {}

        template<typename Type, typename ...Args>
        constexpr static Any Create(Args&&... args)
        {
            return Any(TypeTag<Type>{}, std::forward<Args>(args)...);
        }

        template<typename Type>
        constexpr static Any MoveFrom(Type arg)
        {
            return Any(TypeTag<Type>{}, std::move(arg));
        }

        constexpr Any(const Any&) = delete;
        constexpr Any(Any&& rhs) noexcept : data(std::exchange(rhs.data, nullptr)), deleter(rhs.deleter)
        {

        }

        constexpr Any& operator=(const Any&) = delete;
        constexpr Any& operator=(Any&& rhs) noexcept
        {
            std::swap(data, rhs.data);
            std::swap(deleter, rhs.deleter);
            return *this;
        }

        constexpr ~Any()
        {
            if(data) deleter(data);
        }

        template<typename T>
        constexpr operator T& ()
        {
            return Get();
        }
        template<typename T>
        constexpr operator const T& () const
        {
            return Get();
        }

        template<typename T>
        constexpr T& Get()
        {
            return *std::launder(reinterpret_cast<T*>(data));
        }
        template<typename T>
        constexpr const T& Get() const
        {
            return *std::launder(reinterpret_cast<T*>(data));
        }
    };

}