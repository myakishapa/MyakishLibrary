#pragma once
#include <concepts>
#include <tuple>

#include <MyakishLibrary/Meta.hpp>

namespace myakish::functional
{

    template<typename Type>
    struct LambdaExpression : std::false_type {};

    template<typename Type>
    concept LambdaExpressionConcept = LambdaExpression<std::remove_cvref_t<Type>>::value;


    struct LambdaExpressionTag {};

    template<std::derived_from<LambdaExpressionTag> Type>
    struct LambdaExpression<Type> : std::true_type {};

    namespace detail
    {
        template<myakish::Size Index, typename ...Types, typename ReturnType = meta::At<Index, meta::TypeList<Types...>>::type>
        ReturnType ForwardFromTuple(const std::tuple<Types...>& tuple) requires std::is_reference_v<ReturnType>
        {
            return std::forward<ReturnType>(std::get<Index>(tuple));
        }


        template<typename Invocable, typename ...Types, myakish::Size ...Indices> requires(std::invocable<Invocable&&, Types&&...>)
            decltype(auto) ForwardingApply(Invocable&& invocable, const std::tuple<Types...>& tuple, std::integer_sequence<myakish::Size, Indices...>)
        {
            return std::invoke(std::forward<Invocable>(invocable), ForwardFromTuple<Indices>(tuple)...);
        }

        template<typename Invocable, typename ...Types> requires(std::invocable<Invocable&&, Types&&...>)
            decltype(auto) ForwardingApply(Invocable&& invocable, const std::tuple<Types...>& tuple)
        {
            return ForwardingApply(std::forward<Invocable>(invocable), tuple, std::make_integer_sequence<myakish::Size, sizeof...(Types)>{});
        }
    }


    template<myakish::Size Index>
    struct Placeholder : LambdaExpressionTag
    {
        template<typename Invocable, typename ArgsTuple> requires(0 <= Index && Index < std::tuple_size_v<std::remove_cvref_t<ArgsTuple>>)
            constexpr decltype(auto) LambdaResolve(Invocable&& invocable, const ArgsTuple& argsTuple) const
        {
            return std::invoke(std::forward<Invocable>(invocable), detail::ForwardFromTuple<Index>(argsTuple));
        }
    };
    template<myakish::Size Index>
    inline constexpr Placeholder<Index> Arg;


    template<typename Value>
    struct ConstantExpression : LambdaExpressionTag
    {
        Value value;

        constexpr ConstantExpression(Value value) : value(std::forward<Value>(value)) {}

        template<typename Invocable, typename ArgsTuple>
        constexpr decltype(auto) LambdaResolve(Invocable&& invocable, const ArgsTuple&) const
        {
            return std::invoke(std::forward<Invocable>(invocable), static_cast<Value>(value));
        }
    };
    template<typename RvalueReference>
    ConstantExpression(RvalueReference&&) -> ConstantExpression<RvalueReference>;
    template<typename LvalueReference>
    ConstantExpression(LvalueReference&) -> ConstantExpression<LvalueReference&>;

    namespace detail
    {
        template<typename NonLambda>
        auto MakeExpression(NonLambda&& value)
        {
            return ConstantExpression(std::forward<NonLambda>(value));
        }

        template<LambdaExpressionConcept Lambda>
        Lambda&& MakeExpression(Lambda&& lambda)
        {
            return std::forward<Lambda>(lambda);
        }


        template<typename Continuation>
        decltype(auto) IndirectAccumulate(Continuation&& continuation)
        {
            return std::invoke(std::forward<Continuation>(continuation));
        }


#pragma warning(disable: 26800)
        template<typename Continuation, typename First, typename ...Rest>
        decltype(auto) IndirectAccumulate(Continuation&& continuation, First&& first, Rest&&... rest)
        {
            auto InvokeTarget = [&]<typename... ResolvedArgs>(ResolvedArgs&&... resolvedArgs) -> decltype(auto)
            {
                auto InvokeWithResolved = [&]<typename... OtherArgs>(OtherArgs&&... otherArgs) -> decltype(auto)
                {
                    return std::invoke(std::forward<Continuation>(continuation), std::forward<ResolvedArgs>(resolvedArgs)..., std::forward<OtherArgs>(otherArgs)...);
                };

                return IndirectAccumulate(InvokeWithResolved, std::forward<Rest>(rest)...);
            };

            return std::invoke(std::forward<First>(first), InvokeTarget);
        }




        
        template<LambdaExpressionConcept Resolver, typename ArgsTuple>
        auto IntoIndirectTransform(const Resolver& resolver, const ArgsTuple& argsTuple)
        {
            return [&]<typename Continuation>(Continuation && continuation) -> decltype(auto)
            {
                return resolver.LambdaResolve(std::forward<Continuation>(continuation), argsTuple);
            };
        }

        template<typename Invocable, typename ArgsTuple, LambdaExpressionConcept ...Resolvers>
        decltype(auto) LambdaInvoke(Invocable&& invocable, const ArgsTuple& argsTuple, const Resolvers&... resolvers)
        {
            return IndirectAccumulate(std::forward<Invocable>(invocable), IntoIndirectTransform(resolvers, argsTuple)...);
        }
    }



    template<typename Invocable, LambdaExpressionConcept ...ArgResolvers>
    struct LambdaClosure : LambdaExpressionTag
    {
        Invocable&& baseInvocable;
        std::tuple<ArgResolvers...> resolvers;

        constexpr LambdaClosure(Invocable&& baseInvocable, ArgResolvers... resolvers) : baseInvocable(std::forward<Invocable>(baseInvocable)), resolvers(std::move(resolvers)...) {}

        template<typename Invocable, typename ArgsTuple>
        constexpr decltype(auto) LambdaResolve(Invocable&& invocable, const ArgsTuple& argsTuple) const
        {
            auto InvokeSelf = [&]<typename ...Args>(Args&&... args) -> decltype(auto) { return Invoke(std::forward<Args>(args)...); };
            return std::invoke(std::forward<Invocable>(invocable), detail::ForwardingApply(InvokeSelf, argsTuple));
        }

        template<typename ...Args>
        constexpr decltype(auto) operator()(Args&&... args) const
        {
            return Invoke(std::forward<Args>(args)...);
        }

    private:

        template<typename ...Args>
        constexpr decltype(auto) Invoke(Args&&... args) const
        {
            return Dispatch(std::make_index_sequence<sizeof...(ArgResolvers)>{}, std::forward<Args>(args)...);
        }

        template<typename ...Args, std::size_t ...Indices>
        constexpr decltype(auto) Dispatch(std::index_sequence<Indices...>, Args&&... args) const
        {
            return detail::LambdaInvoke(std::forward<Invocable>(baseInvocable), std::forward_as_tuple(std::forward<Args>(args)...), std::get<Indices>(resolvers)...);
        }
    };

    template<typename Invocable, typename ...ArgResolvers>
    LambdaClosure(Invocable&&, ArgResolvers...) -> LambdaClosure<Invocable&&, ArgResolvers...>;


    template<typename Invocable, typename ...CurryArgs>
    struct ExtensionClosure
    {
        Invocable&& baseInvocable;
        std::tuple<CurryArgs&&...> curryArgs;

        ExtensionClosure(Invocable&& baseInvocable, CurryArgs&&... curryArgs) : baseInvocable(std::forward<Invocable>(baseInvocable)), curryArgs(std::forward<CurryArgs>(curryArgs)...) {}


        template<typename ...Args> requires std::invocable<Invocable&&, Args&&..., CurryArgs&&...>
        decltype(auto) operator()(Args&&... args) const
        {
            return Dispatch(std::make_index_sequence<sizeof...(CurryArgs)>{}, std::forward<Args>(args)...);
        }

    private:

        template<typename ...Args, std::size_t ...Indices> requires std::invocable<Invocable&&, Args&&..., CurryArgs&&...>
        decltype(auto) Dispatch(std::index_sequence<Indices...> seq, Args&&... args) const
        {
            return std::invoke(std::forward<Invocable>(baseInvocable), std::forward<Args>(args)..., std::forward<CurryArgs>(std::get<Indices>(curryArgs))...);
        }
    };

    template<typename Invocable, typename ...CurryArgs>
    ExtensionClosure(Invocable&&, CurryArgs&&...) -> ExtensionClosure<Invocable&&, CurryArgs&&...>;


    struct ExtensionMethod
    {
        template<typename Self, typename ...Args>
        auto operator[](this Self&& self, Args&&... args)
        {
            if constexpr (meta::QuotedAnyOf<meta::Compose<std::remove_cvref, LambdaExpression>, meta::TypeList<Args&&...>>::value)
                return LambdaClosure(std::forward<Self>(self), detail::MakeExpression(std::forward<Args>(args))...);
            else
                return ExtensionClosure(std::forward<Self>(self), std::forward<Args>(args)...);
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


    template<typename Type>
    struct LambdaOperand : std::true_type {};

    template<typename Type>
    concept LambdaOperandConcept = LambdaOperand<std::remove_cvref_t<Type>>::value;


    struct DisableLambdaOperatorsTag {};

    template<std::derived_from<DisableLambdaOperatorsTag> Type>
    struct LambdaOperand<Type> : std::false_type {};


    template<LambdaExpressionConcept Expression>
    struct CompleteLambda : ExtensionMethod
    {
        Expression expression;

        CompleteLambda(Expression expression) : expression(std::move(expression)) {}

        template<typename ...Args>
        decltype(auto) operator()(Args&&... args) const
        {
            return expression(std::forward<Args>(args)...);
        }
    };
    template<LambdaExpressionConcept Expression>
    CompleteLambda(Expression) -> CompleteLambda<Expression>;

    struct CompleteFunctor : ExtensionMethod, DisableLambdaOperatorsTag
    {
        template<LambdaExpressionConcept Expression>
        auto operator()(Expression expression) const
        {
            return CompleteLambda(std::forward<Expression>(expression));
        }
    };
    inline constexpr CompleteFunctor Complete;

    inline namespace higher_order
    {

#define PREFIX_UNARY_OPERATOR(op, name) \
    struct name##Functor : ExtensionMethod \
    { \
        template<class Arg>\
        decltype(auto) operator()(Arg&& arg) const requires requires { op std::forward<Arg>(arg); } \
        { \
            return op std::forward<Arg>(arg); \
        } \
    }; \
    inline constexpr name##Functor name;

#define POSTFIX_UNARY_OPERATOR(op, name) \
    struct name##Functor : ExtensionMethod \
    { \
        template<class Arg>\
        decltype(auto) operator()(Arg&& arg) const requires requires { std::forward<Arg>(arg) op; } \
        { \
            return std::forward<Arg>(arg) op; \
        } \
    }; \
    inline constexpr name##Functor name;

#define BINARY_OPERATOR(op, name) \
    struct name##Functor : ExtensionMethod \
    { \
        template<typename Lhs, typename Rhs> \
        decltype(auto) operator()(Lhs&& lhs, Rhs&& rhs) const requires requires { std::forward<Lhs>(lhs) op std::forward<Rhs>(rhs); } \
        { \
            return std::forward<Lhs>(lhs) op std::forward<Rhs>(rhs); \
        } \
    }; \
    inline constexpr name##Functor name;

        struct PlusFunctor : ExtensionMethod
        {
            template<typename Lhs, typename Rhs> 
            decltype(auto) operator()(Lhs&& lhs, Rhs&& rhs) const requires requires { std::forward<Lhs>(lhs) + std::forward<Rhs>(rhs); }
            {
                return std::forward<Lhs>(lhs) + std::forward<Rhs>(rhs);
            }

            template<typename Arg>
            decltype(auto) operator()(Arg&& arg) const requires requires { +std::forward<Arg>(arg); }
            {
                return +std::forward<Arg>(arg);
            }
        };
        inline constexpr PlusFunctor Plus;

        struct MinusFunctor : ExtensionMethod
        {
            template<typename Lhs, typename Rhs>
            decltype(auto) operator()(Lhs&& lhs, Rhs&& rhs) const requires requires { std::forward<Lhs>(lhs) - std::forward<Rhs>(rhs); }
            {
                return std::forward<Lhs>(lhs) - std::forward<Rhs>(rhs);
            }

            template<typename Arg>
            decltype(auto) operator()(Arg&& arg) const requires requires { -std::forward<Arg>(arg); }
            {
                return -std::forward<Arg>(arg);
            }
        };
        inline constexpr MinusFunctor Minus;

        struct SubscriptFunctor : ExtensionMethod
        {
            template<typename Lhs, typename ...Args>
            decltype(auto) operator()(Lhs&& lhs, Args&&... args) const requires requires { std::forward<Lhs>(lhs)[std::forward<Args>(args)...]; }
            {
                return std::forward<Lhs>(lhs)[std::forward<Args>(args)...];
            }
        };
        inline constexpr SubscriptFunctor Subscript;


        PREFIX_UNARY_OPERATOR(*, Dereference);
        PREFIX_UNARY_OPERATOR(~, BitNot);
        PREFIX_UNARY_OPERATOR(!, LogicalNot);

        PREFIX_UNARY_OPERATOR(++, Increment);
        PREFIX_UNARY_OPERATOR(--, Decrement);

        POSTFIX_UNARY_OPERATOR(++, PostfixIncrement);
        POSTFIX_UNARY_OPERATOR(--, PostfixDecrement);


        BINARY_OPERATOR(=, Assign);
        BINARY_OPERATOR(*, Multiply);
        BINARY_OPERATOR(/, Divide);
        BINARY_OPERATOR(%, Modulus);
        BINARY_OPERATOR(&, BitAnd);
        BINARY_OPERATOR(|, BitOr);
        BINARY_OPERATOR(^, BitXor);
        BINARY_OPERATOR(&&, LogicalAnd);
        BINARY_OPERATOR(||, LogicalOr);
        BINARY_OPERATOR(<<, LeftShift);
        BINARY_OPERATOR(>>, RightShift);
        BINARY_OPERATOR(->*, Projection);

        BINARY_OPERATOR(<, Less);
        BINARY_OPERATOR(>, Greater);
        BINARY_OPERATOR(<=, LessEqual);
        BINARY_OPERATOR(>=, GreaterEqual);
        BINARY_OPERATOR(==, Equal);
        BINARY_OPERATOR(!=, NotEqual);
        BINARY_OPERATOR(<=>, Spaceship);

        BINARY_OPERATOR(+=, PlusAssign);
        BINARY_OPERATOR(-=, MinusAssign);
        BINARY_OPERATOR(*=, MultiplyAssign);
        BINARY_OPERATOR(/=, DivideAssign);
        BINARY_OPERATOR(%=, ModulusAssign);
        BINARY_OPERATOR(&=, BitAndAssign);
        BINARY_OPERATOR(|=, BitOrAssign);
        BINARY_OPERATOR(^=, BitXorAssign);
        BINARY_OPERATOR(<<=, LeftShiftAssign);
        BINARY_OPERATOR(>>=, RightShiftAssign);

#undef BINARY_OPERATOR
#undef POSTFIX_UNARY_OPERATOR
#undef PREFIX_UNARY_OPERATOR


#define PREFIX_UNARY_OPERATOR(op, name) \
    template<LambdaOperandConcept Arg> requires LambdaExpressionConcept<Arg> \
    auto operator op(Arg&& arg) \
    { \
        return name[std::forward<Arg>(arg)]; \
    }

#define POSTFIX_UNARY_OPERATOR(op, name) \
    template<LambdaOperandConcept Arg> requires LambdaExpressionConcept<Arg> \
    auto operator op(Arg&& arg, int) \
    { \
        return name[std::forward<Arg>(arg)]; \
    }

#define BINARY_OPERATOR(op, name) \
    template<LambdaOperandConcept Lhs, LambdaOperandConcept Rhs> requires(LambdaExpressionConcept<Lhs&&> || LambdaExpressionConcept<Rhs&&>) \
    auto operator op(Lhs&& lhs, Rhs&& rhs) \
    { \
        return name[std::forward<Lhs>(lhs), std::forward<Rhs>(rhs)]; \
    }


        PREFIX_UNARY_OPERATOR(*, Dereference);
        PREFIX_UNARY_OPERATOR(~, BitNot);
        PREFIX_UNARY_OPERATOR(!, LogicalNot);

        PREFIX_UNARY_OPERATOR(++, Increment);
        PREFIX_UNARY_OPERATOR(--, Decrement);

        POSTFIX_UNARY_OPERATOR(++, PostfixIncrement);
        POSTFIX_UNARY_OPERATOR(--, PostfixDecrement);


        BINARY_OPERATOR(*, Multiply);
        BINARY_OPERATOR(+, Plus);
        BINARY_OPERATOR(-, Minus);
        BINARY_OPERATOR(/ , Divide);
        BINARY_OPERATOR(%, Modulus);
        BINARY_OPERATOR(&, BitAnd);
        BINARY_OPERATOR(| , BitOr);
        BINARY_OPERATOR(^, BitXor);
        BINARY_OPERATOR(&&, LogicalAnd);
        BINARY_OPERATOR(|| , LogicalOr);
        BINARY_OPERATOR(<< , LeftShift);
        BINARY_OPERATOR(>> , RightShift);
        BINARY_OPERATOR(->*, Projection);

        BINARY_OPERATOR(< , Less);
        BINARY_OPERATOR(> , Greater);
        BINARY_OPERATOR(<= , LessEqual);
        BINARY_OPERATOR(>= , GreaterEqual);
        BINARY_OPERATOR(== , Equal);
        BINARY_OPERATOR(!= , NotEqual);
        BINARY_OPERATOR(<=> , Spaceship);

        BINARY_OPERATOR(+=, PlusAssign);
        BINARY_OPERATOR(-=, MinusAssign);
        BINARY_OPERATOR(*=, MultiplyAssign);
        BINARY_OPERATOR(/=, DivideAssign);
        BINARY_OPERATOR(%=, ModulusAssign);
        BINARY_OPERATOR(&=, BitAndAssign);
        BINARY_OPERATOR(|=, BitOrAssign);
        BINARY_OPERATOR(^=, BitXorAssign);
        BINARY_OPERATOR(<<=, LeftShiftAssign);
        BINARY_OPERATOR(>>=, RightShiftAssign);

#undef BINARY_OPERATOR
#undef POSTFIX_UNARY_OPERATOR
#undef PREFIX_UNARY_OPERATOR
    }


    template<typename Type>
    concept PipelinableTo = std::derived_from<std::remove_cvref_t<Type>, ExtensionMethod> || meta::InstanceOfConcept<Type, ExtensionClosure> || meta::InstanceOfConcept<Type, ExtensionMethod::PartialApplier>;

    template<typename Arg, PipelinableTo Extension>
    decltype(auto) operator|(Arg&& arg, Extension&& ext)
    {
        return std::forward<Extension>(ext)(std::forward<Arg>(arg));
    }
    
}
