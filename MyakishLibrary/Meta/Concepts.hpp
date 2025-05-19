#pragma once

#include <concepts>
#include <type_traits>
#include <tuple>

namespace myakish::meta
{
    template<typename Type, typename Stripped>
    concept SameBase = std::same_as<Stripped, std::remove_cvref_t<Type>>;

    template<typename Type>
    concept TriviallyCopyable = std::is_trivially_copyable_v<Type>;

    template<typename Ref, typename Base>
    concept ForwardingReferenceOf = std::same_as<std::remove_cvref_t<Ref>, std::remove_cvref_t<Base>>&& std::is_reference_v<Ref&&>;

    template<typename Type>
    concept RvalueReference = std::is_rvalue_reference_v<Type>;

    template<typename Type>
    concept Aggregate = std::is_aggregate_v<Type>;

    namespace detail
    {
        template<class Type, std::size_t N>
        concept HasTupleElement = requires(Type t)
        {
            typename std::tuple_element_t<N, std::remove_const_t<Type>>;
            { get<N>(t) } -> std::convertible_to<const std::tuple_element_t<N, Type>&>;
        };
    }

    template<typename Type>
    concept TupleLike = requires(Type tuple)
    {
        typename std::tuple_size<Type>::type;
    } && []<std::size_t... Indices>(std::index_sequence<Indices...>)
    {
        return (detail::HasTupleElement<Type, Indices> && ...);
    }(std::make_index_sequence<std::tuple_size_v<Type>>());

    template<typename Type>
    concept Enum = std::is_scoped_enum_v<Type>;

    namespace detail
    {
        template <class Type>
        concept BooleanTestableImpl = std::convertible_to<Type, bool>;

        template <class Type>
        concept BooleanTestable = BooleanTestableImpl<Type> && requires(Type&& val)
        {
            { !static_cast<Type&&>(val) } -> BooleanTestableImpl;
        };

        template <class Type1, class Type2>
        concept HalfEqualityComparableWith = requires(const std::remove_reference_t<Type1>&__x, const std::remove_reference_t<Type2>&__y)
        {
            { __x == __y } -> BooleanTestable;
            { __x != __y } -> BooleanTestable;
        };
    }

    template<typename Type1, typename Type2 = Type1>
    concept EqualityComparableWith = detail::HalfEqualityComparableWith<Type1, Type2>&& detail::HalfEqualityComparableWith<Type2, Type1>;

    template<typename Type>
    concept LvalueReference = std::is_lvalue_reference_v<Type>;
}
