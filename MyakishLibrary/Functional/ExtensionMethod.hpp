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


    template<typename Lambda, typename ...Args>
    concept Resolvable = LambdaExpressionConcept<Lambda> && requires(const Lambda resolver, Args&&... args)
    {
        resolver.LambdaResolve(std::forward<Args>(args)...);
    };


    struct LambdaExpressionTag {};

    template<std::derived_from<LambdaExpressionTag> Type>
    struct LambdaExpression<Type> : std::true_type {};

    namespace detail
    {
        template<myakish::Size Index, typename ...Types, typename ReturnType = meta::At<Index, meta::TypeList<Types...>>::type>
        constexpr ReturnType ForwardFromTuple(const std::tuple<Types...>& tuple) requires std::is_reference_v<ReturnType>
        {
            return std::forward<ReturnType>(std::get<Index>(tuple));
        }


        template<typename Invocable, typename ...Types, myakish::Size ...Indices> requires(std::invocable<Invocable&&, Types&&...>)
            constexpr decltype(auto) ForwardingApply(Invocable&& invocable, const std::tuple<Types...>& tuple, std::integer_sequence<myakish::Size, Indices...>)
        {
            return std::invoke(std::forward<Invocable>(invocable), ForwardFromTuple<Indices>(tuple)...);
        }

        template<typename Invocable, typename ...Types> requires(std::invocable<Invocable&&, Types&&...>)
            constexpr decltype(auto) ForwardingApply(Invocable&& invocable, const std::tuple<Types...>& tuple)
        {
            return ForwardingApply(std::forward<Invocable>(invocable), tuple, std::make_integer_sequence<myakish::Size, sizeof...(Types)>{});
        }

        /*template<myakish::Size Start, myakish::Size Count, typename Invocable, typename ...Types> requires(std::invocable<Invocable&&, Types&&...>)
        constexpr decltype(auto) ForwardingApply(Invocable&& invocable, const std::tuple<Types...>& tuple)
        {
            return ForwardingApply(std::forward<Invocable>(invocable), tuple, std::make_integer_sequence<myakish::Size, sizeof...(Types)>{});
        }*/
    }

    struct LambdaMarkerType : LambdaExpressionTag
    {
        template<std::invocable Invocable, typename ArgsTuple>
        constexpr decltype(auto) LambdaResolve(Invocable&& invocable, const ArgsTuple& argsTuple) const
        {
            return std::invoke(std::forward<Invocable>(invocable));
        }
    };
    inline constexpr LambdaMarkerType LambdaMarker;


    template<myakish::Size Index>
    struct Placeholder : LambdaExpressionTag
    {
        template<typename Invocable, typename ArgsTuple> requires(0 <= Index && Index < std::tuple_size_v<std::remove_cvref_t<ArgsTuple>> && std::invocable<Invocable&&, std::tuple_element_t<Index, ArgsTuple>>)
            constexpr decltype(auto) LambdaResolve(Invocable&& invocable, const ArgsTuple& argsTuple) const
        {
            return std::invoke(std::forward<Invocable>(invocable), detail::ForwardFromTuple<Index>(argsTuple));
        }
    };
    template<myakish::Size Index>
    inline constexpr Placeholder<Index> Arg;

    struct RangePlaceholder : LambdaExpressionTag
    {
        template<typename Invocable, typename ArgsTuple>
        constexpr decltype(auto) LambdaResolve(Invocable&& invocable, const ArgsTuple& argsTuple) const requires requires { detail::ForwardingApply(std::forward<Invocable>(invocable), argsTuple); }
        {
            return detail::ForwardingApply(std::forward<Invocable>(invocable), argsTuple);
        }
    };
    inline constexpr RangePlaceholder Args;


    namespace shorthands
    {
        inline constexpr LambdaMarkerType $e;

        inline constexpr Placeholder<0> $0;
        inline constexpr Placeholder<1> $1;
        inline constexpr Placeholder<2> $2;
        inline constexpr Placeholder<3> $3;
        inline constexpr Placeholder<4> $4;
        inline constexpr Placeholder<5> $5;
        inline constexpr Placeholder<6> $6;
        inline constexpr Placeholder<7> $7;

        inline constexpr RangePlaceholder $r;
    }

    template<typename Value>
    struct ConstantExpression : LambdaExpressionTag
    {
        Value value;

        constexpr ConstantExpression(Value value) : value(std::forward<Value>(value)) {}

        template<std::invocable<Value> Invocable, typename ArgsTuple>
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
        constexpr auto MakeExpression(NonLambda&& value)
        {
            return ConstantExpression(std::forward<NonLambda>(value));
        }

        template<LambdaExpressionConcept Lambda>
        constexpr Lambda&& MakeExpression(Lambda&& lambda)
        {
            return std::forward<Lambda>(lambda);
        }

#pragma warning(disable: 26800)

        template<std::invocable Continuation>
        constexpr decltype(auto) IndirectAccumulate(Continuation&& continuation)
        {
            return std::invoke(std::forward<Continuation>(continuation));
        }

        template<typename Invocable, typename... Bound>
        struct FrontBind
        {
            Invocable&& invocable;
            std::tuple<Bound&&...> bound;

            constexpr FrontBind(Invocable&& invocable, Bound&&... bound) : invocable(std::forward<Invocable>(invocable)), bound(std::forward<Bound>(bound)...) {}

            template<typename ...Args> requires std::invocable<Invocable&&, Bound&&..., Args&&...>
            constexpr decltype(auto) operator()(Args&&... args) const
            {
                auto InvokeTarget = [&](Bound&&... resolvedBound) -> decltype(auto)
                {
                    return std::invoke(std::forward<Invocable>(invocable), std::forward<Bound>(resolvedBound)..., std::forward<Args>(args)...);
                };

                return detail::ForwardingApply(InvokeTarget, bound);
            }
        };
        template<typename Invocable, typename... Bound>
        FrontBind(Invocable&&, Bound&&...) -> FrontBind<Invocable&&, Bound&&...>;

        template<typename Continuation, typename ...Rest>
        struct IndirectAccumulator;

        template<typename Continuation, typename First, typename ...Rest> requires std::invocable<First&&, IndirectAccumulator<Continuation&&, Rest&&...>>
        constexpr decltype(auto) IndirectAccumulate(Continuation&& continuation, First&& first, Rest&&... rest)
        {
            return std::invoke(std::forward<First>(first), IndirectAccumulator(std::forward<Continuation>(continuation), std::forward<Rest>(rest)...));
        }

        template<typename Continuation, typename ...IndirectTransforms>
        concept IndirectAccumulatable = requires(Continuation && continuation, IndirectTransforms&&... indirectTransforms)
        {
            IndirectAccumulate(std::forward<Continuation>(continuation), std::forward<IndirectTransforms>(indirectTransforms)...);
        };

        template<typename Continuation, typename ...Rest>
        struct IndirectAccumulator
        {
            Continuation&& continuation;
            std::tuple<Rest&&...> rest;

            constexpr IndirectAccumulator(Continuation&& continuation, Rest&&... rest) : continuation(std::forward<Continuation>(continuation)), rest(std::forward<Rest>(rest)...) {}

            template<typename... ResolvedArgs> requires IndirectAccumulatable<FrontBind<Continuation&&, ResolvedArgs&&...>&&, Rest&&...>
            constexpr decltype(auto) operator()(ResolvedArgs&&... resolvedArgs)
            {
                auto InvokeTarget = [&](Rest&&... resolvedRest) -> decltype(auto)
                    {
                        return IndirectAccumulate(FrontBind(std::forward<Continuation>(continuation), std::forward<ResolvedArgs>(resolvedArgs)...), std::forward<Rest>(resolvedRest)...);
                    };


                return detail::ForwardingApply(InvokeTarget, rest);
            };
        };
        template<typename Continuation, typename... Rest>
        IndirectAccumulator(Continuation&&, Rest&&...) -> IndirectAccumulator<Continuation&&, Rest&&...>;





        template<LambdaExpressionConcept Resolver, typename ArgsTuple>
        struct LambdaIndirectTransform
        {
            const Resolver& resolver;
            const ArgsTuple& argsTuple;

            constexpr LambdaIndirectTransform(const Resolver& resolver, const ArgsTuple& argsTuple) : resolver(resolver), argsTuple(argsTuple) {}

            template<typename Continuation> requires Resolvable<const Resolver&, Continuation&&, const ArgsTuple&>
            constexpr decltype(auto) operator()(Continuation&& continuation) const
            {
                return resolver.LambdaResolve(std::forward<Continuation>(continuation), argsTuple);         
            }
        };
        template<LambdaExpressionConcept Resolver, typename ArgsTuple>
        LambdaIndirectTransform(const Resolver&, const ArgsTuple&) -> LambdaIndirectTransform<Resolver, ArgsTuple>;

        template<typename Invocable, typename ArgsTuple, LambdaExpressionConcept ...Resolvers> requires IndirectAccumulatable<Invocable&&, LambdaIndirectTransform<Resolvers, ArgsTuple>...>
        constexpr decltype(auto) LambdaInvoke(Invocable&& invocable, const ArgsTuple& argsTuple, const Resolvers&... resolvers)
        {
            return IndirectAccumulate(std::forward<Invocable>(invocable), LambdaIndirectTransform(resolvers, argsTuple)...);
        }
    }

    template<typename Invocable, LambdaExpressionConcept ...ArgResolvers>
    struct LambdaClosure : LambdaExpressionTag
    {
        Invocable&& baseInvocable;
        std::tuple<ArgResolvers...> resolvers;

        constexpr LambdaClosure(Invocable&& baseInvocable, ArgResolvers... resolvers) : baseInvocable(std::forward<Invocable>(baseInvocable)), resolvers(std::move(resolvers)...) {}

        template<typename TargetInvocable, typename ArgsTuple>
        constexpr decltype(auto) LambdaResolve(TargetInvocable&& targetInvocable, const ArgsTuple& argsTuple) const
        {
            auto Transform = [&]<typename ...Args>(Args&&... args) -> decltype(auto)
                {
                    return std::invoke(std::forward<TargetInvocable>(targetInvocable), std::invoke(std::forward<Invocable>(baseInvocable), std::forward<Args>(args)...));
                };

            auto InvokeTarget = [&](const ArgResolvers&... resolvedResolvers) -> decltype(auto)
                {
                    return detail::LambdaInvoke(Transform, argsTuple, resolvedResolvers...);
                };

            return std::apply(InvokeTarget, resolvers);
        }

        template<typename ...Args>
        constexpr decltype(auto) operator()(Args&&... args) const
        {
            auto InvokeTarget = [&](const ArgResolvers&... resolvedResolvers) -> decltype(auto)
            {
                return detail::LambdaInvoke(std::forward<Invocable>(baseInvocable), std::forward_as_tuple(std::forward<Args>(args)...), resolvedResolvers...);
            };

            return std::apply(InvokeTarget, resolvers);
        }

    private:
        /*
        template<typename ...Args>
        constexpr decltype(auto) Invoke(Args&&... args) const
        {
            return Dispatch(std::make_index_sequence<sizeof...(ArgResolvers)>{}, std::forward<Args>(args)...);
        }

        template<typename ...Args, std::size_t ...Indices>
        constexpr decltype(auto) Dispatch(std::index_sequence<Indices...>, Args&&... args) const
        {
            return detail::LambdaInvoke(std::forward<Invocable>(baseInvocable), std::forward_as_tuple(std::forward<Args>(args)...), std::get<Indices>(resolvers)...);
        }*/
    };

    template<typename Invocable, typename ...ArgResolvers>
    LambdaClosure(Invocable&&, ArgResolvers...) -> LambdaClosure<Invocable&&, ArgResolvers...>;


    template<typename Invocable, typename ...CurryArgs>
    struct ExtensionClosure;

    struct ExtensionMethod
    {
        template<typename Self, typename ...Args>
        constexpr auto operator[](this Self&& self, Args&&... args)
        {
            if constexpr (meta::QuotedAnyOf<meta::Compose<std::remove_cvref, LambdaExpression>, meta::TypeList<Args&&...>>::value)
                return LambdaClosure(std::forward<Self>(self), detail::MakeExpression(std::forward<Args>(args))...);
            else
                return ExtensionClosure(std::forward<Self>(self), std::forward<Args>(args)...);
        }
    };

    template<typename Invocable, typename ...CurryArgs>
    struct ExtensionClosure : ExtensionMethod
    {
        Invocable&& baseInvocable;
        std::tuple<CurryArgs&&...> curryArgs;

        constexpr ExtensionClosure(Invocable&& baseInvocable, CurryArgs&&... curryArgs) : baseInvocable(std::forward<Invocable>(baseInvocable)), curryArgs(std::forward<CurryArgs>(curryArgs)...) {}


        template<typename ...Args> requires std::invocable<Invocable&&, Args&&..., CurryArgs&&...>
        constexpr decltype(auto) operator()(Args&&... args) const
        {
            return Dispatch(std::make_index_sequence<sizeof...(CurryArgs)>{}, std::forward<Args>(args)...);
        }

    private:

        template<typename ...Args, std::size_t ...Indices> requires std::invocable<Invocable&&, Args&&..., CurryArgs&&...>
        constexpr decltype(auto) Dispatch(std::index_sequence<Indices...> seq, Args&&... args) const
        {
            return std::invoke(std::forward<Invocable>(baseInvocable), std::forward<Args>(args)..., std::forward<CurryArgs>(std::get<Indices>(curryArgs))...);
        }
    };
    template<typename Invocable, typename ...CurryArgs>
    ExtensionClosure(Invocable&&, CurryArgs&&...) -> ExtensionClosure<Invocable&&, CurryArgs&&...>;



    struct IndirectAccumulateFunctor : ExtensionMethod
    {
        template<typename Continuation, typename ...IndirectTransforms> requires detail::IndirectAccumulatable<Continuation&&, IndirectTransforms&&...>
        constexpr decltype(auto) operator()(Continuation&& continuation, IndirectTransforms&&... indirectTransforms) const
        {
            return detail::IndirectAccumulate(std::forward<Continuation>(continuation), std::forward<IndirectTransforms>(indirectTransforms)...);
        }
    };
    inline constexpr IndirectAccumulateFunctor IndirectAccumulate;



    template<typename Type>
    struct LambdaOperand : std::true_type {};

    template<typename Type>
    concept LambdaOperandConcept = LambdaOperand<std::remove_cvref_t<Type>>::value;


    struct DisableLambdaOperatorsTag {};

    template<std::derived_from<DisableLambdaOperatorsTag> Type>
    struct LambdaOperand<Type> : std::false_type {};


    template<typename Expression>
    struct CompleteLambda : ExtensionMethod
    {
        Expression expression;

        constexpr CompleteLambda(Expression expression) : expression(std::move(expression)) {}

        template<typename ...Args>
        constexpr decltype(auto) operator()(Args&&... args) const
        {
            return expression(std::forward<Args>(args)...);
        }
    };
    template<typename Expression>
    CompleteLambda(Expression) -> CompleteLambda<Expression>;

    struct CompleteFunctor : ExtensionMethod, DisableLambdaOperatorsTag
    {
        template<typename Expression> 
        constexpr auto operator()(Expression&& expression) const
        {
            return CompleteLambda(std::forward<Expression>(expression));
        }
        template<meta::DerivedFrom<ExtensionMethod> Expression>
        constexpr auto operator()(Expression&& expression) const
        {
            return std::forward<Expression>(expression);
        }

        template<typename Expression>
        constexpr auto operator=(Expression&& expression) const
        {
            return operator()(std::forward<Expression>(expression));
        }
    };
    inline constexpr CompleteFunctor Complete;

    namespace shorthands
    {
        inline constexpr auto λ = Complete;
    }

    template<typename Transform, typename Continuation>
    struct TransformedContinuation
    {
        Transform&& transform;
        Continuation&& continuation;

        constexpr TransformedContinuation(Transform&& transform, Continuation&& continuation) : transform(std::forward<Transform>(transform)), continuation(std::forward<Continuation>(continuation)) {}

        template<typename ...Args> requires((std::invocable<Transform&&, Args&&>) && ...)
        constexpr decltype(auto) operator()(Args&&... args) const
        {
            return IndirectAccumulate(std::forward<Continuation>(continuation), std::invoke(std::forward<Transform>(transform), std::forward<Args>(args))...);
        }
    };
    template<typename Transform, typename Continuation>
    TransformedContinuation(Transform&&, Continuation&&) -> TransformedContinuation<Transform&&, Continuation&&>;

    template<LambdaExpressionConcept Expression, typename Transform>
    struct LambdaTransform : LambdaExpressionTag
    {      
        Expression expression;
        Transform&& transform;

        constexpr LambdaTransform(Expression expression, Transform&& transform) : expression(std::move(expression)), transform(std::forward<Transform>(transform)) {}

        template<typename Self, typename Invocable, typename ArgsTuple>
        constexpr decltype(auto) LambdaResolve(this Self&& self, Invocable&& invocable, const ArgsTuple& argsTuple) requires Resolvable<Expression, TransformedContinuation<Transform&&, Invocable&&>, const ArgsTuple&>
        {
            return self.expression.LambdaResolve(TransformedContinuation(std::forward<Transform>(self.transform), std::forward<Invocable>(invocable)), argsTuple);
        }
    };
    template<LambdaExpressionConcept Expression, typename Transform>
    LambdaTransform(Expression, Transform&&) -> LambdaTransform<Expression, Transform&&>;


    inline namespace higher_order
    {

#define PREFIX_UNARY_OPERATOR(op, name) \
    struct name##Functor : ExtensionMethod \
    { \
        template<class Arg>\
        constexpr decltype(auto) operator()(Arg&& arg) const requires requires { op std::forward<Arg>(arg); } \
        { \
            return op std::forward<Arg>(arg); \
        } \
    }; \
    inline constexpr name##Functor name;

#define POSTFIX_UNARY_OPERATOR(op, name) \
    struct name##Functor : ExtensionMethod \
    { \
        template<class Arg>\
        constexpr decltype(auto) operator()(Arg&& arg) const requires requires { std::forward<Arg>(arg) op; } \
        { \
            return std::forward<Arg>(arg) op; \
        } \
    }; \
    inline constexpr name##Functor name;

#define BINARY_OPERATOR(op, name) \
    struct name##Functor : ExtensionMethod \
    { \
        template<typename Lhs, typename Rhs> \
        constexpr decltype(auto) operator()(Lhs&& lhs, Rhs&& rhs) const requires requires { std::forward<Lhs>(lhs) op std::forward<Rhs>(rhs); } \
        { \
            return std::forward<Lhs>(lhs) op std::forward<Rhs>(rhs); \
        } \
    }; \
    inline constexpr name##Functor name;

        struct PlusFunctor : ExtensionMethod
        {
            template<typename Lhs, typename Rhs> 
            constexpr decltype(auto) operator()(Lhs&& lhs, Rhs&& rhs) const requires requires { std::forward<Lhs>(lhs) + std::forward<Rhs>(rhs); }
            {
                return std::forward<Lhs>(lhs) + std::forward<Rhs>(rhs);
            }

            template<typename Arg>
            constexpr decltype(auto) operator()(Arg&& arg) const requires requires { +std::forward<Arg>(arg); }
            {
                return +std::forward<Arg>(arg);
            }
        };
        inline constexpr PlusFunctor Plus;

        struct MinusFunctor : ExtensionMethod
        {
            template<typename Lhs, typename Rhs>
            constexpr decltype(auto) operator()(Lhs&& lhs, Rhs&& rhs) const requires requires { std::forward<Lhs>(lhs) - std::forward<Rhs>(rhs); }
            {
                return std::forward<Lhs>(lhs) - std::forward<Rhs>(rhs);
            }

            template<typename Arg>
            constexpr decltype(auto) operator()(Arg&& arg) const requires requires { -std::forward<Arg>(arg); }
            {
                return -std::forward<Arg>(arg);
            }
        };
        inline constexpr MinusFunctor Minus;

        struct SubscriptFunctor : ExtensionMethod
        {
            template<typename Lhs, typename ...Args>
            constexpr decltype(auto) operator()(Lhs&& lhs, Args&&... args) const requires requires { std::forward<Lhs>(lhs)[std::forward<Args>(args)...]; }
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
    constexpr auto operator op(Arg&& arg) \
    { \
        return name[std::forward<Arg>(arg)]; \
    }

#define POSTFIX_UNARY_OPERATOR(op, name) \
    template<LambdaOperandConcept Arg> requires LambdaExpressionConcept<Arg> \
    constexpr auto operator op(Arg&& arg, int) \
    { \
        return name[std::forward<Arg>(arg)]; \
    }

#define BINARY_OPERATOR(op, name) \
    template<LambdaOperandConcept Lhs, LambdaOperandConcept Rhs> requires(LambdaExpressionConcept<Lhs&&> || LambdaExpressionConcept<Rhs&&>) \
    constexpr auto operator op(Lhs&& lhs, Rhs&& rhs) \
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
    concept PipelinableTo = meta::DerivedFrom<Type, ExtensionMethod>;

    template<typename Arg, PipelinableTo Extension> requires std::invocable<Extension&&, Arg&&>
    constexpr decltype(auto) operator|(Arg&& arg, Extension&& ext)
    {
        return std::invoke(std::forward<Extension>(ext), std::forward<Arg>(arg));
    }
    
    namespace detail
    {
        template<typename Only>
        constexpr Only&& RightFold(auto&&, Only&& only)
        {
            return std::forward<Only>(only);
        }

        template<typename First, typename... Rest, typename Invocable>
        constexpr decltype(auto) RightFold(Invocable&& invocable, First&& first, Rest&&... rest)
        {
            return std::invoke(std::forward<Invocable>(invocable), first, RightFold(std::forward<Invocable>(invocable), std::forward<Rest>(rest)...));
        }
    }

    inline namespace higher_order
    {
        struct ConstantFunctor : ExtensionMethod
        {
            constexpr auto operator()(auto value) const
            {
                return Complete = [=](auto&&...)
                    {
                        return value;
                    };
            }
        };
        inline constexpr ConstantFunctor Constant;

        struct RightFoldFunctor : ExtensionMethod
        {
            template<typename... Args, typename Invocable> 
            constexpr decltype(auto) operator()(Invocable&& invocable, Args&&... args) const
            {
                return detail::RightFold(std::forward<Invocable>(invocable), std::forward<Args>(args)...);
            }
        };
        inline constexpr RightFoldFunctor RightFold;

        template<typename Inner, typename Outer>
        struct Composite : ExtensionMethod
        {
            Inner inner;
            Outer outer;

            constexpr Composite(Inner inner, Outer outer) : inner(std::move(inner)), outer(std::move(outer)) {}

            template<typename Self, typename ...Args>
            constexpr decltype(auto) operator()(this Self&& self, Args&&... args) requires requires { std::invoke(std::forward<Self>(self).outer, std::invoke(std::forward<Self>(self).inner, std::forward<Args>(args)...)); }
            {
                return std::invoke(std::forward<Self>(self).outer, std::invoke(std::forward<Self>(self).inner, std::forward<Args>(args)...));
            }
        };
        template<typename Inner, typename Outer>
        Composite(Inner, Outer) -> Composite<Inner, Outer>;

        struct ComposeFunctor : ExtensionMethod
        {
            template<typename Inner, typename Outer>
            constexpr auto operator()(Inner inner, Outer outer) const
            {
                return Composite(std::move(inner), std::move(outer));
            }
        };
        inline constexpr ComposeFunctor Compose;


        struct MakeCopyFunctor : ExtensionMethod
        {
            template<typename Arg>
            constexpr auto operator()(Arg&& arg) const
            {
                return std::forward<Arg>(arg);
            }
        };
        inline constexpr MakeCopyFunctor MakeCopy;

        struct IdentityFunctor : ExtensionMethod
        {
            template<typename Arg>
            constexpr Arg&& operator()(Arg&& arg) const
            {
                return std::forward<Arg>(arg);
            }
        };
        inline constexpr IdentityFunctor Identity;

        template<typename Type>
        struct ConstructFunctor : ExtensionMethod
        {
            template<typename ...Args> requires std::constructible_from<Type, Args&&...>
            constexpr Type operator()(Args&&... args) const
            {
                return Type(std::forward<Args>(args)...);
            }
        };
        template<typename Type>
        inline constexpr ConstructFunctor<Type> Construct;

        template<template<typename...> typename Template>
        struct DeduceConstructFunctor : ExtensionMethod
        {
            template<typename ...Args>
            auto operator()(Args&&... args) const
            {
                return Template(std::forward<Args>(args)...);
            }
        };
        template<template<typename...> typename Template>
        inline constexpr DeduceConstructFunctor<Template> DeduceConstruct;


        template<typename Type>
        struct StaticCastFunctor : ExtensionMethod
        {
            template<std::convertible_to<Type> Arg>
            constexpr auto operator()(Arg&& arg) const
            {
                return static_cast<Type>(std::forward<Arg>(arg));
            }
        };
        template<typename Type>
        inline constexpr StaticCastFunctor<Type> StaticCast;

        struct InvokeFunctor : ExtensionMethod
        {
            template<typename Invocable, typename ...Args> requires std::invocable<Invocable&&, Args&&...>
            constexpr decltype(auto) operator()(Invocable&& invocable, Args&&... args) const
            {
                return std::invoke(std::forward<Invocable>(invocable), std::forward<Args>(args)...);
            }
        };
        inline constexpr InvokeFunctor Invoke;


        struct IfFunctor : ExtensionMethod
        {
            template<typename True, typename False>
            constexpr decltype(auto) operator()(bool condition, True&& trueValue, False&& falseValue) const
            {
                return condition ? std::forward<True>(trueValue) : std::forward<False>(falseValue);
            }
        };
        inline constexpr IfFunctor If;


        struct BitWidthFunctor : ExtensionMethod
        {
            template<std::unsigned_integral Number>
            constexpr int operator()(Number num) const
            {
                return std::bit_width(num);
            }
        };
        inline constexpr BitWidthFunctor BitWidth;

        struct MaxFunctor : ExtensionMethod
        {
            template<typename Type>
            constexpr const Type& operator()(const Type& f, const Type& s) const
            {
                return std::max(f, s);
            }
        };
        inline constexpr MaxFunctor Max;

        struct MinFunctor : ExtensionMethod
        {
            template<typename Type>
            constexpr const Type& operator()(const Type& f, const Type& s) const
            {
                return std::min(f, s);
            }
        };
        inline constexpr MinFunctor Min;

    }
    
    template<typename Inner, typename Outer> requires(PipelinableTo<Inner> || PipelinableTo<Outer>)
    constexpr decltype(auto) operator>>(Inner inner, Outer outer)
    {
        return Compose(std::move(inner), std::move(outer));
    }


    namespace detail
    {
        template<typename Invocable>
        struct SelectOverloader : std::type_identity<Invocable> {};

        template<typename ReturnType, typename ...Args>
        struct FunctionPointerOverloader
        {
            using Invocable = ReturnType(*)(Args...);

            Invocable invocable;

            FunctionPointerOverloader(Invocable invocable) : invocable(invocable) {}

            ReturnType operator()(Args... args) const
            {
                return std::invoke(invocable, std::forward<Args>(args)...);
            }
        };
        template<typename ReturnType, typename ...Args>
        struct SelectOverloader<ReturnType(*)(Args...)> : meta::ReturnType<FunctionPointerOverloader<ReturnType, Args...>> {};

        template<typename Class, typename Field>
        struct MemberPointerOverloader
        {
            using Invocable = Field Class::*;

            Invocable invocable;

            MemberPointerOverloader(Invocable invocable) : invocable(invocable) {}

            template<meta::SameBaseConcept<Class> Object>
            decltype(auto) operator()(Object&& object) const
            {
                return std::invoke(invocable, std::forward<Object>(object));
            }
        };
        template<typename Class, typename Field>
        struct SelectOverloader<Field Class::*> : meta::ReturnType<MemberPointerOverloader<Class, Field>> {};
    }

    template<typename ...Functions>
    struct Overloads : Functions...
    {
        using Functions::operator()...;
    };
    template<typename ...Functions>
    Overloads(Functions...) -> Overloads<typename detail::SelectOverloader<Functions>::type...>;

    inline namespace higher_order
    {
        inline constexpr auto Overload = DeduceConstruct<Overloads>;
    }
}
